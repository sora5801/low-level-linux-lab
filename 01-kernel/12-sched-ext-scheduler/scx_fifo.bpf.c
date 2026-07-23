/* SPDX-License-Identifier: GPL-2.0 */
/* ===========================================================================
 * scx_fifo.bpf.c — a working CPU scheduler, written as a sched_ext (SCX) BPF
 * program. This is the KERNEL half: it runs *inside* the kernel, in BPF, and
 * makes the actual scheduling decisions for every task on the machine while it
 * is loaded. The userspace half (scx_fifo.c) only loads it and prints stats.
 * ===========================================================================
 *
 * WHAT sched_ext IS
 * -----------------
 * Normally Linux schedules with EEVDF (formerly CFS) — C code baked into the
 * kernel. sched_ext (merged in v6.12) adds a scheduling *class* that delegates
 * the policy to a BPF program. You implement a `struct sched_ext_ops` — a table
 * of callbacks — and the core kernel calls them at the right moments. Load the
 * program and, atomically, every SCHED_NORMAL task starts being scheduled by
 * YOUR code. Unload it (or crash it) and the kernel falls back to EEVDF without
 * missing a beat. That safety net — you cannot hang the box with a bad policy,
 * the watchdog ejects you — is the whole reason this is a sane place to learn
 * scheduling.
 *
 * THE VOCABULARY YOU MUST HOLD IN YOUR HEAD
 * -----------------------------------------
 *   task_struct        one runnable thread. `p->scx` is its scheduler state:
 *                      p->scx.slice   = time budget (ns) it may run before
 *                                       being preempted; the kernel decrements
 *                                       it as the task runs.
 *                      p->scx.weight  = priority weight (nice 0 == 100; higher
 *                                       weight == bigger share of the CPU).
 *                      p->scx.dsq_vtime = this task's virtual-time cursor, the
 *                                       quantity we sort by in weighted mode.
 *
 *   DSQ (Dispatch Queue)  the queues tasks wait in. Two flavours:
 *       - per-CPU LOCAL DSQ (SCX_DSQ_LOCAL): the kernel pulls the next task to
 *         run on a CPU from that CPU's local DSQ. This is the real "run queue."
 *       - user DSQs you create with scx_bpf_create_dsq(): shared staging queues.
 *         We make ONE global shared DSQ (id SHARED_DSQ) that every CPU feeds
 *         from — that is what makes this a *global* scheduler.
 *
 * THE LIFECYCLE OF ONE SCHEDULING DECISION (follow a task through the ops)
 * -----------------------------------------------------------------------
 *   1. task wakes  -> .select_cpu()  we pick a target CPU; if an idle CPU is
 *                                     free we can shove the task straight onto
 *                                     that CPU's LOCAL DSQ (fast path).
 *   2. still queued -> .enqueue()     we insert the task into the SHARED DSQ,
 *                                     either FIFO-ordered or vtime-ordered.
 *   3. a CPU goes idle -> .dispatch() we move one task from the SHARED DSQ to
 *                                     that CPU's LOCAL DSQ so it runs next.
 *   4. task starts  -> .running()     bookkeeping (advance the global vtime).
 *   5. task stops   -> .stopping()    charge the vtime it just consumed,
 *                                     scaled by the inverse of its weight.
 * That five-step loop, repeated billions of times, IS a CPU scheduler.
 *
 * WHAT THIS TEACHING-CORE IS AND IS NOT
 * -------------------------------------
 * IS:  a correct, loadable, global scheduler with two policies you can flip
 *      between at load time — strict global FIFO, or weighted fair-share by
 *      virtual time (a tiny EEVDF cousin). It demonstrates select_cpu /
 *      enqueue / dispatch / the run queues / and vtime arithmetic end to end.
 * IS NOT: a production scheduler. It has no per-CPU/per-domain run queues, no
 *      load balancing across NUMA nodes, no cgroup/bandwidth control, no
 *      latency-nice, no core scheduling, no CPU-frequency coupling. The single
 *      shared DSQ is a global lock hot-spot that would not scale to a big box.
 *      The README's "Going further" says exactly what a real one (scx_rusty,
 *      scx_layered) adds.
 * =========================================================================== */

/* scx/common.bpf.h is the sched_ext BPF prelude. It pulls in vmlinux.h (the
 * CO-RE description of every kernel type, so `struct task_struct` etc. are in
 * scope), the libbpf helper macros, the declarations of every scx_bpf_* kfunc
 * we call below, and the BPF_STRUCT_OPS / SCX_OPS_DEFINE / UEI_* macros. It
 * ships in the kernel tree at tools/sched_ext/include/scx/ and in the
 * sched-ext/scx userspace repo. See the README for exactly where to get it. */
