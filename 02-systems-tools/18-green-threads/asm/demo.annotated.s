# =============================================================================
# demo.annotated.s — asm/demo.s (-O1) explained instruction by instruction.
# =============================================================================
#
# HOW TO READ THIS FILE
# ---------------------
# This is the exact assembly clang emits for demo.c at -O1 (see demo.s for the
# untouched original), annotated line by line. AT&T syntax throughout:
#
#     op   source, destination        # movq %rsi, 8(%rdi)  =>  *(rdi+8) = rsi
#     %reg                             # a register
#     $imm                             # an immediate constant
#     N(%reg)                          # memory at address reg + N
#     (%r1,%r2)                        # memory at address r1 + r2
#
# Register widths are the SAME register: rax(64) / eax(32) / ax(16) / al(8).
# Writing eax zero-extends into rax, so clang uses `xorl %eax,%eax` (2 bytes) to
# zero all 64 bits of rax.
#
# THE SysV AMD64 ABI THIS CODE OBEYS
# ----------------------------------
#   arguments, in order:  rdi, rsi, rdx, rcx, r8, r9      (then the stack)
#   return value:         rax
#   caller-saved (volatile): rax, rcx, rdx, rsi, rdi, r8-r11  — free to clobber
#   callee-saved (must preserve): rbx, rbp, r12, r13, r14, r15, rsp
#   red zone:  128 bytes below rsp a LEAF function may use without moving rsp
#   stack alignment:  rsp % 16 == 0 at every `call`
#
# All the functions here are LEAF functions (they call nothing), so at -O1 clang
# keeps only a frame pointer for debuggability and otherwise works entirely in
# caller-saved registers and the red zone — no stack frame is allocated.
#
# THE STRUCT LAYOUTS (offsets you will see as displacements)
# ----------------------------------------------------------
#   task { void* rsp @0;  int state @8;  task* rq_next @16;  int id @24; }
#   runq { task* head @0;  task* tail @8; }
# So `8(%rax)` is task.state, `16(%rax)` is task.rq_next, `8(%rdi)` is runq.tail.
# These offsets are the SAME as gt_task in gt.h (ctx.rsp is first) — the demo is
# a faithful slice of the real scheduler.
# =============================================================================

	.file	"demo.c"
	.text

# =============================================================================
# usize align_up(usize n, usize a)      n -> rdi, a -> rsi, return -> rax
# -----------------------------------------------------------------------------
# Round n up to a multiple of the power-of-two a:  (n + (a-1)) & ~(a-1).
# The neat part: clang computes the mask ~(a-1) as -a, using the identity
# ~(a-1) == -a in two's complement. So no ~ is emitted at all.
# =============================================================================
	.globl	align_up
	.p2align	4
	.type	align_up,@function
align_up:
	pushq	%rbp                    # PROLOGUE: save caller's frame pointer
	movq	%rsp, %rbp              #   rbp = frame base (kept only for debug)
	leaq	(%rdi,%rsi), %rax       # rax = n + a   (LEA does the add, no flags)
	decq	%rax                    # rax = n + a - 1
	negq	%rsi                    # rsi = -a  ==  ~(a-1): the alignment MASK
	andq	%rsi, %rax              # rax = (n+a-1) & ~(a-1)  = rounded-up value
	popq	%rbp                    # EPILOGUE: restore caller's rbp
	retq                            # return rax
.Lfunc_end0:
	.size	align_up, .Lfunc_end0-align_up

# =============================================================================
# void runq_push(runq *q, task *t)      q -> rdi, t -> rsi
# -----------------------------------------------------------------------------
# Enqueue t at the tail of the FIFO. Two cases: empty queue vs non-empty. Note
# the quirk — clang only bothers to set up a frame pointer on the non-empty
# path; the code is otherwise branch-then-store.
# =============================================================================
	.globl	runq_push
	.p2align	4
	.type	runq_push,@function
runq_push:
	movq	$0, 16(%rsi)            # t->rq_next = NULL  (t will be the new tail)
	movq	8(%rdi), %rax           # rax = q->tail
	testq	%rax, %rax             # tail == NULL ?  (queue empty?)
	je	.LBB1_2                 #   yes -> handle empty queue below
# ---- non-empty: link the old tail to t ----
	pushq	%rbp                    # (clang keeps a frame only on this path)
	movq	%rsp, %rbp
	movq	%rsi, 16(%rax)          # q->tail->rq_next = t   (splice t on)
	popq	%rbp
	movq	%rsi, 8(%rdi)           # q->tail = t            (t is the new tail)
	retq
