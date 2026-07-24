# A C10k HTTP/1.1 server 🟧

**What it is.** A single-threaded-per-core, non-blocking HTTP/1.1 static file
server built to scale toward the **C10k** regime — ten thousand concurrent
connections on one box. It multiplexes every socket through an **epoll
edge-triggered** event loop, uses `accept4` + `O_NONBLOCK` so nothing ever
blocks the loop, parses requests with a **hand-rolled incremental state machine**
that tolerates partial reads (and thus resists slowloris), streams file bodies
with **`sendfile`** (zero-copy), keeps connections alive between requests, and
scales across cores with **`SO_REUSEPORT`** worker processes. It is a teaching
*core*: see [Scope](#scope-what-this-covers-and-omits) for exactly what is and
is not implemented.

## What you'll learn

- **`epoll`** and the difference between **edge-triggered (`EPOLLET`)** and
  level-triggered readiness — and why edge-triggered forces you to *drain every
  fd to `EAGAIN`* on each wakeup or hang forever.
- **`accept4(2)`** with `SOCK_NONBLOCK | SOCK_CLOEXEC`: accepting and setting fd
  flags in one syscall to avoid the `accept()`+`fcntl()` race.
- **`O_NONBLOCK`** socket semantics: `EAGAIN`/`EWOULDBLOCK`, `EINTR`, and
  partial `send`/`recv`.
- A **byte-at-a-time HTTP parser** as a resumable finite-state machine — the
  shape of software that must survive receiving one byte at a time.
- **`sendfile(2)`**: copying a file's page cache straight to a socket without a
  user-space bounce buffer.
- **`SO_REUSEPORT`**: many processes binding the same port, kernel-load-balanced,
  with no shared accept lock or thundering herd.
- The connection lifecycle: **HTTP keep-alive**, `EPOLLRDHUP`, `EPOLLERR`,
  `EPOLLHUP`, and `SIGPIPE`/`MSG_NOSIGNAL`.
- Network byte order (`htons`/`htonl`) and why the wire is big-endian.

## Build & run

**Platform: Linux (or WSL2).** The event-loop code needs Linux syscalls
(`epoll`, `accept4`, `sendfile`, `SO_REUSEPORT`); it will not build on Windows
or macOS. The *assembly* deliverable is host-portable (clang cross-targets
Linux) — you can run `make asm` anywhere.

```bash
make                       # clang -Wall -Wextra -O2 server.c http_parser.c -o server
./server                   # defaults: port 8080, root ./www, 1 worker
./server 8080 ./www 4      # port 8080, docroot ./www, 4 worker processes

# from another terminal:
curl -v --http1.1 http://localhost:8080/          # keep-alive GET of index.html
curl -v -X HEAD  http://localhost:8080/index.html # headers only, no body
curl -v          http://localhost:8080/nope       # 404

# load test (the whole point) — 10k connections, 4 threads, 10 seconds:
wrk -t4 -c10000 -d10s http://localhost:8080/
```

Inspect the machinery while it runs:

```bash
strace -f -e trace=network,epoll_ctl,epoll_wait,sendfile ./server   # watch the syscalls
ss -tan | grep :8080                                                # sockets & states
cat /proc/sys/net/core/somaxconn                                    # your accept backlog cap
# to actually reach 10k fds you must raise the per-process limit:
ulimit -n 100000
```

## How it works

Five files; read them in this order.

- **`http_parser.h` / `http_parser.c`** — the incremental request-head parser. A
  finite-state machine (`enum hp_state`) that consumes each received byte
  *exactly once* and remembers its position in `req->parsed`, so a request
  split across many `recv()` calls parses correctly with no re-scanning. Field
  boundaries are stored as `(offset,length)` slices *into the caller's buffer* —
  zero copy, zero allocation. A hard `HP_MAX_HEADER_BYTES` ceiling counts every
  head byte: this single bound is what turns "tolerant of partial reads" into
  "immune to slowloris." The file has **no system headers**, so it doubles as a
  freestanding library and yields clean teaching assembly.

- **`server.c`** — the event loop and everything around it:
  - `make_listener()` — `socket` + `SO_REUSEADDR` + `SO_REUSEPORT` + `bind` +
    `listen`. Each worker calls it for its **own** listening fd.
  - `accept_loop()` — edge-triggered accept: `accept4` in a loop until `EAGAIN`.
  - `on_readable()` — edge-triggered read: `recv` until `EAGAIN`, appending to a
    per-connection buffer, then hand the bytes to the parser.
  - `build_response()` — method routing (GET/HEAD), path-traversal defense,
    `open`+`fstat`, MIME typing, and the response head.
  - `flush()` — `send` the head then `sendfile` the body, both draining to
    `EAGAIN`; if the socket fills, it registers `EPOLLOUT` and resumes later.
  - `process()` — the engine that ties parse → build → flush together and loops
    over pipelined requests, driving keep-alive.
  - `worker_loop()` — `epoll_create1`, register the listener, and the
    `epoll_wait` dispatch loop.
  - `main()` — `fork` the worker pool (each an independent loop over its own
    `SO_REUSEPORT` socket) and ignore `SIGPIPE`.

- **`www/index.html`** — something to serve.

### Edge-triggered vs level-triggered (the core idea)

`epoll` can report readiness two ways:

- **Level-triggered (default):** "this fd is readable" is re-reported on every
  `epoll_wait` as long as *any* data is buffered. Forgiving — you can read a
  little and come back — but a busy fd can wake you repeatedly.
- **Edge-triggered (`EPOLLET`):** readiness is reported only on the *transition*
  from not-ready to ready (a new packet lands). If you read some but not all of
  the buffered bytes, epoll will **not** wake you again for the leftovers — you
  will hang until, by luck, more data arrives.

The consequence, obeyed everywhere in this server: **on every edge-triggered
wakeup, loop until the syscall returns `EAGAIN`/`EWOULDBLOCK`.** That is why
`accept_loop`, `on_readable`, and `flush` are all `while` loops that terminate on
`EAGAIN`. Edge-triggered means fewer wakeups (better at 10k fds) at the cost of
this discipline.

### The write path and keep-alive

A response is a header block (`whead`) followed, for GET, by a file body streamed
with `sendfile`. `flush()` pushes both, tracking `wsent`/`file_left` so a partial
write resumes exactly where it stopped. If the socket send buffer fills, we stop,
switch the connection's epoll interest to `EPOLLOUT`, and return; the next
writable edge re-enters `process()` to continue. When the whole response is out,
a keep-alive connection is *recycled* (`keepalive_reset`) — leftover pipelined
bytes are shifted to the front of the buffer and the parser is zeroed — otherwise
the socket is closed.

## Scope: what this covers and omits

Honest boundaries of the teaching core:

- **Implements:** GET and HEAD for static files; HTTP/1.1 keep-alive; correct
  partial-read / slowloris handling; path-traversal rejection; `sendfile`
  bodies; `SO_REUSEPORT` multi-process scaling; back-to-back (pipelined)
  *bodyless* requests.
- **Omits:** request bodies (POST/PUT) — a request carrying `Content-Length > 0`
  is answered then the connection is closed to avoid stream desync; chunked
  transfer-encoding; TLS; per-connection idle timeouts (a real server arms a
  timer wheel to reap slow-but-not-dead clients); URL percent-decoding; a
  growable read buffer (a single >16 KiB pipelined burst is a documented
  edge-triggered stall — see the comment in `on_readable`). None of these change
  the event-loop lessons; they are additive.

## Assembly notes

The annotated assembly is [`asm/demo.annotated.s`](asm/demo.annotated.s), built
from the project's most instructive pure-logic routine: the **parser state
machine step**, extracted self-contained into [`asm/demo.c`](asm/demo.c) (its
own types, no system headers). What the annotation highlights:

- A **`switch (state)` compiled with `-fno-jump-tables` becomes a binary search**
  of `cmp`/`jcc` over the state value — that *is* what a state machine looks like
  as machine code. (With jump tables it would be one indirect jump; the file
  explains the trade-off.)
- **`is_tchar()` is inlined** at every call site, and its 14-way "is this token
  punctuation?" `case` collapses into a single **`btq`** bit-test against a
  64-bit mask (`0xE00000000000367D`), indexed by `c - 33`. The two out-of-window
  characters `|` and `~` are tested separately — the file decodes the mask
  bit-by-bit.
- The whole per-byte loop runs with **no stack locals**: the cursor, current
  byte, running header-byte count, and both loop-invariant constants all live in
  registers, which is why the machine is cheap enough to run on every octet.
- The three `return` statements collapse into a branchless `sete`/`cmov` tail.

Because `http_parser.c` is *also* self-contained, its real compiler output is
committed too — [`asm/http_parser.s`](asm/http_parser.s) (and `.O0.s`/`.O2.s`) —
so you can compare the distilled demo against the production parser.

Regenerate everything (works on any host — clang cross-targets Linux):

```bash
make asm
```

The committed `.s` files are genuine `clang --target=x86_64-pc-linux-gnu -S`
output at `-O0` (naive, one block per C statement), `-O1` (the annotated
baseline), and `-O2` (optimizer unleashed).

## Going further

- **Stretch — HTTP/2:** binary framing over one connection, stream multiplexing,
  and **HPACK** header compression (a static + dynamic table of header fields
  with Huffman coding). The parser stops being a byte state machine and becomes a
  frame decoder; flow control moves into the protocol.
- **Stretch — HTTP/3 over QUIC:** the transport moves to **UDP**, with streams,
  loss recovery, and congestion control implemented in user space, plus TLS 1.3
  baked in. `sendfile` no longer applies; you copy into QUIC packets yourself.
- **What production does:** `io_uring` instead of epoll for fully-async I/O
  including disk (see `../05-io-uring-server`); a timer wheel for idle-connection
  reaping; `TCP_NODELAY`/`TCP_CORK` tuning; `accept` with `EPOLLEXCLUSIVE` when
  sharing one listener across threads; sendfile fallbacks (`splice`, or `read`+
  `send` for TLS); a real MIME database and byte-range (`Range:`) support.

## References

- **C10k problem** — Dan Kegel's original write-up (the name of this whole class
  of problem).
- `man 7 epoll` (esp. the "Edge-triggered and level-triggered" and "Possible
  pitfalls" sections), `man 2 epoll_ctl`, `man 2 epoll_wait`.
- `man 2 accept4`, `man 2 sendfile`, `man 7 socket` (`SO_REUSEPORT`),
  `man 2 recv`, `man 2 send` (`MSG_NOSIGNAL`).
- **RFC 7230** — HTTP/1.1 message syntax (the grammar the parser implements).
- Read the source of the real thing: **nginx** (`src/event/ngx_epoll_module.c`,
  `src/http/ngx_http_parse.c`), **haproxy**, and **seastar** (thread-per-core).
