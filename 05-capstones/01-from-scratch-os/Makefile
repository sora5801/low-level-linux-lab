# =============================================================================
# Makefile — assemble the boot sector, build the freestanding kernel, fuse them
#            into a bootable disk image, and launch it under QEMU.
# =============================================================================
# PLATFORM: build & run on Linux or WSL2. You need a normal ELF toolchain
# (gcc or clang + GNU binutils ld/objcopy), coreutils `truncate`, and
# `qemu-system-x86_64`. On Debian/Ubuntu:
#     sudo apt install build-essential binutils qemu-system-x86 coreutils
#
# TARGETS:
#     make            build os.img (the bootable disk image)
#     make run        boot it in QEMU (VGA window + COM1 mirrored to your term)
#     make run-serial boot it headless (everything on the serial console)
#     make debug      boot stopped, waiting for gdb on :1234 (see README)
#     make asm        regenerate the committed teaching assembly for asm/demo.c
#     make clean
#
# The ONLY host-independent target is `make asm`; everything else needs the
# Linux/WSL toolchain above because it produces and boots real machine code.
# =============================================================================

# ---- toolchain (override on the command line, e.g. `make CC=clang`) ---------
CC      ?= gcc
LD      ?= ld
OBJCOPY ?= objcopy
QEMU    ?= qemu-system-x86_64

# The boot sector loads this many 512-byte sectors of kernel from disk. It MUST
# equal `.set KERNEL_SECTORS` in boot/boot.S; the build fails loudly if the
# kernel outgrows it (see the guard in the kernel.bin rule).
KERNEL_SECTORS := 64

# ---- freestanding kernel compile flags (the WHY of each is in the README) ---
#   -ffreestanding -nostdlib : no libc, no CRT — we ARE the runtime
#   -fno-builtin             : do not turn our mem*() loops into self-calls, and
#                              keep the compiler from assuming libc semantics
#   -mno-red-zone            : an interrupt would corrupt the SysV red zone
#   -mno-mmx -mno-sse*       : we never set up the FPU/SSE, so forbid those regs
#   -fno-pic -fno-pie        : link at a fixed address (0x10000), absolute addrs
#   -fno-stack-protector     : there is no __stack_chk_guard in a bare kernel
CFLAGS  ?= -std=gnu11 -ffreestanding -nostdlib -fno-builtin -fno-stack-protector \
           -fno-pic -fno-pie -fno-asynchronous-unwind-tables -fcf-protection=none \
           -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
           -Wall -Wextra -O2 -g -Ikernel
ASFLAGS ?= -fno-pic

# ld: -n (nmagic) packs sections without page-alignment padding, keeping the
# flat image compact; -T selects our layout script.
LDFLAGS ?= -m elf_x86_64 -n

BUILD := build

# Kernel objects. Note the two "isr" sources map to DISTINCT object names so the
# .S stubs and the .c dispatcher never collide.
KOBJS := $(BUILD)/entry.o $(BUILD)/kmain.o $(BUILD)/vga.o $(BUILD)/serial.o \
         $(BUILD)/pic.o $(BUILD)/idt.o $(BUILD)/isr.o $(BUILD)/isr_stubs.o \
         $(BUILD)/mem.o

# Force System V AMD64 (Linux) ABI when generating the teaching assembly, so it
# is identical regardless of the host you run `make asm` on.
ASM_TARGET ?= x86_64-pc-linux-gnu

.PHONY: all
all: os.img

$(BUILD):
	mkdir -p $(BUILD)

# ---- compile the C kernel translation units ---------------------------------
$(BUILD)/%.o: kernel/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# ---- assemble the kernel's asm pieces (distinct object names) ----------------
$(BUILD)/entry.o: kernel/entry.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@
$(BUILD)/isr_stubs.o: kernel/isr.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

# ---- link + flatten the kernel ----------------------------------------------
# Link to an ELF at 0x10000, then strip it to a raw binary the boot sector can
# splat into memory verbatim (no ELF headers to interpret in a 512-byte loader).
$(BUILD)/kernel.elf: $(KOBJS) linker.ld
	$(LD) $(LDFLAGS) -T linker.ld -o $@ $(KOBJS)

kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	@sz=$$(wc -c < $@); max=$$((512 * $(KERNEL_SECTORS))); \
	 if [ $$sz -gt $$max ]; then \
	   echo "ERROR: kernel.bin is $$sz bytes > $$max ($(KERNEL_SECTORS) sectors)."; \
	   echo "       Raise KERNEL_SECTORS in BOTH the Makefile and boot/boot.S."; \
	   rm -f $@; exit 1; \
	 fi; \
	 truncate -s $$max $@   # pad with zeros so the boot read always finds data

# ---- assemble + flatten the boot sector -------------------------------------
$(BUILD)/boot.o: boot/boot.S | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@
boot.bin: $(BUILD)/boot.o boot/boot.ld
	$(LD) -m elf_x86_64 -T boot/boot.ld -o $(BUILD)/boot.elf $(BUILD)/boot.o
	$(OBJCOPY) -O binary $(BUILD)/boot.elf $@
	@sz=$$(wc -c < $@); \
	 if [ $$sz -ne 512 ]; then echo "ERROR: boot.bin is $$sz bytes, not 512"; exit 1; fi

# ---- the bootable disk image: MBR sector followed by the kernel -------------
os.img: boot.bin kernel.bin
	cat boot.bin kernel.bin > $@
	@echo "built os.img ($$(wc -c < $@) bytes) — try:  make run"

# ---- run it -----------------------------------------------------------------
# -drive ...raw : present os.img as the first hard disk (BIOS boots its MBR)
# -serial stdio : wire the guest's COM1 to this terminal (our serial_puts log)
# -no-reboot/-no-shutdown : on a triple fault, freeze instead of looping, so you
#                           can actually read the panic.
.PHONY: run
run: os.img
	$(QEMU) -drive format=raw,file=os.img -serial stdio -no-reboot -no-shutdown

# Headless: fold VGA + serial into the terminal (good over SSH / in CI logs).
.PHONY: run-serial
run-serial: os.img
	$(QEMU) -drive format=raw,file=os.img -nographic -no-reboot -no-shutdown

# Boot halted, waiting for gdb: `target remote :1234` then `set arch i386:x86-64`.
.PHONY: debug
debug: os.img
	$(QEMU) -drive format=raw,file=os.img -serial stdio -no-reboot -no-shutdown -s -S

# ---- regenerate the committed teaching assembly for asm/demo.c --------------
# EXACTLY the three commands the project spec mandates. The hand-written
# asm/demo.annotated.s is authored, never overwritten here.
.PHONY: asm
asm:
	clang --target=$(ASM_TARGET) -S -O0 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.O0.s
	clang --target=$(ASM_TARGET) -S -O1 -fno-asynchronous-unwind-tables -fno-jump-tables -fno-omit-frame-pointer asm/demo.c -o asm/demo.s
	clang --target=$(ASM_TARGET) -S -O2 -fno-asynchronous-unwind-tables -fno-jump-tables asm/demo.c -o asm/demo.O2.s
	@echo "regenerated asm/demo.{O0.s,s,O2.s}; asm/demo.annotated.s is hand-written"

.PHONY: clean
clean:
	rm -rf $(BUILD) os.img boot.bin kernel.bin