.LBB1_2:                                # ---- empty queue: t becomes head & tail
	movq	%rsi, (%rdi)            # q->head = t
	movq	%rsi, 8(%rdi)           # q->tail = t
	retq
.Lfunc_end1:
	.size	runq_push, .Lfunc_end1-runq_push

# =============================================================================
# task *runq_pop(runq *q)               q -> rdi, return -> rax
# -----------------------------------------------------------------------------
# Dequeue from the head (FIFO). Returns the old head or NULL, and nulls the tail
# when the queue becomes empty so a later push starts clean.
# =============================================================================
	.globl	runq_pop
	.p2align	4
	.type	runq_pop,@function
runq_pop:
	movq	(%rdi), %rax            # rax = t = q->head
	testq	%rax, %rax             # head == NULL ?
	je	.LBB2_1                 #   yes -> return NULL
# ---- non-empty ----
	pushq	%rbp
	movq	%rsp, %rbp
	movq	16(%rax), %rcx          # rcx = t->rq_next   (the new head)
	movq	%rcx, (%rdi)            # q->head = t->rq_next
	testq	%rcx, %rcx             # did the queue just become empty?
	jne	.LBB2_5                 #   no -> skip the tail fix-up
	movq	$0, 8(%rdi)            # q->tail = NULL   (queue is now empty)
.LBB2_5:
	movq	$0, 16(%rax)           # t->rq_next = NULL   (unlink the returned node)
	popq	%rbp
	retq                            # return t (in rax)
.LBB2_1:
	xorl	%eax, %eax             # rax = 0  (empty -> NULL)
	retq
.Lfunc_end2:
	.size	runq_pop, .Lfunc_end2-runq_pop

# =============================================================================
# task *pick_next(runq *q)              q -> rdi, return -> rax
# -----------------------------------------------------------------------------
# The scheduler's decision procedure: walk the queue and return (unlinked) the
# first task whose state == ST_READY(0), skipping WAITING/DEAD entries. This is
# the richest control flow in the file — a linked-list walk that must remember
# the PREVIOUS node so it can bridge over the one it unlinks, and must fix the
# tail pointer if it unlinks the last node.
#
# Register roles inside the walk:
#     rax = t     (candidate node currently under inspection)
#     rdx = t     (advanced form; becomes prev on the next step)
#     rcx = prev  (node before the match, or NULL if the match is the head)
# =============================================================================
	.globl	pick_next
	.p2align	4
	.type	pick_next,@function
pick_next:
	movq	(%rdi), %rax            # rax = t = q->head
	testq	%rax, %rax             # empty queue?
	je	.LBB3_1                 #   yes -> return NULL
	pushq	%rbp
	movq	%rsp, %rbp
	cmpl	$0, 8(%rax)            # is the HEAD already ST_READY (state==0)?
	je	.LBB3_4                 #   yes -> match at head, prev = NULL
# ---- head not ready: enter the scan loop, current node in rdx ----
	movq	%rax, %rdx             # rdx = t (head); becomes prev for the step
	.p2align	4
.LBB3_13:                               # LOOP: advance to the next node
	movq	16(%rdx), %rax          # rax = t = prev->rq_next   (step forward)
	testq	%rax, %rax             # ran off the end of the list?
	je	.LBB3_14                 #   yes -> nothing runnable, return NULL
	cmpl	$0, 8(%rax)            # is THIS node ST_READY?
	movq	%rdx, %rcx             #   prev = rdx  (remember predecessor)
	movq	%rax, %rdx             #   rdx = t     (advance the "prev" cursor)
	jne	.LBB3_13                #   not ready -> keep scanning
	jmp	.LBB3_6                 #   ready -> go unlink (rax=t, rcx=prev)
.LBB3_1:
	xorl	%eax, %eax             # empty queue -> NULL (no frame was set up)
	retq
.LBB3_4:                                # match was the head node:
	xorl	%ecx, %ecx             #   prev = NULL
# ---- .LBB3_6: unlink node t (in rax) whose predecessor is prev (in rcx) ----
.LBB3_6:
	movq	16(%rax), %rdx          # rdx = t->rq_next  (what follows the match)
	testq	%rcx, %rcx             # prev == NULL  (is t the head)?
	je	.LBB3_8                 #   yes -> unlink at the head
	movq	%rdx, 16(%rcx)          # prev->rq_next = t->rq_next  (bridge over t)
	cmpq	%rax, 8(%rdi)          # was t the tail (q->tail == t)?
	jne	.LBB3_11                #   no -> done linking
