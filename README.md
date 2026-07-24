# low-level-linux-lab

A **teaching lab** of ~65 low-level systems-programming projects — from kernel
modules and a userspace TCP/IP stack to a JIT, a from-scratch OS, and hand-
written x86-64 — each implemented to be *read and learned from*, not shipped.

Every project is self-contained in its own directory and ships:

- **Heavily-commented source** — the *why* of every syscall, register, memory
  layout, and concurrency decision, not just the *what*.
- **A didactic README** — the concept, the syscalls/ABI it touches, how to build
  and run it (mostly on Linux/WSL/QEMU), and what to take away.
- **Generated + hand-annotated assembly** (for the C projects) — the Linux
  System V AMD64 assembly the compiler actually emits, with a companion
  `.annotated.s` that comments essentially every instruction. See the reference:
  [`04-security-asm/01-nolibc-programs/asm/hello.annotated.s`](04-security-asm/01-nolibc-programs/asm/hello.annotated.s).

> This lab implements the project list in
> [`../linux-low-level-projects.md`](linux-low-level-projects.md). It is for
> learning on your **own machines and VMs**. The offensive/security projects
> ship their blue-team detector where the list calls for one — that half is the
> more instructive build.

## Build status — complete ✅

All **65 projects** are implemented. Each ships heavily-commented source, a
7-part README, a Makefile/Kbuild, and — for the C projects — genuine
compiler-generated Linux SysV assembly plus a hand-annotated `.annotated.s`.

| Section | Built | Total |
|---------|:-----:|:-----:|
| 1. Kernel, drivers & modules | 15 | 15 ✅ |
| 2. Systems tools & utilities | 19 | 19 ✅ |
| 3. Networking & concurrency | 14 | 14 ✅ |
| 4. Security, RE & assembly | 12 | 12 ✅ |
| 5. Capstones | 5 | 5 ✅ |
| **Total** | **65** | **65 ✅** |

By the numbers: ~85k lines of commented source across 461 C/H/asm files, 66
hand-annotated assembly files, and 228 generated `.s` files. Every committed
`.s` is real `clang --target=x86_64-pc-linux-gnu -S` output (verified by
recompilation), never hand-faked. 🟥 giants ship an honest, runnable
*teaching-core* — each README states exactly what its core covers and omits.

> **Platform.** Most projects are Linux-only by nature (kernel modules, TUN/TAP,
> KVM, io_uring, seccomp, …); build and run them on Linux/WSL/QEMU as each README
> describes. The **assembly is host-portable**: `make asm` regenerates it
> anywhere clang is installed, because clang cross-targets Linux.

## How to use it

1. Pick a project. `cd` into its directory. Read its `README.md`.
2. `make` it (on Linux or WSL — most of this is Linux-only by nature).
3. Read the source top to bottom; it's written for that.
4. For C projects, open `asm/<unit>.annotated.s` next to the source and correlate
   the two. `make asm` regenerates the raw assembly on any host (clang cross-
   targets Linux). Reading machine code fluently is the meta-skill this lab
   trains.

## The toolbox you'll be using

`gdb` (scripting, `tui`, `rr` reverse-debug, KGDB) · `strace`/`ltrace` · `perf`
· `ftrace`/`trace-cmd` · `bpftrace`/`libbpf` · binutils (`objdump -d`,
`readelf -a`, `nm`, `addr2line`) · ASan/UBSan/TSan/valgrind · **QEMU** (boot
kernels, emulate ISAs, `-s -S` for gdb) · `/proc` & `/sys`.

## Build requirements

- **clang** (or gcc) — the assembly is generated with
  `clang --target=x86_64-pc-linux-gnu -S`, which works on any host.
- **Linux** (or WSL2) to *run* most projects; several need root, kernel headers
  (`linux-headers-$(uname -r)`), `libbpf`, `liburing`, or **QEMU**. Each README
  states its own requirements. **Do kernel work in a VM.**

---

## The projects

Difficulty: 🟩 solid-intermediate · 🟧 hard · 🟥 brutal (multi-week+).
Status: ✅ implemented · 🧩 teaching-core (a runnable subset of a 🟥 giant, with
the full path documented).

