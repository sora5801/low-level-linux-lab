# =============================================================================
# Makefile — build the scx_fifo sched_ext scheduler (BPF kernel half + the
# userspace loader) and regenerate the teaching assembly.
# =============================================================================
#
# WHY THIS IS NOT A Kbuild `obj-m` MODULE
# ---------------------------------------
# Most projects in 01-kernel/ are loadable .ko modules built with Kbuild
# (obj-m + `make -C /lib/modules/.../build`). A sched_ext scheduler is NOT a
# kernel module: it is a BPF program that the ALREADY-RUNNING kernel loads and
# verifies via libbpf, plus a userspace process that attaches it. So the build
# is the standard BPF toolchain instead:
#   1. clang -target bpf   compiles scx_fifo.bpf.c to BPF bytecode (an ELF .o),
#   2. bpftool gen skeleton turns that .o into a C header the loader #includes,
#   3. cc                   compiles the userspace loader and links libbpf.
# There is no .ko and no `insmod`; you run ./scx_fifo and it becomes the live
# scheduler until you Ctrl-C it. This must run on a Linux box (or QEMU VM) whose
# kernel is >= 6.12 with CONFIG_SCHED_CLASS_EXT=y. See the README.
#
# DEPENDENCIES (install on the Linux host / VM)
#   clang + llvm (BPF backend), bpftool, libbpf-dev, libelf-dev, zlib
#   the scx helper headers  <scx/common.bpf.h> / <scx/common.h>  — from the
#   kernel tree tools/sched_ext/include/, or the sched-ext/scx repo. Point
#   SCX_INCLUDE at wherever they live.
# =============================================================================

# ---- toolchain ----
CLANG   ?= clang
BPFTOOL ?= bpftool
CC      ?= cc

# ---- where to find the scx helper headers and (optionally) a local libbpf ----
# Adjust to your checkout. Defaults assume the Linux source tree layout.
KERNEL_SRC  ?= /usr/src/linux
SCX_INCLUDE ?= $(KERNEL_SRC)/tools/sched_ext/include
ARCH        ?= x86

# ---- BPF object: scx_fifo.bpf.c -> scx_fifo.bpf.o ----
# -g            keep BTF (required: the verifier and struct_ops need it)
# -O2           the BPF verifier expects optimized bytecode; -O0 often fails
# -target bpf   emit BPF, not host code
# -D__TARGET_ARCH_x86  selects the arch-specific bits of vmlinux.h
BPF_CFLAGS ?= -g -O2 -Wall -target bpf -D__TARGET_ARCH_$(ARCH) \
              -I. -I$(SCX_INCLUDE)

# ---- userspace loader: scx_fifo.c -> ./scx_fifo ----
USER_CFLAGS ?= -g -O2 -Wall -Wextra -I. -I$(SCX_INCLUDE)
USER_LDLIBS ?= -lbpf -lelf -lz

# The System V AMD64 target for the portable teaching-asm generation.
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all
all: scx_fifo

# vmlinux.h = the CO-RE description of every kernel type, dumped from the
# running kernel's BTF. scx_fifo.bpf.c needs `struct task_struct` etc. from it.
vmlinux.h:
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > $@

# Compile the scheduler to BPF bytecode.
scx_fifo.bpf.o: scx_fifo.bpf.c vmlinux.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

# Generate the libbpf skeleton the loader #includes. `name scx_fifo` fixes the
# generated struct/type names the loader refers to.
scx_fifo.bpf.skel.h: scx_fifo.bpf.o
	$(BPFTOOL) gen skeleton $< name scx_fifo > $@

# Build and link the userspace loader.
scx_fifo: scx_fifo.c scx_fifo.bpf.skel.h
	$(CC) $(USER_CFLAGS) $< -o $@ $(USER_LDLIBS)
	@echo "built ./scx_fifo — run as root on a 6.12+ kernel:"
	@echo "    sudo ./scx_fifo        # weighted-vtime fair-share (default)"
	@echo "    sudo ./scx_fifo -f     # strict global FIFO"

# ---------------------------------------------------------------------------
# Teaching assembly. asm/demo.c is the self-contained vtime/weight arithmetic
# extracted from the BPF scheduler (which cannot be compiled to host asm). These
# are the EXACT commands the lab uses; they cross-target Linux and run on ANY
# host (no kernel needed), which is why the .s files can be committed.
# The hand-written asm/demo.annotated.s is authored, NOT regenerated here.
# ---------------------------------------------------------------------------
ASM_FLAGS := -S -fno-asynchronous-unwind-tables -fno-jump-tables

.PHONY: asm
asm:
	$(CLANG) --target=$(ASM_TARGET) $(ASM_FLAGS) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CLANG) --target=$(ASM_TARGET) $(ASM_FLAGS) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CLANG) --target=$(ASM_TARGET) $(ASM_FLAGS) -O2 asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s} — asm/demo.annotated.s is hand-written"

.PHONY: clean
clean:
	rm -f scx_fifo scx_fifo.bpf.o scx_fifo.bpf.skel.h vmlinux.h
	# note: the committed asm/*.s teaching files are left in place on purpose.
