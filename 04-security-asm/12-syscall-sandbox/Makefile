# =============================================================================
# Makefile — the three-layer syscall sandbox.
# =============================================================================
#
# Two kinds of target:
#   * The BINARIES (`sandbox`, `target`) are Linux-only: they call seccomp(2),
#     ptrace(2), and the landlock_* syscalls, which exist only on Linux (5.13+
#     for Landlock). Build and run them on Linux or WSL2.
#   * The teaching ASSEMBLY (`make asm`) is host-portable: clang cross-targets
#     the Linux SysV ABI, so `asm/demo.{s,O0.s,O2.s}` regenerate on any OS.
#
# `make` builds both binaries. `make run` runs the full demo. `make asm`
# regenerates the committed assembly. See README.md for what each layer does.

CC      ?= clang

# The launcher is an ordinary glibc program; warnings-as-conscience.
SB_CFLAGS  ?= -Wall -Wextra -O2 -g
SB_SRCS     = sandbox.c seccomp_filter.c landlock_fs.c supervisor.c

# The confined target is a NO-LIBC static binary (like ../01-nolibc-programs),
# so its syscall surface is exactly the syscalls written in target.c — which is
# what makes the allowlist small and legible.
#   -nostdlib -static -no-pie : no CRT, no loader, fixed address (we supply _start)
#   -fno-stack-protector      : the canary would call into libc we did not link
TGT_CFLAGS ?= -Wall -Wextra -Os -nostdlib -static -no-pie -fno-stack-protector -ffreestanding

# For asm generation we FORCE the Linux SysV target so the output matches every
# other project in the repo, regardless of the host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_FLAGS  ?= --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

.PHONY: all
all: sandbox target

# The launcher: installs no_new_privs + Landlock + seccomp, then execs the child.
sandbox: $(SB_SRCS) sandbox.h
	$(CC) $(SB_CFLAGS) $(SB_SRCS) -o $@
	@echo "built ./sandbox"

# The confined victim/escape-artist.
target: target.c
	$(CC) $(TGT_CFLAGS) target.c -o $@
	@echo "built ./target (static, no-libc)"

# -----------------------------------------------------------------------------
# Demos (Linux/WSL only). Each prints the layer decisions to stderr.
# -----------------------------------------------------------------------------

# Full stack: ptrace supervisor ON (so openat paths are inspected), and Landlock
# grants read/exec beneath /etc. Expected finale: socket() is not on the
# allowlist, so the kernel kills ./target with SIGSYS. That is the allowlist
# default-deny working.
.PHONY: run
run: all
	@echo "=== seccomp + Landlock(/etc) + ptrace supervisor ==="
	-./sandbox --trace --allow /etc -- ./target
	@echo "(exit status above: 128+31=159 means SIGSYS, i.e. socket() was blocked)"

# seccomp + Landlock only (no tracer). openat degrades from TRACE to ALLOW at the
# seccomp layer and Landlock alone enforces the filesystem. Contrast the openat
# results with the --trace run.
.PHONY: run-notrace
run-notrace: all
	@echo "=== seccomp + Landlock(/etc), NO ptrace supervisor ==="
	-./sandbox --allow /etc -- ./target

# Tightest: deny ALL filesystem access (no --allow) and no tracer. Every openat
# should fail with EACCES from Landlock; socket() still ends it with SIGSYS.
.PHONY: run-strict
run-strict: all
	@echo "=== seccomp + Landlock(deny-all filesystem) ==="
	-./sandbox -- ./target

# A convenience alias used by the repo's test convention.
.PHONY: test
test: run

# -----------------------------------------------------------------------------
# Teaching assembly. Regenerate the committed asm/demo.{s,O0.s,O2.s} from
# asm/demo.c with the EXACT flags the repo standard uses. The hand-written
# asm/demo.annotated.s is authored, never overwritten here.
# -----------------------------------------------------------------------------
.PHONY: asm
asm:
	$(CC) $(ASM_FLAGS) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) $(ASM_FLAGS) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) $(ASM_FLAGS) -O2                         asm/demo.c -o asm/demo.O2.s
	@echo "wrote asm/demo.O0.s (naive), asm/demo.s (-O1 baseline), asm/demo.O2.s (-O2)"
	@echo "asm/demo.annotated.s is hand-written from asm/demo.s — not regenerated."

# Optional: show the encoded bytes of the demo (proves the .s assembles).
.PHONY: asm-bytes
asm-bytes:
	$(CC) --target=$(ASM_TARGET) -c -O1 asm/demo.c -o asm/demo.o
	objdump -d asm/demo.o | sed -n '1,80p'

.PHONY: clean
clean:
	rm -f sandbox target asm/demo.o
