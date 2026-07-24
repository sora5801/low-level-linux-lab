# Makefile for the shellcode: mechanics & defense project.
#
# The DEFAULT target builds only the defensive analyzer (nullscan). The syscall
# walkthrough is assembled to an object for `objdump -d` inspection on demand —
# it is teaching material, deliberately not linked into a runnable payload.

CC      ?= cc
CFLAGS  ?= -Wall -Wextra -O2
# Harden the defensive tool itself — it is the kind of parser that should model
# good hygiene: stack protector, fortified libc calls, PIE, full RELRO.
HARDEN  ?= -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE -pie -Wl,-z,relro,-z,now

nullscan: nullscan.c
	$(CC) $(CFLAGS) $(HARDEN) $< -o $@
	@echo "built ./nullscan — the defensive bad-char/null-byte analyzer"

.PHONY: test
test: nullscan
	@echo "# a clean ASCII buffer (should be NUL-free, exit 0):"
	@printf 'GET / HTTP/1.1' | ./nullscan || true
	@echo "# a buffer containing a NUL at offset 3 (should flag it, exit 1):"
	@printf 'abc\x00def' | ./nullscan || true

# Assemble the annotated walkthrough to an object purely so you can read the
# real encoded bytes with `objdump -d walkthrough.o`. It is NOT linked or run.
.PHONY: walkthrough
walkthrough: execve_syscall.S
	$(CC) -c execve_syscall.S -o walkthrough.o
	@echo "assembled walkthrough.o — inspect with:  objdump -d walkthrough.o"

# Regenerate the committed teaching assembly of the defensive scanner core.
.PHONY: asm
asm:
	../../tools/gen_asm.sh asm/demo.c

.PHONY: clean
clean:
	rm -f nullscan walkthrough.o
