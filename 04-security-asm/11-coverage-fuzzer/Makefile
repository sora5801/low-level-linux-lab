# =============================================================================
# Makefile — coverage-guided fuzzer (mini-AFL), a teaching core.
# =============================================================================
#
# LINUX ONLY to build & run (fork, SysV shm, waitpid, poll). On another host you
# can still regenerate the teaching assembly with `make asm` — clang cross-
# targets Linux and asm/demo.c is header-free, so it compiles anywhere.
#
# Three target BUILDS of the same vulnerable parser, so you can watch each
# mitigation change the outcome the fuzzer sees:
#   parser           deliberately WEAK: -fno-stack-protector -no-pie  -> raw SIGSEGV
#   parser_hardened  stack canary + FORTIFY                           -> clean SIGABRT
#   parser_asan      AddressSanitizer                                 -> precise report
# All three carry -fsanitize-coverage=trace-pc-guard so the fuzzer can steer.
#
# >>> On-your-own-box note: `parser` is intentionally exploitable. Build and fuzz
# >>> it only on machines you own. See README for the full authorization note.
# =============================================================================

CC     ?= clang
WARN    = -Wall -Wextra

# The compiler edge-coverage instrumentation. clang inserts a call to our
# __sanitizer_cov_trace_pc_guard() (in rt.c) at every edge and emits the guard
# array + init callback. This is the "compile-time instrumentation" half.
COV     = -fsanitize-coverage=trace-pc-guard

# Mitigation toggles — the whole point of having three builds:
#  -fno-stack-protector : remove the canary so the overflow faults directly.
#  -no-pie              : fixed load address (simpler crash addresses to read).
VULN_FLAGS   = -O1 -g -fno-stack-protector -no-pie
#  -fstack-protector-strong : plant a random canary; the overflow trips it and
#                             the process aborts cleanly instead of being hijacked.
#  -D_FORTIFY_SOURCE=2      : compile-time/-runtime checks on known-size buffers.
HARDEN_FLAGS = -O2 -g -fstack-protector-strong -D_FORTIFY_SOURCE=2
#  -fsanitize=address : redzone + shadow memory; catches the FIRST OOB byte with
#                       a symbolized stack-buffer-overflow report — ideal triage.
ASAN_FLAGS   = -O1 -g -fsanitize=address

# Assembly generation targets the Linux SysV ABI regardless of host OS.
ASM_TARGET  = x86_64-pc-linux-gnu
ASM_COMMON  = --target=$(ASM_TARGET) -S -fno-asynchronous-unwind-tables -fno-jump-tables

.PHONY: all
all: fuzzer parser

# ---- the fuzzer driver ------------------------------------------------------
fuzzer: fuzzer.c forkserver.h
	$(CC) $(WARN) -O2 fuzzer.c -o $@

# ---- the instrumentation runtime --------------------------------------------
# CRITICAL: rt.o is built WITHOUT $(COV). If the runtime instrumented itself,
# the fork-server loop's own edges would scribble into the coverage map and every
# input would look "new". Read rt.c's header for why. One plain rt.o links into
# every target build below.
rt.o: rt.c forkserver.h
	$(CC) $(WARN) -O2 -c rt.c -o $@

# ---- the deliberately-vulnerable target (default) ---------------------------
parser: parser.c rt.o forkserver.h
	$(CC) $(WARN) $(COV) $(VULN_FLAGS) parser.c rt.o -o $@

# ---- the hardened target: canary catches the overflow (SIGABRT) -------------
parser_hardened: parser.c rt.o forkserver.h
	$(CC) $(WARN) $(COV) $(HARDEN_FLAGS) parser.c rt.o -o $@

# ---- the ASan target: precise memory-error report ---------------------------
# rt.o stays plain (uninstrumented, no ASan): a non-sanitized object links fine
# into an ASan binary, just like libc does.
parser_asan: parser.c rt.o forkserver.h
	$(CC) $(WARN) $(COV) $(ASAN_FLAGS) parser.c rt.o -o $@

# =============================================================================
# Run / test
# =============================================================================
# Point the fuzzer at the weak target and let it climb the FZR1 magic ladder.
# It prints a stats line and announces the first crash (usually < a few seconds).
.PHONY: run
run: fuzzer parser
	./fuzzer ./parser seeds out dict.txt

# Fuzz the ASan build. abort_on_error=1 makes ASan raise SIGABRT (which our fork
# server reports as a crash) instead of exit(1); detect_leaks=0 keeps the child
# fast. Read out/crashes/<id> then reproduce with `./parser_asan out/crashes/<id>`
# to see the full symbolized stack-buffer-overflow report.
.PHONY: run-asan
run-asan: fuzzer parser_asan
	ASAN_OPTIONS=abort_on_error=1:detect_leaks=0:symbolize=1 \
		./fuzzer ./parser_asan seeds out dict.txt

# Smoke test (Linux): fuzz for a bounded time, then assert a crash was found.
# `timeout` returns 124 when it stops the fuzzer; we swallow that and check the
# crashes directory. Proves the whole pipeline end to end.
.PHONY: test
test: fuzzer parser
	@rm -rf out && mkdir -p out
	@echo "[*] fuzzing for up to 30s to find the planted crash..."
	@timeout 30 ./fuzzer ./parser seeds out dict.txt || true
	@if ls out/crashes/* >/dev/null 2>&1; then \
		echo "[+] PASS: crash(es) found:"; ls -1 out/crashes; \
		echo "[+] reproduce: ./parser out/crashes/$$(ls out/crashes | head -1)"; \
	else \
		echo "[!] FAIL: no crash found in the time budget"; exit 1; \
	fi

# Reproduce a saved crash standalone (no fuzzer): the target runs once, the fork
# server no-ops because there is no control pipe, and it faults on the input.
.PHONY: repro
repro:
	@echo "run a saved crash directly, e.g.:"
	@echo "    ./parser        out/crashes/<id>   # weak build: SIGSEGV"
	@echo "    ./parser_asan   out/crashes/<id>   # ASan: full OOB report"

# =============================================================================
# Assembly deliverable — regenerate the committed teaching .s from asm/demo.c.
# The hand-written asm/demo.annotated.s is authored, never overwritten here.
# =============================================================================
.PHONY: asm
asm:
	$(CC) $(ASM_COMMON) -O0 -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	$(CC) $(ASM_COMMON) -O1 -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	$(CC) $(ASM_COMMON) -O2                          asm/demo.c -o asm/demo.O2.s
	@echo "wrote asm/demo.{O0.s,s,O2.s} — now compare with asm/demo.annotated.s"

.PHONY: clean
clean:
	rm -f fuzzer parser parser_hardened parser_asan rt.o
	rm -rf out
