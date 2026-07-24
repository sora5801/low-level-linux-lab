# =============================================================================
# Makefile — build the io_uring echo server, the epoll baseline, and the client;
#            run a side-by-side syscall-count comparison; regenerate teaching asm.
# =============================================================================
# PLATFORM: the two servers are Linux-only. echo_uring additionally needs the
# io_uring userspace library (liburing) headers + lib:
#     Debian/Ubuntu : sudo apt install liburing-dev
#     Fedora        : sudo dnf install liburing-devel
#     Arch          : sudo pacman -S liburing
# and a kernel >= 5.19 (for provided buffer RINGS + multishot recv). On Windows,
# build inside WSL2. `make asm` works on ANY host (clang cross-targets Linux).
#
#   make            build echo_uring, epoll_echo, bench_client
#   make run        run the io_uring server on :8080  (Ctrl-C to stop)
#   make run-epoll  run the epoll server on :8080
#   make bench      build, start echo_uring, hammer it with bench_client, stop it
#   make compare    strace -c BOTH servers under identical load: the money shot,
#                   showing io_uring's syscalls-per-message collapse vs epoll
#   make asm        regenerate asm/demo.{O0.s,s,O2.s} with the exact clang cmds
#   make clean
# =============================================================================

CC      ?= cc
# -Wall -Wextra: warnings are errors in spirit (see CONVENTIONS.md). -O2 for a
# fair throughput comparison; -g keeps symbols for perf/gdb.
CFLAGS  ?= -Wall -Wextra -O2 -g
LDLIBS_URING   := -luring
LDLIBS_PTHREAD := -pthread

# Force the System V AMD64 (Linux) ABI for asm generation regardless of host OS.
ASM_TARGET ?= x86_64-pc-linux-gnu

PORT ?= 8080

.PHONY: all
all: echo_uring epoll_echo bench_client

# ---- the flagship io_uring server (needs liburing) --------------------------
echo_uring: echo_uring.c common.h
	$(CC) $(CFLAGS) echo_uring.c -o $@ $(LDLIBS_URING)

# ---- the epoll baseline (no external deps) ----------------------------------
epoll_echo: epoll_echo.c common.h
	$(CC) $(CFLAGS) epoll_echo.c -o $@

# ---- the load generator (POSIX threads) -------------------------------------
bench_client: bench_client.c common.h
	$(CC) $(CFLAGS) bench_client.c -o $@ $(LDLIBS_PTHREAD)

# ---- run targets ------------------------------------------------------------
.PHONY: run run-epoll
run: echo_uring
	./echo_uring $(PORT)
run-epoll: epoll_echo
	./epoll_echo $(PORT)

# ---- throughput smoke test (io_uring server) --------------------------------
# Starts the server in the background, drives it for 5s with 64 connections,
# then shuts it down. Adjust conns/seconds/msgsize on the bench_client line.
.PHONY: bench
bench: all
	@./echo_uring $(PORT) & echo $$! > .srv.pid ; sleep 0.3 ; \
	 ./bench_client 127.0.0.1 $(PORT) 64 5 64 ; \
	 kill `cat .srv.pid` 2>/dev/null ; rm -f .srv.pid

# ---- the headline comparison: SYSCALLS PER MESSAGE --------------------------
# Runs each server under `strace -f -c` (counts every syscall the process + its
# threads make) against an identical 3s / 64-connection load, then prints both
# summaries. The io_uring server's read/write/epoll_ctl counts should be ~0 on
# the hot path — the work happened inside a handful of io_uring_enter calls —
# while the epoll server shows one recvfrom + one sendto (or write) per echo.
# Requires strace. Runs the whole thing twice, once per server.
.PHONY: compare
compare: all
	@command -v strace >/dev/null || { echo "need strace: apt install strace"; exit 1; }
	@echo "=================================================================="
	@echo " io_uring server under strace -c  (expect: mostly io_uring_enter)"
	@echo "=================================================================="
	@strace -f -c -o uring.strace ./echo_uring $(PORT) & echo $$! > .srv.pid ; \
	 sleep 0.3 ; ./bench_client 127.0.0.1 $(PORT) 64 3 64 ; \
	 kill `cat .srv.pid` 2>/dev/null ; sleep 0.2 ; rm -f .srv.pid ; \
	 tail -n +1 uring.strace
	@echo "=================================================================="
	@echo " epoll server under strace -c   (expect: epoll_wait + recvfrom +"
	@echo "                                 sendto/write, one pair per echo)"
	@echo "=================================================================="
	@strace -f -c -o epoll.strace ./epoll_echo $(PORT) & echo $$! > .srv.pid ; \
	 sleep 0.3 ; ./bench_client 127.0.0.1 $(PORT) 64 3 64 ; \
	 kill `cat .srv.pid` 2>/dev/null ; sleep 0.2 ; rm -f .srv.pid ; \
	 tail -n +1 epoll.strace
	@echo "(full per-syscall tables saved to uring.strace and epoll.strace)"

# ---- regenerate the committed teaching assembly -----------------------------
# The EXACT commands the project spec mandates; asm/demo.annotated.s is authored
# by hand, NOT overwritten here.
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-written"

.PHONY: clean
clean:
	rm -f echo_uring epoll_echo bench_client .srv.pid uring.strace epoll.strace