### 1. Kernel, drivers & modules  — `01-kernel/`
| # | Project | Diff | What it teaches |
|---|---------|:----:|-----------------|
| 01 | [hello-lkm](01-kernel/01-hello-lkm) | 🟩 | `module_init`/`exit`, `module_param`, `printk` |
| 02 | [char-device-ioctl](01-kernel/02-char-device-ioctl) | 🟧 | `file_operations`, `ioctl`, wait queues, `mmap` |
| 03 | [ram-block-device](01-kernel/03-ram-block-device) | 🟧 | `blk_mq`, `bio`, `gendisk` |
| 04 | [in-memory-fs](01-kernel/04-in-memory-fs) | 🟥 | VFS: `super`/`inode`/`dentry`, `mount` |
| 05 | [proc-sys-debugfs](01-kernel/05-proc-sys-debugfs) | 🟧 | `proc_create`, `sysfs`, `debugfs`, `seq_file` |
| 06 | [kprobe-ftrace-tracer](01-kernel/06-kprobe-ftrace-tracer) | 🟧 | `kprobe`/`kretprobe`, `ftrace_ops` |
| 07 | [ebpf-xdp-filter](01-kernel/07-ebpf-xdp-filter) | 🟥 | `libbpf`, XDP, BPF maps, the verifier |
| 08 | [lkm-rootkit-and-detector](01-kernel/08-lkm-rootkit-and-detector) | 🟥 | ftrace syscall hooks **+ the detector** |
| 09 | [netfilter-hook](01-kernel/09-netfilter-hook) | 🟧 | `nf_register_net_hook`, `sk_buff`, verdicts |
| 10 | [virtual-net-device](01-kernel/10-virtual-net-device) | 🟥 | `net_device_ops`, NAPI, `sk_buff` |
| 11 | [add-syscall](01-kernel/11-add-syscall) | 🟥 | `SYSCALL_DEFINEn`, the syscall table |
| 12 | [sched-ext-scheduler](01-kernel/12-sched-ext-scheduler) | 🟥 | `sched_ext` BPF schedulers |
| 13 | [lsm-module](01-kernel/13-lsm-module) | 🟥 | LSM hooks, MAC, `current`, credentials |
| 14 | [rcu-hashtable](01-kernel/14-rcu-hashtable) | 🟥 | RCU, grace periods, memory barriers |
| 15 | [kvm-hypervisor](01-kernel/15-kvm-hypervisor) | 🟥 | `/dev/kvm` ioctls, VM exits, guest memory |

### 2. Systems tools & utilities  — `02-systems-tools/`
| # | Project | Diff | What it teaches |
|---|---------|:----:|-----------------|
| 01 | [shell](02-systems-tools/01-shell) | 🟧 | `fork`/`execve`/`dup2`/`pipe`, **job control** |
| 02 | [container-runtime](02-systems-tools/02-container-runtime) | 🟧 | namespaces, cgroups v2, `pivot_root`, seccomp |
| 03 | [init-supervisor](02-systems-tools/03-init-supervisor) | 🟧 | PID 1, `SIGCHLD` reaping, `signalfd` |
| 04 | [coreutils](02-systems-tools/04-coreutils) | 🟩 | short read/write, `EINTR`, `inotify`, `getdents64` |
| 05 | [malloc](02-systems-tools/05-malloc) | 🟥 | `brk`/`mmap`, free lists, coalescing, `LD_PRELOAD` |
| 06 | [garbage-collector](02-systems-tools/06-garbage-collector) | 🟥 | conservative mark-sweep, stack scanning |
| 07 | [dynamic-linker](02-systems-tools/07-dynamic-linker) | 🟥 | ELF load, relocations, PLT/GOT, `.init_array` |
| 08 | [elf-toolkit](02-systems-tools/08-elf-toolkit) | 🟧 | ELF headers, symbols, a disassembler backend |
| 09 | [ptrace-debugger](02-systems-tools/09-ptrace-debugger) | 🟥 | `ptrace`, `0xCC` breakpoints, DWARF lines |
| 10 | [strace-clone](02-systems-tools/10-strace-clone) | 🟧 | `PTRACE_SYSCALL`, seccomp trace, arg decode |
| 11 | [sampling-profiler](02-systems-tools/11-sampling-profiler) | 🟧 | `perf_event_open`, ring buffer, flame graphs |
| 12 | [jit-compiler](02-systems-tools/12-jit-compiler) | 🟥 | encode x86-64, `mmap(PROT_EXEC)`, W^X |
| 13 | [embedded-db](02-systems-tools/13-embedded-db) | 🟥 | B-tree, WAL, crash recovery, `fsync` |
| 14 | [regex-engine](02-systems-tools/14-regex-engine) | 🟧 | Thompson NFA → DFA, `O(n)` matching |
| 15 | [language-vm](02-systems-tools/15-language-vm) | 🟥 | bytecode, computed-goto dispatch, a GC |
| 16 | [make-clone](02-systems-tools/16-make-clone) | 🟧 | dependency DAG, staleness, jobserver |
| 17 | [mini-git](02-systems-tools/17-mini-git) | 🟧 | content-addressed store, zlib, the index |
| 18 | [green-threads](02-systems-tools/18-green-threads) | 🟥 | context switch in asm, M:N scheduler |
| 19 | [sync-primitives](02-systems-tools/19-sync-primitives) | 🟧 | `futex`, atomics, memory ordering |

