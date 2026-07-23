# A dynamic linker / loader 🟥

**What it is.** A working, from-scratch **ELF loader and dynamic linker** for
x86-64 Linux — the job the kernel normally hands to
`/lib64/ld-linux-x86-64.so.2`. You invoke it like the real `ld.so`:

```bash
./loader ./test/prog arg1 arg2
```

and it maps the program into memory, loads its shared libraries, applies the
relocations that wire up the GOT and PLT, runs the constructors, builds a fresh
initial stack, and jumps to the entry point. It has **no libc of its own** —
which is not a stunt but a necessity: a dynamic linker is the thing that *sets
libc up*, so it cannot depend on it. (The happy side effect is that `loader.c`
compiles straight to Linux assembly, so the committed `asm/loader.s` is this
real code, not a toy.)

This is a 🟥 giant shipped as an **honest teaching core**. See
[Scope](#scope-what-works-and-what-doesnt) for exactly what it does and does not
do.

## What you'll learn

- **ELF loading**: parsing the ELF header and program headers; `mmap`ing each
  `PT_LOAD` segment at the right address with the right protections
  (`PROT_READ/WRITE/EXEC`); zeroing the `.bss` tail; the "reserve the whole span
  once, then `MAP_FIXED` each segment" trick that fixes a PIE's **load bias**.
- **PT_DYNAMIC**: walking the `Elf64_Dyn` array to find `DT_RELA`, `DT_JMPREL`,
  `DT_SYMTAB`, `DT_STRTAB`, `DT_HASH`/`DT_GNU_HASH`, `DT_NEEDED`,
  `DT_INIT_ARRAY`, `DT_RELR`, and friends.
- **Relocations**: applying `R_X86_64_RELATIVE` (`B+A`), `R_X86_64_GLOB_DAT` and
  `R_X86_64_JUMP_SLOT` (`S+A`), `R_X86_64_64`, `R_X86_64_IRELATIVE` (call a
  resolver), `R_X86_64_COPY`, and the compact `DT_RELR` encoding. **This is the
  core** and is the subject of the annotated assembly.
- **Symbol resolution & the GOT/PLT**: how a `JUMP_SLOT` becomes a function
  address, how `GLOB_DAT` gives code an indirect handle on a data symbol, and
  why **first-definition-wins** across the load order is exactly what makes
  `LD_PRELOAD` interposition work.
- **auxv & process start-up**: the shape of the initial stack the kernel builds
  (`argc`, `argv`, `envp`, `auxv`), which auxv entries (`AT_PHDR`, `AT_ENTRY`,
  `AT_BASE`, `AT_EXECFN`, …) the loader must patch, and the exact register state
  at the `_start` hand-off (`rsp`→`argc`, `rdx`=0, 16-byte alignment).
- **Hardening**: applying `PT_GNU_RELRO` (making the GOT read-only *after*
  relocation) — the runtime half of RELRO/BIND_NOW.
- Syscalls used directly: `openat(2)`, `pread64(2)`, `mmap(2)`, `mprotect(2)`,
  `close(2)`, `write(2)`, `exit_group(2)`.

## Build & run (Linux / WSL only)

You need a Linux-targeting `clang` (or `gcc`) with `lld`/`ld`.

```bash
make            # builds ./loader  (nolibc, static, no-pie)
make test       # builds the three test targets and runs them under ./loader
```

`make test` prints something like:

```
== 1) static-PIE, no symbols, just R_X86_64_RELATIVE/RELR ==
[hello-pie] constructor ran before entry
Hello from a static-PIE, loaded by hand!

== 2) dynamic exe + libgreet.so: JUMP_SLOT/GLOB_DAT/init_array ==
[greetlib] constructor ran (DT_INIT_ARRAY)
[prog] constructor ran (DT_INIT_ARRAY)
[prog] running under the toy dynamic linker
Hello from greetlib.so!
name = dynamic-linker-lab
answer = 42
```

Watch every step the loader takes:

```bash
LDLAB_DEBUG=1 ./loader ./test/hello-pie
LDLAB_LIBRARY_PATH=test LDLAB_DEBUG=1 ./loader ./test/prog
```

Inspect what you're loading with the real tools:

```bash
readelf -l test/prog        # program headers: PT_LOAD, PT_DYNAMIC, PT_GNU_RELRO
readelf -d test/prog        # dynamic section: NEEDED libgreet.so, RELA, JMPREL
readelf -r test/prog        # the relocations our loader applies
```

## How it works (file by file)

- **`loader.c`** — the whole loader, built bottom-up:
  - *Syscall layer* (`syscall6` + typed wrappers): raw `syscall` instructions,
    because there is no libc to call. Every return value is checked.
  - *Freestanding helpers*: our own `memset`/`memcpy` (with a `volatile`
    destination so the optimizer can't rewrite them into a call to themselves),
    `kstrlen`, `kstreq`, plus `die()`/`trace()` diagnostics straight to fd 2.
  - *`map_object`* — computes the `[minva, maxva)` span of all `PT_LOAD`s,
    reserves it with one `PROT_NONE` `mmap` (which also picks the PIE load
    address and thus the **bias**), then `MAP_FIXED`-maps each segment from the
    file with real protections and extends `.bss` with anonymous pages.
  - *`parse_dynamic`* — turns `PT_DYNAMIC` into a `struct obj` with every table
    pointer already bias-adjusted.
  - *`global_lookup` / `lookup_in`* — resolve a symbol by scanning every loaded
    object in load order; first definition wins (interposition), strong beats
    weak, `STT_GNU_IFUNC` is resolved by *calling* it.
  - *`apply_rela` / `apply_relr`* — **the relocation engine**: the `S+A` / `B+A`
    switch that writes GOT and PLT slots.
  - *`load_needed`* — finds and recursively loads `DT_NEEDED` libraries by
    soname (searching `DT_RUNPATH`, `$LDLAB_LIBRARY_PATH`, the program's dir,
    then `.`).
  - *`apply_relro`* — `mprotect`s the `PT_GNU_RELRO` region read-only.
  - *`run_init`* — runs `DT_INIT` then `DT_INIT_ARRAY`, dependencies first.
  - *`handoff` / `enter_program`* — builds the child's `argc/argv/envp/auxv`
    block on a fresh `mmap`ed stack and jumps to the entry with a clean ABI
    register state.
  - *`_start`* — module-level asm: grab `rsp`, 16-align it, call `loader_main`.
- **`test/hello-pie.c`** — a freestanding **static-PIE**: no dependencies, but a
  global pointer forces an `R_X86_64_RELATIVE`, and a constructor exercises
  `DT_INIT_ARRAY`. The simplest thing the loader runs end to end.
- **`test/greetlib.c`** — a freestanding **shared object** exporting a function
  (`JUMP_SLOT`), a data object (`GLOB_DAT`), and an internal relative pointer.
- **`test/prog.c`** — a freestanding **dynamic executable** that `DT_NEEDED`s
  `libgreet.so` and uses one symbol of each shape.

## Assembly notes

The annotated file, **[`asm/demo.annotated.s`](asm/demo.annotated.s)**, walks
`apply_relocations()` — the relocation inner loop from `asm/demo.c` — one
instruction at a time. The lesson it makes visible:

- **Every relocation is one store**: `*(base + r_offset) = V`, where `V` is
  `B+A`, `S+A`, or a resolver's return value. A dynamic linker *is* that loop
  plus the bookkeeping to find `S`.
- clang **merged** the `R_X86_64_64`, `GLOB_DAT`, and `JUMP_SLOT` cases into a
  single code path because their arithmetic (`S+A`) is identical.
- The symbol's `st_value` is fetched with `leaq (%rsi,%rsi,2)` — strength
  reduction of `idx * sizeof(Elf64_Sym)` (24 = 3 qwords).
- `symtab` is **spilled and reloaded** around the IFUNC `callq *%rax`, a
  correct-ABI detail (the resolver may clobber `rcx`) you can see in the asm but
  never in the C.

Because `loader.c` itself is header-free, its real assembly is committed too:
**[`asm/loader.s`](asm/loader.s)**. Look for `_start` at the top, the many raw
`syscall` instructions, and — in `handoff` — the four-instruction transfer
trampoline that is the loader's finale:

```asm
movq %rsi, %rsp     # switch to the program's stack
xorl %edx, %edx     # rdx = 0: no rtld_fini
xorl %ebp, %ebp     # end the frame chain
jmpq *%rdi          # jump to the entry point — never returns
```

Regenerate everything with `make asm` (works on any host — clang cross-targets
Linux). Compare `asm/demo.O0.s` (naive, one C statement at a time) with
`asm/demo.s` (`-O1`, annotated) and `asm/demo.O2.s` (tighter scheduling).

## Scope: what works, and what doesn't

**Fully working teaching core:**

- **static-PIE** executables (map, apply `RELATIVE`/`RELR`/`IRELATIVE`, run
  constructors, hand off).
- **Simple dynamic** executables that depend on the *freestanding* shared
  objects in `test/`: recursive `DT_NEEDED` loading, symbol resolution across
  the load set, `GLOB_DAT`/`JUMP_SLOT`/`64`/`COPY` relocations, `PT_GNU_RELRO`,
  and `DT_INIT`/`DT_INIT_ARRAY`.
- **Eager binding** (BIND_NOW-style): every `JUMP_SLOT` is resolved at load
  time.

**Deliberately not covered** (each is its own rabbit hole; the code detects and
reports these rather than silently misbehaving):

- **Thread-Local Storage** — `R_X86_64_DTPMOD64/DTPOFF64/TPOFF64` need a TCB, a
  DTV, per-module TLS blocks, and `%fs` set up via `arch_prctl`. The loader
  `die()`s clearly on a TLS relocation.
- **Symbol versioning** — `DT_VERSYM`/`DT_VERNEED` are ignored; we bind to the
  base symbol name. Fine for our test objects, not for versioned glibc symbols.
- **Lazy PLT binding** — we bind eagerly. Real lazy binding leaves each
  `JUMP_SLOT` pointing at a PLT stub that pushes a relocation index and jumps to
  `_dl_runtime_resolve`, which resolves the symbol, patches the GOT, and tail-
  calls the target. See *Going further*.
- **Loading real glibc** — it needs all of the above (TLS especially), so this
  loader targets its own self-contained test objects, not `/lib/libc.so.6`.

## Going further (the `Stretch:` from the list)

- **`dlopen`/`dlsym`.** Add a runtime API: `dlopen` runs the same
  load→relocate→init pipeline on a new object and appends it to the link map;
  `dlsym` is `global_lookup` restricted to that object's dependency scope.
  Reference counting and `dlclose` (running `DT_FINI_ARRAY`, unmapping) is the
  fiddly part.
- **`LD_PRELOAD` interposition ordering.** Our first-definition-wins scan
  *already* implements interposition — a preloaded object simply has to appear
  earlier in the load order. Parse `LD_PRELOAD`, load those objects immediately
  after the executable and before its `DT_NEEDED`s, and preloaded `malloc`/`free`
  will shadow the library's. The subtlety production ld.so handles is that the
  *executable's* own definitions still win over preloads for `COPY`-relocated
  data.
- **Lazy binding for real.** Point each `JUMP_SLOT` at `base + PLT stub`,
  implement `_dl_runtime_resolve` in asm (save the argument registers, call a C
  resolver with the reloc index, restore, `jmp` to the resolved address), and
  wire `GOT[1]`/`GOT[2]` (the link-map pointer and the resolver entry) the way
  the psABI specifies.
- **What production does.** glibc's `elf/rtld.c` bootstraps by relocating
  *itself* first (it is a PIE too), then `dl-load.c`/`dl-reloc.c` do the general
  case with versioning, TLS, and `ld.so.cache` lookups.

## References

- System V AMD64 psABI — relocation types, the `_start` stack layout, auxv.
- *Linkers & Loaders*, John R. Levine — the canonical tour.
- glibc source: `elf/rtld.c`, `elf/dl-load.c`, `elf/dl-reloc.c`,
  `sysdeps/x86_64/dl-machine.h` (the `elf_machine_rela` switch this project
  mirrors), `sysdeps/x86_64/dl-trampoline.S` (`_dl_runtime_resolve`).
- musl `ldso/dynlink.c` — a far more readable complete dynamic linker.
- `man 5 elf`, `man 3 dlopen`, `man 8 ld.so`.