.LBB3_10:
	movq	%rcx, 8(%rdi)          # q->tail = prev   (t was the last node)
.LBB3_11:
	movq	$0, 16(%rax)           # t->rq_next = NULL  (fully detached)
	popq	%rbp
	retq                            # return t (in rax)
.LBB3_14:                               # walked off the end: nothing READY
	xorl	%eax, %eax             # return NULL
	popq	%rbp
	retq
.LBB3_8:                                # t was the head node:
	movq	%rdx, (%rdi)           # q->head = t->rq_next
	cmpq	%rax, 8(%rdi)          # was t also the tail (single-element queue)?
	jne	.LBB3_11                #   no -> done
	jmp	.LBB3_10                #   yes -> also set q->tail = prev(NULL)
.Lfunc_end3:
	.size	pick_next, .Lfunc_end3-pick_next

# =============================================================================
# void *frame_init(u64 top, void *fn, void *arg, void *trampoline)
#     top -> rdi, fn -> rsi, arg -> rdx, trampoline -> rcx, return -> rax
# -----------------------------------------------------------------------------
# Build a brand-new task's fake initial stack frame 56 bytes below `top`, and
# return the resulting rsp. The seven 8-byte slots, low to high, are:
#     [r15=0][r14=0][r13=arg][r12=fn][rbx=0][rbp=0][retaddr=trampoline]
# which is EXACTLY what gt_switch (switch.S) pops on the first switch in. Watch
# clang fold the four zero-slots into two 16-byte SSE stores.
# =============================================================================
	.globl	frame_init
	.p2align	4
	.type	frame_init,@function
frame_init:
	pushq	%rbp
	movq	%rsp, %rbp
	leaq	-56(%rdi), %rax         # rax = top - 56 = &frame[0]  (the new rsp)
	xorps	%xmm0, %xmm0            # xmm0 = 0  (a 16-byte zero to store twice)
	movups	%xmm0, -56(%rdi)        # frame[0..1] = 0,0   -> r15 slot, r14 slot
	movq	%rdx, -40(%rdi)         # frame[2]   = arg    -> r13 slot (top-40)
	movq	%rsi, -32(%rdi)         # frame[3]   = fn     -> r12 slot (top-32)
	movups	%xmm0, -24(%rdi)        # frame[4..5] = 0,0   -> rbx slot, rbp slot
	movq	%rcx, -8(%rdi)          # frame[6]   = trampoline -> return address
	popq	%rbp
	retq                            # return &frame[0]  (store this as task->rsp)
.Lfunc_end4:
	.size	frame_init, .Lfunc_end4-frame_init

# =============================================================================
# int demo_main(void)   — a self-check driver (return -> rax, expect 2)
# -----------------------------------------------------------------------------
# Builds three tasks on the stack (a=WAITING, b=READY id 2, c=READY id 3), pushes
# them, and returns pick_next()->id. At -O1 clang INLINED runq_push and pick_next
# straight into this function (there are no `call`s), initialising the three
# 32-byte task structs from .rodata templates with SSE moves and then running the
# same walk you annotated in pick_next above. The point of keeping it: it gives
# the optimiser something concrete to fold, so demo.O2.s shows the whole thing
# collapse toward the constant 2. See demo.s lines for the full inlined body;
# it repeats the pick_next control flow already explained.
# =============================================================================
# (Body omitted from annotation — it is pick_next + runq_push inlined; the
#  interesting mechanics are documented on those functions above.)

	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
# =============================================================================
# WHAT TO TAKE AWAY
#   * The scheduler's hot path is pure pointer chasing: loads, compares, and a
#     handful of stores. No syscalls, no allocation — that is why a user-space
#     switch is cheap.
#   * pick_next shows the classic singly-linked-list unlink: keep `prev`, bridge
#     prev->next over the removed node, and special-case head and tail.
#   * frame_init is the bridge to switch.S: the seven words it writes are the
#     exact image gt_switch pops. Read them side by side.
#   * Compare with demo.O0.s (every value spilled to the stack, easiest to trace)
#     and demo.O2.s (aggressive inlining/folding of demo_main).
# =============================================================================
