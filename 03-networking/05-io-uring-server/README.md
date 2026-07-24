# An io_uring server 🟥

**What it is.** A fully asynchronous TCP echo server built on **io_uring**, the
Linux completion-based I/O interface. Accept, receive, send, and close are all
submitted to the kernel as ring entries and reaped later as completions — the
program never calls `accept()`, `recv()`, `send()`, or `close()` directly. It
uses the four features that make io_uring more than "epoll with extra steps":
**multishot accept**, **multishot recv**, a **provided-buffer ring** (registered
buffers the kernel draws from on demand), and a **registered file** (the listener
lives in the ring's fixed-file table). Alongside it ships an **epoll** server
speaking the identical protocol and a load generator, so you can measure the
thing io_uring is really about: **syscalls per message**.

This is a **teaching core**, honestly scoped. `echo_uring.c` is a complete,
correct, end-to-end async server — it accepts, echoes, and closes thousands of
connections through one thread and (in steady state) one syscall per batch. It
deliberately omits the production machinery discussed in
[Going further](#going-further): `IORING_SETUP_SQPOLL`/`DEFER_TASKRUN`, direct
(registered) descriptors for *accepted* sockets, `READ_FIXED`/`WRITE_FIXED`
fixed buffers, and short-write re-submission. Those gaps are called out inline in
the source and summarized below.

## What you'll learn

- **The SQ/CQ ring model.** Two shared ring buffers — a Submission Queue you
  produce into and a Completion Queue the kernel produces into — plus one
  `io_uring_enter(2)` that submits and waits at once. Why one enter can carry a
  whole batch of operations *and* their results.
- **The head/tail memory ordering** — the single synchronization between you and
  the kernel. The producer **store-releases** the tail after filling an entry;
  the consumer **acquire-loads** the tail before reading it. `asm/demo.c`
  distills this to plain atomics and the annotated asm shows it costs *zero*
  extra instructions on x86-TSO.
- **Multishot operations** — one SQE that stays armed and emits many CQEs
  (`IORING_CQE_F_MORE`), removing the per-accept and per-read re-arm syscall.
- **Provided buffer rings** (`io_uring_setup_buf_ring`, `IOSQE_BUFFER_SELECT`) —
  letting the kernel pick a receive buffer from a registered pool, and the
  ownership discipline (don't recycle a buffer until the echo that reads from it
  has completed).
- **Registered files** (`io_uring_register_files`, `IOSQE_FIXED_FILE`) — naming a
  fd by table index to skip the per-op `fdget`/`fput`.
- **The contrast with epoll** — readiness (`epoll_wait` then *you* `read`) vs
  completion (the kernel `read`s and reports), and why that changes the syscall
  count per message from O(1) to ~O(1/batch).
- **`user_data` as a tagged cookie** — packing op-type + fd + buffer-id into the
  free 64-bit word io_uring hands back, so no per-connection allocation is needed.

## Build & run (Linux / WSL)

**Linux-only.** `echo_uring` needs **liburing** and a **kernel ≥ 5.19**:

```bash
sudo apt install liburing-dev strace   # Debian/Ubuntu (Fedora: liburing-devel)
make                                    # builds echo_uring, epoll_echo, bench_client
```

Run a server (Ctrl-C to stop) and, in another terminal, drive it:

```bash
make run                                # io_uring server on :8080
# or: make run-epoll

./bench_client 127.0.0.1 8080 64 5 64   # host port conns seconds msgsize
#   -> connections / round-trips / echoes-per-sec / MiB-per-sec
```

`make bench` does the start-load-stop dance for you. Verify correctness with any
raw-TCP tool — the server echoes bytes verbatim:

```bash
printf 'hello io_uring\n' | nc -q1 127.0.0.1 8080     # prints: hello io_uring
```

The **assembly** in `asm/` regenerates on any host (clang cross-targets Linux):

```bash
make asm
```

### The headline measurement: `make compare`

```bash
make compare      # runs BOTH servers under `strace -f -c` on identical load
```

`strace -c` tallies every syscall. The point is not the throughput number (this
is a *closed-loop* client — one request in flight per connection — so both
servers are latency-bound, not saturated); the point is the **shape of the
syscall table**. Mechanically it must look like this (per echo message):

| syscall              | epoll server           | io_uring server                     |
|----------------------|------------------------|-------------------------------------|
| readiness / wait     | `epoll_wait` (1)       | folded into `io_uring_enter`        |
| get the bytes        | `recvfrom`/`read` (1)  | kernel does it; **0** from us       |
| send the echo        | `sendto`/`write` (1)   | kernel does it; **0** from us       |
| re-arm interest      | occasional `epoll_ctl` | **0** (multishot stays armed)       |
| **total user syscalls/msg** | **~3**          | **~1/batch → approaches 0**         |

So under load the epoll server's syscall count scales with the message count,
while the io_uring server's scales with the number of `io_uring_enter` *batches*
— many messages reaped per enter. That collapse (visible directly in the two
`strace` tables) is the entire result. Numbers are described mechanistically
rather than pasted here because they depend on your kernel and load; run
`make compare` to see them on your box (the full tables are saved to
`uring.strace` and `epoll.strace`).

## How it works

**`echo_uring.c`** — the flagship. Read it top-to-bottom; it is built as:

- *`user_data` codec* — `ud_make`/`ud_op`/`ud_fd`/`ud_bid` pack op-type (8 bits),
  buffer id (16), and fd (32) into the cookie io_uring echoes back, so a CQE is
  self-describing with no side table.
- *Setup (`main`)* — `io_uring_queue_init` mmaps the rings;
  `io_uring_register_files` pins the listener at fixed-file index 0;
  `io_uring_setup_buf_ring` registers a 2048-entry provided-buffer pool and the
  initial fill publishes the whole pool with one `io_uring_buf_ring_advance`
  (one store-release).
- *Arming helpers* — `arm_accept` (one multishot ACCEPT via the fixed listener),
  `arm_recv` (multishot RECV with `IOSQE_BUFFER_SELECT`), `arm_send` (echo
  straight out of the pool buffer, zero-copy), `arm_close` (async CLOSE).
- *The event loop* — `io_uring_submit_and_wait(&ring, 1)` is the one steady-state
  syscall: it flushes staged SQEs and blocks for ≥1 CQE. `io_uring_for_each_cqe`
  drains the whole ready batch; `io_uring_cq_advance` releases them all at once.
- *Completion handlers* — `on_accept` re-arms only if the kernel drops
  `F_MORE`; `on_recv` echoes on `res>0`, treats `res==0` as EOF, and re-arms on
  `-ENOBUFS`; `on_send` recycles the buffer back to the pool (the release half of
  the buffer-ownership rule). Every error path is commented with the errno and
  what it means.

**`epoll_echo.c`** — the control. Single-threaded, level-triggered epoll. It
toggles interest between `EPOLLIN` and `EPOLLOUT` so a client with a full send
buffer never spins `epoll_wait`, bounds pending output to one chunk per
connection, and handles `EAGAIN`/`EINTR`/`EPOLLHUP`/`EPOLLERR` and partial
writes. Its job is to be the honest, minimal readiness-model baseline the
comparison is measured against.

**`bench_client.c`** — a pthreaded closed-loop load generator: N connections,
each ping-ponging a fixed message and counting round-trips, with `TCP_NODELAY`,
full-read reassembly (`read_full`), and a monotonic-clock timer.

**`common.h`** — the listener/`setsockopt`/`O_NONBLOCK` boilerplate both servers
share, so each server file starts at its event loop.

**`asm/demo.c`** — a self-contained extraction of the pure ring logic for the
assembly deliverable (see below).

## Assembly notes

Neither server is compilable to standalone assembly — `echo_uring.c` needs
liburing and `epoll_echo.c` needs `<sys/epoll.h>` — so, per the repo convention,
`asm/demo.c` lifts out the part that is 100% register-and-atomic logic and is
also the most instructive: **the ring index masking and the store-release / 
load-acquire handshake** on the head/tail indices. It models the SQ and CQ with
the same `__ATOMIC_ACQUIRE`/`__ATOMIC_RELEASE` the kernel uses
(`smp_load_acquire`/`smp_store_release` in `fs/io_uring.c`) and implements the
cores of `io_uring_get_sqe`, `io_uring_submit`, `io_uring_peek_cqe`,
`io_uring_cq_advance`, and `io_uring_buf_ring_advance`.

```bash
make asm   # regenerates asm/demo.{O0.s, s, O2.s} (needs clang; cross-targets Linux)
```

[`asm/demo.annotated.s`](asm/demo.annotated.s) is the hand-written,
per-instruction walkthrough of the `-O1` output. Its central lesson:

- **A ring slot is one `and`.** `pos & (entries-1)` replaces `pos % entries`
  because the ring size is a power of two and the position counter is free to
  wrap at 2³² (`ring_slot`, and the `andl` inside `get_sqe`/`peek_cqe`).
- **Occupancy is wrap-safe unsigned subtraction.** `entries - (tail - head)`
  stays correct across the 2³² wrap (`sq_space_left`, reassociated by clang into
  `head + entries - tail`).
- **Release/acquire cost nothing on x86.** The store-release of the tail
  (`submit_one`, `buf_ring_advance`) is a plain `movl`/`addl` — **no `mfence`, no
  `lock`** — and the acquire-load (`sq_space_left`, `peek_cqe`) is a plain
  `movl`, because x86-TSO already forbids the reorderings they must forbid. The
  builtins' only job on x86 is to fence the *compiler*; the identical C emits
  `stlr`/`ldar` on ARM. `grep mfence asm/*.s` finds nothing — that absence *is*
  the point.

Compare [`asm/demo.O0.s`](asm/demo.O0.s) (every value spilled to the stack; the
release store still a plain `mov`) with [`asm/demo.O2.s`](asm/demo.O2.s) (frame
pointer dropped, `cq_advance` reduced to a two-instruction memory-add).

## Going further

The **`Stretch:`** direction for this project is to push the syscall count to
*literally zero* on the hot path and to widen the feature use. What a production
io_uring server adds beyond this core:

- **`IORING_SETUP_SQPOLL`** — a kernel poller thread watches the SQ tail, so
  submitting an SQE needs no `io_uring_enter` at all; steady state becomes a
  pure memory write. Pair with **`IORING_SETUP_SINGLE_ISSUER` +
  `IORING_SETUP_DEFER_TASKRUN`** for lower overhead completion processing.
- **Direct (registered) descriptors for accepted sockets** —
  `io_uring_prep_multishot_accept_direct` returns fds straight into the fixed
  file table (`IORING_FILE_INDEX_ALLOC`), so recv/send/close on them also use
  `IOSQE_FIXED_FILE` and never touch the process fd table. This core registers
  only the *listener* to keep the connection lifecycle simple.
- **Fixed buffers (`READ_FIXED`/`WRITE_FIXED`)** — `io_uring_register_buffers`
  pins a fixed iovec set once; each I/O names one by index and the kernel skips
  `get_user_pages` per call. We use a *provided-buffer ring* instead because it
  is what multishot recv requires (a multishot op cannot know how many buffers it
  will need in advance) — the two are different registered-memory mechanisms for
  different access patterns.
- **Short-write handling** — `on_send` assumes a successful send transmitted
  everything, true for small echoes but not in general; production tracks each
  send's length and re-submits the untransmitted tail (often via a **linked
  SQE**, `IOSQE_IO_LINK`, chaining recv→send so the kernel drives the echo with
  no userspace round trip at all).
- **Per-connection send ordering** — unlinked SEND SQEs on one socket have no
  guaranteed transmit order, so a message split across several recv buffers with
  several in-flight sends could be reordered. Our closed-loop client keeps one
  message in flight per connection, so this core never hits it; a pipelining
  server must serialize sends per connection (or link them). See the
  `arm_send` comment.
- **Backpressure & lifecycle** — bounded in-flight sends, idle timeouts
  (`IORING_OP_LINK_TIMEOUT`), and graceful drain on `SIGINT`.

## References

- **`man io_uring`, `man io_uring_setup`, `man io_uring_enter`** — the kernel ABI:
  ring layout, the `IORING_OP_*` opcodes, and the `IOSQE_*`/`IORING_CQE_F_*` flags.
- **liburing** (`git://git.kernel.dk/liburing`) — the userspace library; read
  `src/queue.c` (`io_uring_submit`, `__io_uring_flush_sq`) for the real
  store-release, and `examples/` for `io_uring-cp`, buffer-ring, and multishot demos.
- **"Efficient IO with io_uring"** — Jens Axboe's design PDF; the canonical
  explanation of the SQ/CQ rings and the head/tail memory ordering.
- **Linux `fs/io_uring.c` / `io_uring/`** — `smp_store_release`/`smp_load_acquire`
  on `sq->tail`/`cq->head`; this is the ground truth `asm/demo.c` mirrors.
- **`Documentation/memory-barriers.txt`** (Linux) — why release/acquire is the
  right pair here and what each forbids.
- **04-c10k-http-server** (this repo) — the epoll/threaded C10K server this one is
  the completion-model answer to.