#include <scx/common.bpf.h>

/* The BPF verifier refuses to load a program without a license, and calling
 * GPL-only kernel kfuncs (which most scx_bpf_* helpers are) REQUIRES a GPL
 * license string here. "GPL" is not paperwork — it is a load-time gate. */
char _license[] SEC("license") = "GPL";

/* UEI = "user exit info." When our scheduler is unloaded — cleanly, or because
 * the kernel's watchdog ejected it for misbehaving — the kernel hands .exit()
 * a `struct scx_exit_info` describing why. This macro declares a BSS object the
 * userspace loader can read back to print the reason. It is how you find out
 * "the verifier killed me" vs "you hit Ctrl-C." */
UEI_DEFINE(uei);

/* Chosen at LOAD time by the loader (a `const volatile` becomes a read-only
 * .rodata value the verifier can constant-fold, so the branch on it costs
 * nothing at runtime). false => weighted-vtime policy; true => strict FIFO.
 * The loader sets this from the `-f` command-line flag before load. */
const volatile bool fifo_sched;

/* The id of the single global shared DSQ we create in .init(). Any non-negative
 * number that isn't a reserved SCX_DSQ_* constant works; 0 is conventional. */
#define SHARED_DSQ 0

/* -------------------------------------------------------------------------
 * Per-CPU statistics so the loader can show WHERE tasks got queued: on a
 * CPU-local DSQ (the select_cpu fast path found an idle CPU) or on the global
 * shared DSQ (the normal path). A PERCPU array means each CPU increments its
 * OWN copy with no atomics and no cache-line ping-pong; the loader sums the
 * per-CPU values when it reads them. index 0 = local, index 1 = global.
 * ------------------------------------------------------------------------- */
struct {
	__uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
	__uint(key_size, sizeof(u32));
	__uint(value_size, sizeof(u64));
	__uint(max_entries, 2);			/* [0]=local, [1]=global */
} stats SEC(".maps");

static void stat_inc(u32 idx)
{
	/* map_lookup on a PERCPU_ARRAY returns THIS CPU's slot. It can only fail
	 * if idx is out of range; we still null-check because the verifier
	 * demands every map-lookup result be checked before use — an unchecked
	 * deref is an instant load rejection, not a runtime crash. */
	u64 *cnt = bpf_map_lookup_elem(&stats, &idx);
	if (cnt)
		(*cnt)++;
}

/* =========================================================================
 * VIRTUAL-TIME MACHINERY (the weighted-fair policy)
 *
 * The idea, borrowed from CFS/EEVDF: give every task a virtual clock,
 * `dsq_vtime`. A task's vtime advances by (real time it ran) * (100 / weight).
 * A high-weight task's clock therefore ticks SLOWER, so it keeps sorting to the
 * front of the queue and gets scheduled more often. Always run the task with
 * the smallest vtime => everyone converges to a fair, weight-proportional share
 * of the CPU. We keep a global `vtime_now` as the "current time" reference so a
 * task that slept a long time cannot hoard unbounded budget.
 * ========================================================================= */

/* The global virtual-time cursor. Read/written from multiple CPUs without a
 * lock (see .running below) — deliberately racy, and safe because vtime is
 * monotonic and a stale read only nudges fairness by a hair, never corrupts. */
static u64 vtime_now;

/* Wrap-safe "does vtime a come before vtime b?"  vtime is a u64 that WILL wrap
 * around after ~584 years of ns... but more importantly, comparing absolute
 * u64s breaks the instant the two values straddle a wrap. The trick: compute
 * the DIFFERENCE and look at its sign as a SIGNED integer. (s64)(a - b) < 0 is
 * true exactly when a is "behind" b within half the u64 range, which is the
 * only regime we ever care about. This is the same idiom the TCP sequence-
 * number code uses. Get this wrong and a scheduler starves tasks at wrap. */
static inline bool vtime_before(u64 a, u64 b)
{
	return (s64)(a - b) < 0;
}

/* -------------------------------------------------------------------------
 * .select_cpu — "a task just woke; which CPU should it aim for?"
 *
 * Called with the task, the CPU it last ran on, and wakeup flags. Returning a
 * CPU is a HINT, but this callback is also allowed to dispatch the task
 * directly. We use scx_bpf_select_cpu_dfl(), the kernel's built-in idle-CPU
 * picker: it honours cache/topology (prefer prev_cpu, then an idle SMT sibling,
 * then any idle CPU) and tells us via is_idle whether it found a truly idle
 * one. If it did, we take the FAST PATH: insert straight onto that CPU's LOCAL
 * DSQ so the task runs the moment the CPU is free, skipping the shared queue
 * entirely. This is the single most important latency optimization in any SCX
 * scheduler — an idle CPU should never sit idle while a wakeable task waits.
 * ------------------------------------------------------------------------- */
