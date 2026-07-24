# reverse-proxy / L4 load balancer 🟧

**What it is.** A single-threaded, `epoll`-driven TCP reverse proxy. It accepts
client connections, picks a backend by a selectable policy (**round-robin**,
**least-connections**, or **consistent-hash on the client IP**), and shovels
bytes both ways with **zero-copy `splice(2)`** through a kernel pipe — the data
never enters this process's address space. A `timerfd` drives periodic health
probes that **eject and re-admit** backends with hysteresis; an AF_UNIX control
socket lets an admin **gracefully drain** a backend; and `SIGINT`/`SIGTERM` start
a graceful whole-process drain. Optional **connection pooling** pre-warms idle
upstream sockets so a new client can start splicing without paying a handshake.

This is the *teaching core* of what nginx's `stream {}` module and HAProxy in
TCP mode do. It is a faithful L4 (byte-transparent) proxy; it does **not** parse
HTTP, do TLS, or multiplex requests over pooled upstream connections — see
[Going further](#going-further) for exactly where the line is drawn.

## What you'll learn

- **`splice(2)` / zero-copy.** Why `read()`+`write()` copies every byte twice
  across the user/kernel boundary, and how splicing socket→pipe→socket moves
  bytes by *page reference* inside the kernel instead. (`sendfile(2)` is the
  file→socket special case of the same machinery.)
- **`epoll(7)` edge- vs level-triggered.** One event loop multiplexing thousands
  of fds; why the hot per-connection fds are **edge-triggered** (drain to
  `EAGAIN` or stall forever) while the control fds stay level-triggered.
- **Non-blocking `connect()`** and reading the result back via
  `getsockopt(SO_ERROR)` when `epoll` reports the socket writable.
- **Consistent hashing** with virtual nodes: why `hash % N` is catastrophic when
  `N` changes and how a hash ring remaps only `K/N` keys instead.
- **`timerfd` / `signalfd`**: folding periodic timers and Unix signals into the
  same `epoll` set so there is no async-signal-safety minefield and no separate
  timer thread.
- **Backpressure, half-close, and graceful drain**: TCP `shutdown(SHUT_WR)` to
  relay EOF one direction at a time, and how a proxy applies flow control by
  letting the pipe fill.

Syscalls exercised: `splice`, `epoll_create1`/`epoll_ctl`/`epoll_wait`,
`accept4`, `connect`, `pipe2`, `timerfd_create`/`timerfd_settime`, `signalfd`,
`shutdown`, `getsockopt`, `getaddrinfo`.

## Build & run (Linux / WSL2)

Everything here is Linux-specific (`splice`, `epoll`, `timerfd`, `signalfd`,
`accept4`). It builds and runs on Linux or WSL2. **No root or capabilities are
needed** unless you bind the listener to a privileged port (< 1024).

```bash
make                       # builds ./lb with -Wall -Wextra

# In terminal 1: two throwaway HTTP backends on :9001 and :9002
make backends              # (uses python3 -m http.server)

# In terminal 2: the proxy — consistent hashing, warm pool of 4, 1s health checks
./lb -l 0.0.0.0:8080 -b 127.0.0.1:9001 -b 127.0.0.1:9002 -p hash -w 4 -i 1000
#   (or just `make run`, which uses exactly this line)

# In terminal 3: drive traffic through it
curl -s http://127.0.0.1:8080/         # served by one of the backends
```

Options:

| flag | meaning | default |
|------|---------|---------|
| `-l host:port` | listen address | `0.0.0.0:8080` |
| `-b host:port` | add a backend (repeatable, **required**) | — |
| `-p rr\|lc\|hash` | selection policy | `rr` |
| `-w N` | warm pool size per backend (`0` = off) | `0` |
| `-i ms` | health-check interval | `2000` |
| `-c path` | admin control socket | `/tmp/lb.ctl` |

**Watch health eject/re-admit:** kill a backend (Ctrl-C the `:9002` server) and
watch the log print `backend 127.0.0.1:9002: DOWN (ejected from rotation)` after
`HEALTH_FALL` failed probes; restart it and see `UP (re-added)`.

**Graceful per-backend drain** via the control socket:

```bash
socat - UNIX-CONNECT:/tmp/lb.ctl        # then type a command:
  status                                # dump policy + every backend's state
  drain 1                               # backend 1 takes no NEW conns; open ones finish
  undrain 1                             # put it back in rotation
```

`Ctrl-C` (SIGINT) the proxy to drain the whole process: it stops accepting,
finishes in-flight connections, then exits.

## How it works

Four files:

- **`lb.h`** — the whole data model in one place: `struct backend` (address,
  UP/DOWN, drain flag, live-conn count, warm pool, health probe state),
  `struct conn` (the two fds + two `struct pump`s), `struct pump` (one
  unidirectional splice flow: two socket fds, a staging pipe, and the byte count
  parked in it), the hash `ring`, and the `io_handle` tag every fd carries in its
  `epoll` `data.ptr`.

- **`hashring.c`** — the consistent-hash ring. `lb_hash` is FNV-1a (fast, good
  dispersion, and its mod-2³² multiply *is* the ring coordinate). `ring_build`
  scatters `RING_VNODES` (160) virtual nodes per **eligible** backend around the
  ring and sorts them; `ring_lookup` binary-searches for the first node clockwise
  of the key and wraps. Down/draining backends contribute no vnodes, so a lookup
  can never return one — that is how ejection works for policy `hash`.

- **`lb.c`** — everything with a syscall in it:
  - *Selection* (`select_backend`): RR cursor, LC min-scan, or ring lookup.
  - *The splice data path* (`pump`): the core routine. For one direction it
    loops `splice(src_socket → pipe)` then `splice(pipe → dst_socket)` until
    neither makes progress, tracking bytes in the pipe for backpressure, then
    relays EOF with `shutdown(SHUT_WR)`. Read its header comment for the exact
    `EAGAIN` / `SPLICE_F_MORE` / half-close reasoning.
  - *Connection lifecycle* (`on_accept`, `finalize_connect`, `on_conn_event`,
    `conn_shutdown`): non-blocking connect, edge-triggered pumping, and **lazy
    teardown** — a closed conn is parked and freed only after the whole
    `epoll_wait` batch, because a batch can carry events for *both* of its fds and
    freeing on the first would dangle the second.
  - *Health* (`on_timer`, `launch_probe`, `probe_result`): async TCP connect
    probes with `HEALTH_RISE`/`HEALTH_FALL` hysteresis so a single blip can't
    flap a backend.
  - *Warm pool* (`pool_refill`, `pool_pop`, `pool_drain`): pre-connected idle
    sockets, filled one-per-tick, drawn on accept.
  - *Signals & control* (`signalfd`, AF_UNIX socket) folded into the same loop.
  - `main`: parse args, resolve backends, build the `epoll` set (listener +
    timerfd + signalfd + control socket), run the loop, clean up.

The one epoll loop dispatches purely on `handle->kind`, so there is no
fd→object hash table: the object pointer rides along in `data.ptr`.

## Assembly notes

`asm/demo.c` is a **self-contained** extraction of the three selection routines
(no system headers, own types), because `lb.c`/`hashring.c` pull in Linux-only
headers and can't be lowered to standalone asm. It compiles to genuine SysV
assembly with the exact commands in the brief (see the Makefile `asm` target):

- **`asm/demo.s`** (`-O1`) is annotated instruction-by-instruction in
  **`asm/demo.annotated.s`** with a full ABI header. Highlights:
  - `fnv1a_32` keeps its entire state in `eax` (also the return register): the
    loop is just `movzbl` a byte, `xorl` it in, `imull` by the FNV prime — and
    that 32-bit `imul` *is* the modular arithmetic that makes the hash a ring
    coordinate.
  - `ring_lookup` is a lower-bound binary search; watch the overflow-safe
    midpoint (`sub`/`sar`/`lea`), the **unsigned** `jae` on `.hash` (ring coords
    are unsigned), the scale-8 struct addressing (`.hash` at `+0`, `.idx` at
    `+4`), and the **branchless wrap** — the "if past the end, use node 0" step
    compiles to a single `cmovne`.
  - `least_conn_pick` shows how seeding `best = -1` lets one `js` take the first
    eligible backend with no separate "is this the first?" bookkeeping.
- **`asm/demo.O0.s`** is the naive mapping (every variable spilled to the stack),
  easiest to read one C statement at a time; **`asm/demo.O2.s`** shows the
  optimizer off the leash. Regenerate all three with `make asm` (clang
  cross-targets Linux from any host).

## Going further

The list's **`Stretch:`** goal and what production systems add:

- **L7 / HTTP awareness.** Parse the request line and route on `Host:`/path;
  this is also the prerequisite for *real* connection pooling — reusing one
  keep-alive backend connection across many client requests, which an
  L4 byte-splicer fundamentally cannot do (a raw TCP stream is consumed by its
  one client). Our pool therefore only *pre-warms* connections.
- **The PROXY protocol** (HAProxy's) so the backend learns the real client IP,
  since an L4 proxy otherwise hides it behind the proxy's own address.
- **Weighted / bounded-load consistent hashing** (heterogeneous backends;
  "consistent hashing with bounded loads") and **IPv6** (already one `AF_UNSPEC`
  away — the code carries `sockaddr_storage` throughout).
- **Passive health checks** (eject on real traffic errors, not just probes) and
  **outlier detection**, plus per-connection byte budgets in `pump` for fairness
  under load, and growing the pipe with `fcntl(F_SETPIPE_SZ)`.

## References

- `man 2 splice`, `man 2 sendfile`, `man 7 epoll`, `man 2 timerfd_create`,
  `man 2 signalfd`, `man 2 accept4` — the primary sources for every syscall here.
- **nginx** `ngx_event_pipe.c` / the `stream` module, and **HAProxy** `mux_pt.c`
  — production splice-based L4 proxying. HAProxy pioneered kernel-splice proxying.
- Karger et al., *"Consistent Hashing and Random Trees"* (STOC 1997), and
  **Ketama** (last.fm) — the origin and the canonical memcached implementation of
  the virtual-node ring in `hashring.c`.
- Linux `fs/splice.c` — how `struct pipe_buffer` passes page references around,
  i.e. why "zero-copy" is literally zero copies.
