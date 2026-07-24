# =============================================================================
# Makefile — build the four DELIBERATELY-VULNERABLE targets, each with the exact
#            mitigation flags that make its rung of the ladder exploitable, plus
#            an `asm` target for the teaching assembly and a `harden` target that
#            rebuilds Rung 1 with mitigations ON so you can watch the exploit FAIL.
#
#   >>> These binaries are unsafe by design. Build and run them ONLY on your own
#   >>> machine, for learning. See README.md. Building/running needs Linux (WSL
#   >>> is fine); the `asm` target works on any OS (clang cross-compiles).
#
# The offsets in the exploit scripts were measured against clang -O0 (see
# asm/demo.annotated.s). Keeping CC=clang here matches those measurements; if you
# switch compilers, re-measure with `python3 exploits/common.py cyclic ...`.
# =============================================================================

CC      ?= clang
# -g  : keep symbols/line info so gdb can show you the frame.
# -O0 : predictable, source-faithful stack layout (no inlining/reordering).
COMMON  := -g -O0 -Wall -Wextra -Wno-unused-parameter

# For asm generation we force the Linux SysV target so output matches the ABI
# every project in this repo assumes, regardless of the host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

TARGETS := targets/vuln_stack targets/vuln_ret2libc targets/vuln_rop targets/vuln_fmt

.PHONY: all
all: $(TARGETS)
	@echo
	@echo "Built the ladder. Disable ASLR for the lab, then run an exploit:"
	@echo "    make aslr-off"
	@echo "    python3 exploits/exploit_rung1_stack.py"
	@echo "    python3 exploits/exploit_rung2_ret2libc.py"
	@echo "    python3 exploits/exploit_rung3_rop.py"
	@echo "    python3 exploits/exploit_rung4_fmt.py"

# ---- Rung 1: classic stack smash into an EXECUTABLE stack --------------------
# -fno-stack-protector : no canary guarding the return address (canary = defense)
# -z execstack         : make the stack executable so injected shellcode runs
#                        (NX/DEP = defense; this flag removes it)
# -no-pie -fno-pie     : fixed load address (ASLR/PIE = defense)
targets/vuln_stack: targets/vuln_stack.c
	$(CC) $(COMMON) -fno-stack-protector -z execstack -no-pie -fno-pie $< -o $@

# ---- Rung 2: ret2libc (NX stays ON — no execstack) --------------------------
targets/vuln_ret2libc: targets/vuln_ret2libc.c
	$(CC) $(COMMON) -fno-stack-protector -no-pie -fno-pie $< -o $@

# ---- Rung 3: ROP (NX ON; no-PIE so planted gadgets are at fixed addresses) ---
targets/vuln_rop: targets/vuln_rop.c
	$(CC) $(COMMON) -fno-stack-protector -no-pie -fno-pie $< -o $@

# ---- Rung 4: format-string %n (mitigations deliberately DISABLED) -----------
# -Wno-format-security : silence the warning that IS the primary defense
# -U_FORTIFY_SOURCE    : drop glibc's runtime %n-in-writable-format guard
targets/vuln_fmt: targets/vuln_fmt.c
	$(CC) $(COMMON) -no-pie -fno-pie -Wno-format-security -U_FORTIFY_SOURCE $< -o $@

# =============================================================================
# DEFENSE DEMO: rebuild Rung 1 with every mitigation ON. The Rung-1 exploit now
# fails — the canary aborts the overflow, NX faults the shellcode, PIE+ASLR move
# the buffer. This is the blue-team half made concrete.
# =============================================================================
.PHONY: harden
harden: targets/vuln_stack.c
	$(CC) $(COMMON) -fstack-protector-strong -z noexecstack -pie -fPIE \
	      -D_FORTIFY_SOURCE=2 -Wl,-z,relro,-z,now targets/vuln_stack.c \
	      -o targets/vuln_stack_hardened
	@echo "Built targets/vuln_stack_hardened (canary+NX+PIE+RELRO)."
	@echo "Run the Rung-1 exploit against it and watch it FAIL:"
	@echo "    python3 exploits/exploit_rung1_stack.py targets/vuln_stack_hardened"

# =============================================================================
# ASLR toggles (need sudo; they write /proc/sys/kernel/randomize_va_space).
#   0 = off (lab), 1 = partial, 2 = full (default on most distros).
# =============================================================================
.PHONY: aslr-off aslr-on
aslr-off:
	echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
aslr-on:
	echo 2 | sudo tee /proc/sys/kernel/randomize_va_space

# =============================================================================
# Teaching assembly for the vulnerable function (asm/demo.c). Regenerates the
# three compiler outputs; the hand-written asm/demo.annotated.s is NOT touched.
# =============================================================================
.PHONY: asm
asm:
	$(CC) --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables \
	      -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "Regenerated asm/demo.{O0.s,s,O2.s}. Hand-written demo.annotated.s kept."

# =============================================================================
# run  : the most self-contained demo — build Rung 3 and pop a shell via ROP.
# test : build everything, then non-interactively smoke-test the Rung-3 chain
#        (looks for the RUNG3-PWNED marker). Linux only.
# =============================================================================
.PHONY: run
run: targets/vuln_rop
	python3 exploits/exploit_rung3_rop.py

.PHONY: test
test: all
	@echo "[test] smoke-testing the Rung-3 ROP chain..."
	@printf 'id\nexit\n' | timeout 15 python3 exploits/exploit_rung3_rop.py \
	    | tee /tmp/rung3.out; \
	 grep -q RUNG3-PWNED /tmp/rung3.out \
	    && echo "[test] PASS: got a shell via ROP" \
	    || { echo "[test] FAIL: no shell (ASLR on? gadgets missing?)"; exit 1; }

.PHONY: clean
clean:
	rm -f $(TARGETS) targets/vuln_stack_hardened /tmp/rung3.out
	rm -f exploits/*.pyc
	rm -rf exploits/__pycache__