s32 BPF_STRUCT_OPS(fifo_select_cpu, struct task_struct *p, s32 prev_cpu,
		   u64 wake_flags)
{
	bool is_idle = false;
	s32 cpu;

	cpu = scx_bpf_select_cpu_dfl(p, prev_cpu, wake_flags, &is_idle);
	if (is_idle) {
		stat_inc(0);	/* local-DSQ fast path taken */
		/* scx_bpf_dsq_insert(p, dsq, slice, flags): put p on a DSQ with
		 * a time slice. SCX_DSQ_LOCAL == "the local DSQ of the CPU this
		 * runs on." SCX_SLICE_DFL is the default 20ms budget. Because we
		 * inserted onto a LOCAL DSQ, .enqueue() will NOT be called for
		 * this task — it is already placed. (Older kernels spell this
		 * kfunc scx_bpf_dispatch(); the DSQ-centric name is the modern
		 * one.) */
		scx_bpf_dsq_insert(p, SCX_DSQ_LOCAL, SCX_SLICE_DFL, 0);
	}

	/* If not idle, we fall through: the task will reach .enqueue() next and
	 * we queue it globally there. `cpu` is still returned as the affinity
	 * hint the kernel uses when it later runs the task. */
	return cpu;
}

/* -------------------------------------------------------------------------
 * .enqueue — "make this runnable task wait somewhere until a CPU wants it."
 *
 * This is the policy's core. Every task that did NOT take the select_cpu fast
 * path lands here, and we drop it into the single SHARED DSQ. HOW we order it
 * inside that DSQ is the entire difference between our two policies:
 *   - FIFO:  scx_bpf_dsq_insert appends to the tail. First in, first out. Fair
 *            in arrival order, ignores priority. Simple and predictable.
 *   - vtime: scx_bpf_dsq_insert_vtime inserts in SORTED order by a vtime key,
 *            so dispatch() always pulls the task with the smallest vtime =
 *            the one most "owed" CPU. That yields weighted fair-share.
 * ------------------------------------------------------------------------- */
void BPF_STRUCT_OPS(fifo_enqueue, struct task_struct *p, u64 enq_flags)
{
	stat_inc(1);	/* global-DSQ path taken */

	if (fifo_sched) {
		/* Strict global FIFO: tail-insert with a default slice. No
		 * priority, no vtime — arrival order only. */
		scx_bpf_dsq_insert(p, SHARED_DSQ, SCX_SLICE_DFL, enq_flags);
		return;
	}

	/* --- weighted-vtime policy --- */
	u64 vtime = p->scx.dsq_vtime;

	/* Anti-hoarding clamp. A task that slept for a long time would otherwise
	 * carry a vtime far *behind* vtime_now and, on waking, monopolise the
	 * CPU until its clock caught up — a burst of unfair priority. We refuse
	 * to let any task be more than one slice "in credit": if its vtime is
	 * further back than (vtime_now - one slice), snap it forward to exactly
	 * that floor. This bounds wakeup latency for everyone else. */
	if (vtime_before(vtime, vtime_now - SCX_SLICE_DFL))
		vtime = vtime_now - SCX_SLICE_DFL;

	/* Insert sorted by vtime. The DSQ keeps itself ordered; dispatch() will
	 * naturally pop the smallest-vtime task first. (Older name:
	 * scx_bpf_dispatch_vtime().) */
	scx_bpf_dsq_insert_vtime(p, SHARED_DSQ, SCX_SLICE_DFL, vtime, enq_flags);
}

/* -------------------------------------------------------------------------
 * .dispatch — "this CPU's local run queue is empty; feed it work."
 *
 * The kernel calls this when a CPU has nothing to run and needs the next task.
 * Our job: move one task from the SHARED DSQ onto this CPU's LOCAL DSQ.
 * scx_bpf_dsq_move_to_local() pops the head of the given DSQ (the smallest
 * vtime in weighted mode, the oldest arrival in FIFO mode) and hands it to the
 * calling CPU. It returns false if the shared DSQ was empty, in which case the
 * CPU simply goes idle — perfectly fine. (Older name: scx_bpf_consume().)
 *
 * This one line is where "global scheduler" is realised: every CPU pulls from
 * the same shared queue, so any task can run on any CPU, load-balancing for
 * free. The cost is that every dispatch touches the one shared DSQ's lock —
 * the scalability wall a production scheduler climbs with per-CPU DSQs. */
