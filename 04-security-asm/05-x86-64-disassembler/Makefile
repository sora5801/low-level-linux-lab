# =============================================================================
# Makefile — x86-64 disassembler (teaching core).
# =============================================================================
# This project is a DECODER, not a deliberately-vulnerable target, so it toggles
# no mitigations: it builds as an ordinary, warnings-clean userspace tool. The
# interesting flags live in the `asm` target, which cross-targets Linux/SysV so
# the committed teaching assembly is meaningful on any host (see gen_asm.sh).
#
#   make            build ./disasm
#   make run        decode a small demo instruction stream
#   make test       self-check a handful of encodings (no external tools)
#   make oracle     diff our AT&T output against `objdump -d` (needs binutils)
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} (NOT demo.annotated.s)
#   make clean
# =============================================================================

CC      ?= clang
# -std=c11 for designated initializers; warnings-as-conscience per repo policy.
CFLAGS  ?= -Wall -Wextra -O2 -std=c11

# Cross-target the Linux System V ABI for the teaching assembly, exactly as the
# repo's gen_asm.sh does, so demo.s is the ABI every project here assumes.
ASM_TARGET ?= x86_64-pc-linux-gnu
ASMFLAGS    = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

OBJHDR = disasm.h

disasm: disasm.c main.c $(OBJHDR)
	$(CC) $(CFLAGS) disasm.c main.c -o $@
	@echo "built ./disasm — try:  make run   or   ./disasm 48 89 e5"

# ---- demo run ---------------------------------------------------------------
.PHONY: run
run: disasm
	@echo "# prologue + syscall + ret, disassembled (AT&T):"
	./disasm --base 0x401000 55 48 89 e5 48 83 ec 10 0f 05 c9 c3

# ---- self-contained regression check (no objdump needed) --------------------
# Decodes a known stream and asserts a few expected tokens appear. If any is
# missing the recipe fails (grep -q returns non-zero), so `make test` is a gate.
.PHONY: test
test: disasm
	@echo ">> decoding a mixed sample and checking key encodings..."
	@./disasm --base 0x401000 \
	    48 89 e5  48 8b 45 f8  48 8d 05 00 00 00 00 \
	    48 8b 04 25 78 56 34 12  ff d0  0f 84 00 00 00 00 \
	    48 b8 88 77 66 55 44 33 22 11 > .test.out
	@grep -q 'mov	%rsp,%rbp'                 .test.out || { echo "FAIL: mov rsp,rbp"; exit 1; }
	@grep -q 'mov	-0x8(%rbp),%rax'           .test.out || { echo "FAIL: disp8";       exit 1; }
	@grep -q 'lea	0x0(%rip),%rax'            .test.out || { echo "FAIL: rip-rel";     exit 1; }
	@grep -q 'mov	0x12345678,%rax'           .test.out || { echo "FAIL: abs sib";     exit 1; }
	@grep -q 'call	\*%rax'                    .test.out || { echo "FAIL: indirect";    exit 1; }
	@grep -q 'movabs	\$0x1122334455667788,%rax' .test.out || { echo "FAIL: movabs";   exit 1; }
	@rm -f .test.out
	@echo "PASS"

# ---- validate against the objdump oracle (needs binutils + python3 or xxd) --
# Builds a raw blob from a hex string, disassembles it both ways, and diffs the
# instruction text. This is the workflow the README recommends for extending the
# decoder: whenever you add an opcode, make it agree with objdump.
.PHONY: oracle
oracle: disasm
	@HEX="48 89 e5 48 8b 45 f8 48 8d 05 00 00 00 00 48 8b 04 25 78 56 34 12 ff d0 0f b6 c0 48 63 c1 c7 00 05 00 00 00"; \
	printf '%s' "$$HEX" | tr -d ' \n' | xxd -r -p > .oracle.bin; \
	echo "== objdump (oracle) =="; \
	objdump -D -b binary -m i386:x86-64 -M att --adjust-vma=0x401000 .oracle.bin \
	    | sed -n '/<.data>:/,$$p' | tail -n +2; \
	echo "== ours =="; \
	./disasm --base 0x401000 -f .oracle.bin; \
	rm -f .oracle.bin

# ---- regenerate the committed teaching assembly -----------------------------
# The hand-written asm/demo.annotated.s is authored, never overwritten here.
.PHONY: asm
asm:
	$(CC) $(ASMFLAGS) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) $(ASMFLAGS) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) $(ASMFLAGS) -O2 asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.O0.s, asm/demo.s, asm/demo.O2.s"

.PHONY: clean
clean:
	rm -f disasm disasm.exe *.o asm/*.o .test.out .oracle.bin
