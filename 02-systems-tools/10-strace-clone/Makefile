# =============================================================================
# Makefile — build the two strace clones and (re)generate the teaching assembly.
# =============================================================================
#
# `make`        builds ./ministrace and ./seccomp-strace (LINUX ONLY — they use
#               ptrace(2), seccomp(2), <sys/user.h>; they do not build on Windows
#               or macOS).
# `make run`    traces /bin/echo with the ptrace tracer.
# `make test`   runs both tracers on a small command and greps for a known line.
# `make asm`    regenerates asm/*.s from the SELF-CONTAINED units (demo.c and
#               syscall_table.c). This works on ANY host because clang cross-
#               targets Linux; the annotated asm/demo.annotated.s is authored by
#               hand and is never overwritten.

CC      ?= clang
# -Wall -Wextra: warnings are errors in spirit (CONVENTIONS.md rule 7).
# -O2:           realistic codegen; the tracer is I/O-bound on ptrace anyway.
# -g:            keep symbols so you can debug the tracer with the tracer.
CFLAGS  ?= -Wall -Wextra -O2 -g -std=c11
# _GNU_SOURCE exposes PTRACE_GET_SYSCALL_INFO / struct ptrace_syscall_info and
# the seccomp constants from glibc's headers.
CPPFLAGS ?= -D_GNU_SOURCE

# For asm generation we force the Linux System V target so the output is the ABI
# every project in this repo assumes, regardless of the host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

# Shared objects: the syscall table and the argument decoder.
COMMON_OBJS := syscall_table.o decode.o

BINS := ministrace seccomp-strace

.PHONY: all
all: $(BINS)

# The PTRACE_SYSCALL tracer.
ministrace: ministrace.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "built ./ministrace  — try:  ./ministrace /bin/echo hello"

# The seccomp SECCOMP_RET_TRACE tracer.
seccomp-strace: seccomp_trace.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "built ./seccomp-strace — try:  ./seccomp-strace /bin/echo hello"

# Generic compile rule with header dependencies.
%.o: %.c syscall_table.h decode.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY: run
run: ministrace
	./ministrace /bin/echo hello

.PHONY: test
test: $(BINS)
	@echo "== ptrace tracer =="
	./ministrace /bin/echo hello 2>&1 | grep -E 'write\(1, "hello' \
	  && echo "PASS: ministrace decoded the write()" || echo "FAIL"
	@echo "== seccomp tracer =="
	./seccomp-strace /bin/echo hello 2>&1 | grep -E 'write\(1, "hello' \
	  && echo "PASS: seccomp-strace decoded the write()" || echo "FAIL"

# Regenerate the committed teaching assembly for BOTH self-contained units, at
# the three optimization levels, with the exact flags CONVENTIONS.md prescribes.
.PHONY: asm
asm:
	@mkdir -p asm
	for u in asm/demo syscall_table; do \
	  src=$$u.c; base=$$(basename $$u); \
	  $(CC) --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
	    -fno-jump-tables -fno-omit-frame-pointer $$src -o asm/$$base.O0.s; \
	  $(CC) --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
	    -fno-jump-tables -fno-omit-frame-pointer $$src -o asm/$$base.s; \
	  $(CC) --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
	    -fno-jump-tables $$src -o asm/$$base.O2.s; \
	done
	@echo "regenerated asm/demo.{O0,,O2}.s and asm/syscall_table.{O0,,O2}.s"
	@echo "NOTE: asm/demo.annotated.s is hand-authored and NOT regenerated."

.PHONY: clean
clean:
	rm -f $(BINS) *.o
