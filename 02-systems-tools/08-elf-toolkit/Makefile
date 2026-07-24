# =============================================================================
# Makefile — build the `elftk` ELF toolkit and (re)generate its teaching asm.
# =============================================================================
# Platform: Linux / WSL (the tool uses mmap(2)/open(2)). The teaching assembly,
# however, is produced by a cross-targeting clang and regenerates on ANY host.
#
#   make            build ./elftk
#   make check      build, then run elftk against its OWN binary (a smoke test)
#   make asm        regenerate asm/demo.{O0,,O2}.s and asm/disasm.{O0,,O2}.s
#   make clean      remove build artifacts (never touches the committed .s)
# =============================================================================

CC      ?= clang
# -Wall -Wextra: warnings are errors in spirit (CONVENTIONS §7).
# -std=c11    : we use _Static_assert in elf.h.
CFLAGS  ?= -std=c11 -Wall -Wextra -O2 -g

# The toolkit is five translation units: the loader/CLI, three "front ends"
# (readelf/nm/objdump-lite), and the self-contained disassembler backend.
SRCS := elftk.c readelf.c nm.c objdump.c disasm.c
OBJS := $(SRCS:.c=.o)
HDRS := elftk.h elf.h disasm.h

elftk: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@
	@echo "built ./elftk — try:  ./elftk -a ./elftk | less"

# Every object depends on the headers (small project; rebuild on any header edit).
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# ---- smoke test: the tool is its own best test input ------------------------
# An ELF64 tool should be able to read itself. This dumps its header, sections,
# symbols, disassembles its .text, and symbolizes its entry point.
.PHONY: check
check: elftk
	@echo "== elftk -h (self) ==" ; ./elftk -h ./elftk
	@echo "== elftk -S (self) ==" ; ./elftk -S ./elftk
	@echo "== elftk nm (self, first 20) ==" ; ./elftk nm ./elftk | head -20
	@echo "== elftk -D (self, first 40 insns) ==" ; ./elftk -D ./elftk | head -40
	@echo "== addr2line of the entry point ==" ; \
	  E=$$(./elftk -h ./elftk | awk '/Entry point/{print $$4}') ; \
	  ./elftk addr2line ./elftk $$E

# ---- teaching assembly ------------------------------------------------------
# The exact commands CONVENTIONS/the project spec require. demo.c is the
# extracted pure-logic core; disasm.c is a real source file that is ALSO self-
# contained (no system headers), so we ship its assembly too.
ASM_TARGET ?= x86_64-pc-linux-gnu
ASM_COMMON := --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

.PHONY: asm
asm:
	$(CC) $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c   -o asm/demo.O0.s
	$(CC) $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c   -o asm/demo.s
	$(CC) $(ASM_COMMON) -O2                          asm/demo.c   -o asm/demo.O2.s
	$(CC) $(ASM_COMMON) -O0 -fno-omit-frame-pointer disasm.c     -o asm/disasm.O0.s
	$(CC) $(ASM_COMMON) -O1 -fno-omit-frame-pointer disasm.c     -o asm/disasm.s
	$(CC) $(ASM_COMMON) -O2                          disasm.c     -o asm/disasm.O2.s
	@echo "regenerated asm/*.s (the hand-written asm/demo.annotated.s is NOT touched)"

.PHONY: clean
clean:
	rm -f elftk $(OBJS)