### 3. Networking & concurrency  — `03-networking/`
| # | Project | Diff | What it teaches |
|---|---------|:----:|-----------------|
| 01 | [userspace-tcpip](03-networking/01-userspace-tcpip) | 🟥 | TUN/TAP, ARP, IPv4, ICMP, UDP, **TCP** |
| 02 | [dns-resolver-server](03-networking/02-dns-resolver-server) | 🟧 | DNS wire format, iterative resolution, cache |
| 03 | [dhcp-client-server](03-networking/03-dhcp-client-server) | 🟧 | `AF_PACKET`, the DORA exchange, leases |
| 04 | [c10k-http-server](03-networking/04-c10k-http-server) | 🟧 | `epoll` ET, non-blocking, `sendfile` |
| 05 | [io-uring-server](03-networking/05-io-uring-server) | 🟥 | `io_uring` SQ/CQ rings, multishot |
| 06 | [reverse-proxy-lb](03-networking/06-reverse-proxy-lb) | 🟧 | `splice`, backend selection, health checks |
| 07 | [redis-clone](03-networking/07-redis-clone) | 🟥 | RESP, event loop, dict, RDB/AOF, pub/sub |
| 08 | [packet-sniffer](03-networking/08-packet-sniffer) | 🟧 | `AF_PACKET`, `PACKET_MMAP`, BPF filters |
| 09 | [mini-wireguard-vpn](03-networking/09-mini-wireguard-vpn) | 🟥 | TUN, ChaCha20-Poly1305, Curve25519 |
| 10 | [port-scanner](03-networking/10-port-scanner) | 🟧 | raw sockets, `IP_HDRINCL`, SYN scan |
| 11 | [userspace-nic-driver](03-networking/11-userspace-nic-driver) | 🟥 | VFIO, MMIO, DMA descriptor rings (ixy-style) |
| 12 | [work-stealing-threadpool](03-networking/12-work-stealing-threadpool) | 🟧 | per-thread deques, work stealing |
| 13 | [lockfree-structures](03-networking/13-lockfree-structures) | 🟥 | CAS, Michael-Scott queue, hazard pointers |
| 14 | [raft-consensus](03-networking/14-raft-consensus) | 🟥 | leader election, log replication, snapshots |

### 4. Security, reverse engineering & pure assembly  — `04-security-asm/`
| # | Project | Diff | What it teaches |
|---|---------|:----:|-----------------|
| 01 | [nolibc-programs](04-security-asm/01-nolibc-programs) | 🟩 | raw `syscall`, own `_start`, the ABI **(reference)** |
| 02 | [simd-primitives](04-security-asm/02-simd-primitives) | 🟧 | SSE/AVX2 `memcpy`/`strlen`/`memchr`/utf-8 |
| 03 | [hw-crypto](04-security-asm/03-hw-crypto) | 🟧 | AES-NI, SHA-NI, constant-time fallback |
| 04 | [mini-libc](04-security-asm/04-mini-libc) | 🟧 | CRT startup, `auxv`/TLS, `printf`, syscalls |
| 05 | [x86-64-disassembler](04-security-asm/05-x86-64-disassembler) | 🟥 | REX/ModR-M/SIB decoding, opcode maps |
| 06 | [assembler](04-security-asm/06-assembler) | 🟧 | encode instructions, emit a relocatable ELF |
| 07 | [linker](04-security-asm/07-linker) | 🟥 | section layout, relocations, GOT/PLT |
| 08 | [crackme-keygen](04-security-asm/08-crackme-keygen) | 🟧 | static+dynamic RE, anti-debug, patching |
| 09 | [shellcode](04-security-asm/09-shellcode) | 🟧 | PIC `execve("/bin/sh")`, null-free, XOR encoder |
| 10 | [memory-corruption-ladder](04-security-asm/10-memory-corruption-ladder) | 🟥 | smash → ret2libc → ROP → format string |
| 11 | [coverage-fuzzer](04-security-asm/11-coverage-fuzzer) | 🟥 | edge coverage, fork server, shm bitmap |
| 12 | [syscall-sandbox](04-security-asm/12-syscall-sandbox) | 🟧 | `seccomp-bpf` allowlist, `ptrace`, Landlock |

### 5. Capstones  — `05-capstones/`
| # | Project | Diff | What it combines |
|---|---------|:----:|------------------|
| 01 | [from-scratch-os](05-capstones/01-from-scratch-os) | 🟥 | boot → protected → long mode, GDT/IDT, paging, IRQs |
| 02 | [container-engine](05-capstones/02-container-engine) | 🟥 | namespaces + cgroups + seccomp + overlayfs + veth |
| 03 | [microvm-monitor](05-capstones/03-microvm-monitor) | 🟥 | KVM VMM + virtio-blk/net/console |
| 04 | [networked-database](05-capstones/04-networked-database) | 🟥 | storage + event-loop server + Raft + tracing |
| 05 | [language-runtime](05-capstones/05-language-runtime) | 🟥 | compiler + bytecode VM + JIT + GC + green threads |

---

## Repo conventions

See [`CONVENTIONS.md`](CONVENTIONS.md) for the coding, comment, README, and
assembly-annotation rules every project follows. In short: comment the *why*
exhaustively; commit the generated + annotated assembly for C; be honest when a
🟥 project ships a teaching core rather than a complete implementation.

## License

MIT — see [`LICENSE`](LICENSE).
