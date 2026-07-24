# =============================================================================
# Makefile — minild, a minimal static ELF linker (teaching core).
# =============================================================================
#
# Two toolchains are in play, on purpose:
#
#   1. The LINKER ITSELF (minild) is a portable HOST program — pure byte
#      manipulation, no Linux headers — so it builds and runs anywhere with
#      $(CC). On Windows it produces minild.exe; on Linux, minild.
#
#   2. The EXAMPLE OBJECTS and the linked output are Linux x86-64 ELF. We build
#      them with clang's Linux cross-target so `make examples`/`make link` work
#      on any host; only RUNNING the linked a.out needs Linux (or WSL).
#
# Quick start (Linux/WSL):   make link && ./demo.elf ; echo "exit=$?"
# On any host:               make link   (produces + inspects demo.elf)
#                            make asm    (regenerates the teaching assembly)
# =============================================================================

CC        ?= clang

# Host build of the linker: warnings on, C11, optimized. No target flags — this
# is a native tool.
HOSTFLAGS ?= -Wall -Wextra -std=c11 -O2

# Cross-target for the Linux ELF example objects. -fno-pic keeps the compiler
# emitting the simple absolute/PC-relative relocations minild implements
# (R_X86_64_{64,PC32,PLT32,32,32S}); -fno-common puts tentative definitions in
# .bss (minild refuses COMMON on purpose); -ffreestanding/-fno-stack-protector
# keep the objects nolibc so the static link needs nothing but themselves.
XTARGET   ?= x86_64-pc-linux-gnu
XCFLAGS   ?= --target=$(XTARGET) -ffreestanding -fno-pic -fno-stack-protector \
             -fno-common -c

# Assembly generation target (matches CONVENTIONS + the reference project).
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all
all: minild

# ---- the linker (host tool) -------------------------------------------------
minild: minild.c elf.h
	$(CC) $(HOSTFLAGS) minild.c -o $@
	@echo "built ./$@ — usage: ./$@ -o out start.o msg.o"

# ---- the two example objects (Linux ELF, cross-compiled) --------------------
.PHONY: examples
examples: start.o msg.o
start.o: examples/start.c
	$(CC) $(XCFLAGS) examples/start.c -o start.o
msg.o: examples/msg.c
	$(CC) $(XCFLAGS) examples/msg.c -o msg.o

# ---- link the demo: start.o + msg.o -> demo.elf (a static ET_EXEC) ----------
# start.o goes first so _start (the entry) lands at the front of .text.
demo.elf: minild examples
	./minild -o demo.elf start.o msg.o
	-@chmod +x demo.elf 2>/dev/null || true

.PHONY: link
link: demo.elf

# ---- run it (Linux/WSL only) ------------------------------------------------
.PHONY: run
run: demo.elf
	./demo.elf ; echo "exit=$$?"     # expect: Hello... printed twice, exit 0

# ---- inspect the output with binutils (works on any host with readelf) ------
.PHONY: check
check: demo.elf
	@echo "=== ELF header ==="        ; readelf -h demo.elf
	@echo "=== program headers ==="   ; readelf -l demo.elf
	@echo "=== disassembly (.text) ===" ; objdump -d demo.elf

# ---- regenerate the committed teaching assembly for asm/demo.c --------------
# These are the exact commands CONVENTIONS specifies; demo.annotated.s is
# authored by hand from demo.s and is NOT overwritten here.
.PHONY: asm
asm:
	$(CC) --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s} — asm/demo.annotated.s is authored"

.PHONY: clean
clean:
	rm -f minild minild.exe start.o msg.o demo.elf
