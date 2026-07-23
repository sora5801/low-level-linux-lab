# =============================================================================
# Makefile — build the sampling profiler (a USERSPACE Linux program).
# =============================================================================
#
# LINUX ONLY to build and run: the sources use perf_event_open(2), the perf mmap
# ring buffer, <ucontext.h> register access, setitimer(ITIMER_PROF), and dladdr.
# None of that exists on the Windows host this repo may live on. The `asm` target
# is the exception — clang cross-compiles asm/demo.c to Linux assembly on ANY
# host, so the teaching assembly regenerates everywhere.
#
# TWO FLAGS THAT ARE NOT OPTIONAL HERE
# ------------------------------------
#   -fno-omit-frame-pointer : the whole profiler follows the %rbp frame-pointer
#       chain to unwind stacks (both the kernel's callchain unwinder and our own
#       SIGPROF fp_walk). Omit frame pointers and every stack collapses to depth
#       1. This is the single most important build flag in the project.
#   -rdynamic : export the executable's global symbols into .dynsym so dladdr can
#       name our own functions (see symbolize.c). Without it the flame graph is
#       all hex addresses.
# =============================================================================

CC      ?= cc
# -O2   : this is a profiler; the workload and drain loop want to be fast.
# -g    : keep debug info so gdb/addr2line stay useful.
CFLAGS  ?= -Wall -Wextra -O2 -g -fno-omit-frame-pointer
LDFLAGS ?= -rdynamic
LDLIBS  ?= -ldl            # dladdr lives in libdl on older glibc (harmless on new)

OBJS := profiler.o perf_sampler.o sigprof_sampler.o stackmap.o symbolize.o
HDRS := sampler.h stackmap.h symbolize.h

.PHONY: all run run-timer flame asm clean

# Default: build the profiler binary.
all: profiler

# Every object depends on the headers it may include; simplest correct rule is
# to rebuild all objects if any header changes.
$(OBJS): $(HDRS)

profiler: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	@echo "built ./profiler — try:  make run   (perf)   or   make run-timer (SIGPROF)"

# Self-profile the built-in workload via perf_event_open, printing collapsed
# stacks. Needs /proc/sys/kernel/perf_event_paranoid <= 2 (the common default).
run: profiler
	./profiler

# Self-profile via the SIGPROF timer backend. Needs no privileges at all — use
# this if `make run` prints a perf_event_open permission error.
run-timer: profiler
	./profiler -t

# Produce a foldable profile file and remind you how to render it. flamegraph.pl
# is Brendan Gregg's script (see References); it is not vendored here.
flame: profiler
	./profiler > profile.folded
	@echo "wrote profile.folded — render it with:"
	@echo "    flamegraph.pl profile.folded > profile.svg"

# Regenerate the committed teaching assembly from the extracted pure-logic core.
# These are the EXACT commands the project spec mandates; each targets the Linux
# SysV ABI so the output is identical on any host. The hand-written
# asm/demo.annotated.s is authored, never overwritten here.
asm:
	clang --target=x86_64-pc-linux-gnu -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=x86_64-pc-linux-gnu -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=x86_64-pc-linux-gnu -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s} (asm/demo.annotated.s is hand-written)"

clean:
	rm -f profiler $(OBJS) profile.folded profile.svg