void BPF_STRUCT_OPS(fifo_dispatch, s32 cpu, struct task_struct *prev)
{
	scx_bpf_dsq_move_to_local(SHARED_DSQ);
}

/* -------------------------------------------------------------------------
 * .running — "the kernel is about to run task p on a CPU."
 *
 * Only meaningful in weighted mode: advance the GLOBAL vtime cursor. vtime_now
 * should track the frontier of virtual time so the enqueue() clamp above has a
 * sane reference. We only ever move it FORWARD (monotonic), never back. The
 * read-test-write is racy across CPUs and we accept that: the worst case is a
 * momentarily stale vtime_now, which perturbs fairness imperceptibly and self-
 * corrects on the next task that starts. A lock here would serialise every
 * context switch on the machine — far worse than a hair of imprecision. */
void BPF_STRUCT_OPS(fifo_running, struct task_struct *p)
{
	if (fifo_sched)
		return;

	if (vtime_before(vtime_now, p->scx.dsq_vtime))
		vtime_now = p->scx.dsq_vtime;
}

/* -------------------------------------------------------------------------
 * .stopping — "task p just stopped running (preempted, blocked, or yielded)."
 *
 * Charge the virtual time it consumed. The kernel decremented p->scx.slice as
 * the task ran, so (SCX_SLICE_DFL - p->scx.slice) is the ns it actually used of
 * its 20ms grant. We scale that by 100/weight: a task with weight 200 (higher
 * priority) is charged HALF as much virtual time for the same real time, so its
 * clock lags, so it keeps getting picked first. This single line is the entire
 * fairness mechanism. Integer math order matters: multiply by 100 BEFORE
 * dividing by weight to keep precision (see asm/demo.c, which extracts exactly
 * this arithmetic and shows the 64-bit multiply and divide the CPU runs). */
void BPF_STRUCT_OPS(fifo_stopping, struct task_struct *p, bool runnable)
{
	if (fifo_sched)
		return;

	p->scx.dsq_vtime += (SCX_SLICE_DFL - p->scx.slice) * 100 / p->scx.weight;
}

/* -------------------------------------------------------------------------
 * .enable — "task p is now managed by our scheduler."
 *
 * Called once when a task enters our scheduling class (at load time for every
 * existing task, and later for each new one). Seed its virtual clock to the
 * current global frontier so a freshly-enabled task neither starves nor
 * unfairly leaps ahead — it starts life exactly "on time." */
void BPF_STRUCT_OPS(fifo_enable, struct task_struct *p)
{
	p->scx.dsq_vtime = vtime_now;
}

/* -------------------------------------------------------------------------
 * .init — "our scheduler is being loaded; set up shared state."
 *
 * SLEEPABLE because creating a DSQ may allocate and sleep. We create the one
 * global shared DSQ here. -1 means "no specific NUMA node." A non-zero return
 * aborts the load — if the DSQ can't be created we must NOT come up half-built.
 * ------------------------------------------------------------------------- */
s32 BPF_STRUCT_OPS_SLEEPABLE(fifo_init)
{
	return scx_bpf_create_dsq(SHARED_DSQ, -1);
}

/* -------------------------------------------------------------------------
 * .exit — "our scheduler is being unloaded (cleanly or by the watchdog)."
 *
 * Record the exit info so the userspace loader can print why we stopped. After
 * this returns the kernel has already switched every task back to EEVDF. */
void BPF_STRUCT_OPS(fifo_exit, struct scx_exit_info *ei)
{
	UEI_RECORD(uei, ei);
}

/* -------------------------------------------------------------------------
 * The ops table. SCX_OPS_DEFINE builds the `struct sched_ext_ops` in a special
 * ".struct_ops.link" section; libbpf turns it into a kernel struct_ops object
 * at load, and attaching it is what makes this program THE scheduler. Every
 * callback is optional — the ones we omit (e.g. .tick, .yield, .cpu_acquire)
 * fall back to sensible kernel defaults. `.name` shows up in
 * /sys/kernel/sched_ext/ and in `dmesg`. `.timeout_ms` (left default here) is
 * the watchdog: if a runnable task is starved longer than that, the kernel
 * ejects us and calls .exit — the safety net that makes live experimentation
 * safe. */
SCX_OPS_DEFINE(fifo_ops,
	       .select_cpu	= (void *)fifo_select_cpu,
	       .enqueue		= (void *)fifo_enqueue,
	       .dispatch	= (void *)fifo_dispatch,
	       .running		= (void *)fifo_running,
	       .stopping	= (void *)fifo_stopping,
	       .enable		= (void *)fifo_enable,
	       .init		= (void *)fifo_init,
	       .exit		= (void *)fifo_exit,
	       .name		= "fifo");
