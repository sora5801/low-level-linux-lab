# =============================================================================
# Makefile — build the XDP data plane (.bpf.o), its skeleton, and the loader.
# =============================================================================
#
# WHY THIS IS NOT A Kbuild `obj-m` Makefile
# -----------------------------------------
# The other projects in 01-kernel/ are loadable kernel MODULES (.ko), so they
# use a Kbuild Makefile and `make -C /lib/modules/.../build M=$(PWD) modules`.
# An eBPF program is NOT a .ko: it is bytecode that clang emits and that libbpf
# streams into the kernel through the bpf(2) syscall, where the verifier checks
# it. So there is nothing for Kbuild to do. Instead we have three build steps:
#
#   1. clang -target bpf   ...  xdp_filter.bpf.c  -> xdp_filter.bpf.o   (bytecode)
#   2. bpftool gen skeleton     xdp_filter.bpf.o  -> xdp_filter.skel.h  (C API)
#   3. clang (host)             xdp_loader.c      -> xdp_loader         (loader)
#
# REQUIREMENTS (Linux; do the actual run in a VM — see README):
#   clang/llvm >= 12, libbpf-dev, bpftool, and kernel headers.
#   Debian/Ubuntu:  sudo apt install clang llvm libbpf-dev linux-tools-common \
#                                    linux-tools-$(uname -r) libelf-dev zlib1g-dev
# =============================================================================

# ---- Toolchain --------------------------------------------------------------
CLANG   ?= clang
BPFTOOL ?= bpftool

# The BPF target's own arch define. libbpf's headers switch on __TARGET_ARCH_*
# to select the right register layout for things like PT_REGS_* macros. We are
# on x86-64; change to arm64/etc. if you cross-build. Detected from `uname -m`.
ARCH := $(shell uname -m | sed 's/x86_64/x86/;s/aarch64/arm64/')

# ---- BPF-side compile flags -------------------------------------------------
#   -target bpf   : emit BPF bytecode, not host machine code.
#   -g            : REQUIRED — it makes clang emit BTF, without which libbpf can
#                   neither generate a skeleton nor do CO-RE relocations.
#   -O2           : the BPF verifier effectively requires optimized code; -O0
#                   BPF often fails to verify (dead code, spills it dislikes).
#   -Wall -Wextra : warnings are errors in spirit (CONVENTIONS.md).
BPF_CFLAGS ?= -target bpf -g -O2 -Wall -Wextra -D__TARGET_ARCH_$(ARCH)

# ---- Host-side (loader) flags -----------------------------------------------
# libbpf is the runtime; libelf and zlib are its dependencies (it parses ELF/BTF
# and may inflate compressed BTF). If libbpf-dev is in a nonstandard prefix, add
# -I/path/include and -L/path/lib here.
HOST_CFLAGS ?= -Wall -Wextra -O2
LDLIBS      ?= -lbpf -lelf -lz

# For regenerating the teaching assembly (host-independent; clang cross-targets).
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all
all: xdp_loader

# ---- Step 1: BPF C -> BPF bytecode object -----------------------------------
xdp_filter.bpf.o: xdp_filter.bpf.c xdp_filter.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@
	@echo "built $@ (BPF bytecode + BTF)"

# ---- Step 2: bytecode -> typed C skeleton header ----------------------------
# The skeleton embeds the .bpf.o as a byte array and generates open/load/attach
# plus typed handles for every map and program. This is what makes the loader a
# few dozen lines instead of hundreds of raw bpf(2) calls.
xdp_filter.skel.h: xdp_filter.bpf.o
	$(BPFTOOL) gen skeleton $< > $@
	@echo "generated $@"

# ---- Step 3: host loader (depends on the generated skeleton) ----------------
xdp_loader: xdp_loader.c xdp_filter.skel.h xdp_filter.h
	$(CLANG) $(HOST_CFLAGS) $< -o $@ $(LDLIBS)
	@echo "built ./$@ — run (as root, in a VM):  sudo ./xdp_loader <ifname>"

# ---- Teaching assembly for the extracted flow-hash core ---------------------
# These are the EXACT commands the lab uses; the .s files are committed so you
# can read them without a toolchain. `make asm` regenerates them on any host.
.PHONY: asm
asm:
	$(CLANG) --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
		-fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CLANG) --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
		-fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CLANG) --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
		-fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-authored"

# ---- Bonus: emit the BPF bytecode assembly for the same core ----------------
# Because asm/demo.c has no kernel deps, we can ALSO compile it to BPF assembly
# and compare instruction sets (BPF has no imul-by-immediate the same way; it is
# a load/store RISC). Purely for study.
.PHONY: bpf-asm
bpf-asm:
	$(CLANG) -target bpf -S -O2 asm/demo.c -o asm/demo.bpf.s
	@echo "wrote asm/demo.bpf.s (BPF ISA, for comparison)"

.PHONY: clean
clean:
	rm -f xdp_filter.bpf.o xdp_filter.skel.h xdp_loader asm/demo.bpf.s
