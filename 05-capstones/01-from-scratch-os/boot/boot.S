/* =============================================================================
 * boot.S — a 512-byte MBR that takes an x86 PC from 16-bit real mode all the
 *          way to a 64-bit long-mode C kernel. THE star annotated artifact.
 * =============================================================================
 *
 * HOW TO READ THIS FILE
 * ---------------------
 * This is hand-written GNU-assembler (AT&T) source, NOT compiler output, and it
 * is commented on essentially every instruction because the mode transitions it
 * performs are the whole lesson of a from-scratch OS. AT&T syntax means:
 *
 *     op  src, dst           # movw $0, %ax   =>  ax = 0
 *     %reg  register   $imm immediate   label(%reg) memory   $label its address
 *
 * The file is assembled ONCE but changes CPU mode THREE times, and we tell the
 * assembler which instruction encoding to emit with `.code16 / .code32 /
 * .code64`. The bytes for `mov`, `jmp`, etc. differ per mode; these directives
 * are how one source file speaks all three dialects.
 *
 * THE JOURNEY (why each step is mandatory, in order)
 * --------------------------------------------------
 *   [16-bit REAL MODE]  BIOS drops us here at physical 0x7C00. 1 MiB of address
 *      space, segment:offset addressing, BIOS interrupts available. We use one
 *      (INT 13h) to read the rest of the kernel off the disk, because the BIOS
 *      only loaded THIS sector. Then we leave real mode forever.
 *   [32-bit PROTECTED]  Set CR0.PE and load a flat GDT so segments become
 *      base-0/limit-4GiB "just get out of my way" descriptors. A far jump
 *      reloads CS with a 32-bit code descriptor — the ONLY way to change CS.
 *      We are here only briefly: long mode is reached FROM protected mode.
 *   [64-bit LONG MODE]  Build 4-level page tables (paging is MANDATORY in long
 *      mode — you cannot enter it with paging off), turn on PAE, set EFER.LME,
 *      then CR0.PG. A second far jump into a 64-bit code descriptor makes the
 *      CPU 64 bits wide. We hand control to the C kernel at 0x10000.
 *
 * MEMORY MAP we impose:
 *   0x00000000..0x000004FF  real-mode IVT + BIOS data (leave alone)
 *   0x00001000..0x00003FFF  our page tables: PML4 @0x1000, PDPT @0x2000, PD @0x3000
 *   0x00007C00..0x00007DFF  this boot sector (512 bytes)
 *   below 0x7C00            real-mode stack (grows down)
 *   0x00010000..            the kernel image, loaded from disk, run in long mode
 *   0x000B8000              VGA text buffer (the kernel writes here)
 * ============================================================================= */

	.set KERNEL_SECTORS, 64        /* how many 512-byte sectors of kernel to load.
	                                * MUST match the Makefile's padding of kernel.bin. */
	.set KERNEL_SEG,     0x1000    /* load segment: 0x1000:0x0000 == phys 0x10000  */
	.set PML4_ADDR,      0x1000    /* physical base of the top-level page table     */

	.text
	.code16                        /* the CPU starts in 16-bit real mode            */
	.globl realmode_start
realmode_start:
	/* BIOS jumps to us with CS:IP = 0000:7C00 (SeaBIOS/QEMU) but that is not
	 * guaranteed across BIOSes, so normalize CS to 0 with a far jump to a known
	 * label. A far jump is the only instruction that reloads CS. */
	cli                            /* no interrupts while our state is half-built   */
	ljmp $0x0000, $flush_cs        /* CS <- 0, IP <- flush_cs (a linear 0x7Cxx addr)*/
flush_cs:
	xor  %ax, %ax                  /* AX = 0, the segment value we want everywhere  */
	mov  %ax, %ds                  /* DS=0: data references become plain linear addrs*/
	mov  %ax, %es                  /* ES=0: INT 13h uses ES:DI for some calls        */
	mov  %ax, %ss                  /* SS=0: flat stack segment                       */
	mov  $0x7C00, %sp              /* stack top just below us; it grows DOWN from here*/

	/* The BIOS passes the drive we booted from in DL. Save it before anything
	 * can clobber it — the disk read below needs it. */
	mov  %dl, boot_drive

	/* ---- Enable the A20 line -------------------------------------------------
	 * For historical 8086-compatibility the 21st address line (A20) is gated
	 * OFF at power-on, so addresses wrap at 1 MiB. We must ungate it or every
	 * access above 1 MiB silently aliases low memory. The "fast A20" method:
	 * set bit 1 of port 0x92. Bit 0 of that port is a FAST CPU RESET — we mask
	 * it off first so we do not accidentally reboot the machine. */
	in   $0x92, %al
	or   $0x02, %al                /* bit 1 = enable A20                            */
	and  $0xFE, %al                /* bit 0 = fast reset; keep it CLEAR             */
	out  %al, $0x92

	/* ---- Load the kernel off the disk ---------------------------------------
	 * The BIOS loaded only THIS 512-byte sector. The kernel lives in the very
	 * next sectors (LBA 1 onward). We read them with INT 13h function 42h
	 * (extended read), which takes a "Disk Address Packet" (DAP) by DS:SI and,
	 * unlike the old CHS function 02h, is geometry-independent — far more robust.
	 * BIOS disk services may use IRQs internally, so allow interrupts for the call. */
	sti
	mov  $dap, %si                 /* DS:SI -> our DAP describing the transfer      */
	mov  $0x42, %ah                /* AH=42h: extended read sectors                 */
	mov  boot_drive, %dl           /* DL = the drive to read from                   */
	int  $0x13                     /* BIOS reads KERNEL_SECTORS sectors -> 0x10000  */
	jc   disk_error                /* carry set = read failed; bail out loudly      */
	cli                            /* done with the BIOS; no interrupts from here on*/

	/* ---- Enter 32-bit protected mode ----------------------------------------
	 * Load our Global Descriptor Table, then flip CR0.PE (Protection Enable).
	 * The instant PE is set the CPU interprets segment selectors as GDT indices,
	 * but the CURRENTLY cached CS descriptor is still the real-mode one — so we
	 * MUST immediately far-jump to reload CS from the GDT. That jump is the
	 * literal boundary between real and protected mode. */
	lgdt gdtdesc                   /* GDTR <- (limit,base) of our flat GDT          */
	mov  %cr0, %eax
	or   $0x1, %eax                /* set PE (bit 0)                                */
	mov  %eax, %cr0
	ljmp $0x08, $pm_entry          /* far jump: CS <- 0x08 (32-bit code) -> 32-bit  */

/* ------------------------------------------------------------------------- */
	.code32                        /* everything below executes as 32-bit code      */
pm_entry:
	/* Reload the data segment registers with the 32-bit flat DATA selector so
	 * memory accesses use the base-0/limit-4GiB descriptor. In protected mode a
	 * segment register holds a SELECTOR (a GDT index), not a paragraph address. */
	mov  $0x10, %ax                /* 0x10 = the DATA descriptor selector           */
	mov  %ax, %ds
	mov  %ax, %es
	mov  %ax, %ss
	mov  %ax, %fs
	mov  %ax, %gs
	mov  $0x7C00, %esp             /* a valid 32-bit stack (unused, but well-formed)*/

	/* ---- Build 4-level page tables (identity map the first 1 GiB) ------------
	 * Long mode REQUIRES paging, so before we can switch we must have a valid
	 * PML4 -> PDPT -> PD hierarchy. We map virtual == physical for the low 1 GiB
	 * using 2 MiB "huge" pages, which is enough to cover the kernel (0x10000),
	 * the page tables themselves, and the VGA buffer (0xB8000) without a fourth
	 * (PT) level. First zero the three table pages so every unused entry is a
	 * clean "not present". */
	mov  $PML4_ADDR, %edi          /* EDI = 0x1000, the start of table memory       */
	xor  %eax, %eax                /* fill value 0                                  */
	mov  $0x1000, %ecx             /* 0x1000 dwords = 16 KiB = 4 pages, zeroed       */
	rep  stosl                     /* memset(0x1000, 0, 0x4000)                     */

	/* PML4[0] -> PDPT at 0x2000, flags Present|Writable (0x3). The high 32 bits
	 * of the entry are already 0 from the memset, so we write only the low half. */
	movl $0x2003, PML4_ADDR        /* *(0x1000) = 0x2000 | P | RW                    */
	/* PDPT[0] -> PD at 0x3000, Present|Writable. */
	movl $0x3003, 0x2000           /* *(0x2000) = 0x3000 | P | RW                    */

	/* PD[0..511]: 512 entries, each a 2 MiB page frame with PS(bit7)|RW|P = 0x83.
	 * Entry i maps physical i*2MiB. 512 * 2 MiB = 1 GiB identity-mapped. */
	mov  $0x3000, %edi             /* EDI -> PD[0]                                   */
	mov  $0x83, %eax               /* first frame: phys 0 | PS | RW | P             */
	mov  $512, %ecx                /* 512 entries to write                          */
fill_pd:
	movl %eax, (%edi)              /* low 32 bits of the PD entry (addr | flags)    */
	movl $0, 4(%edi)               /* high 32 bits = 0 (phys < 4 GiB, NX unused)    */
	add  $0x200000, %eax           /* advance the physical frame by 2 MiB           */
	add  $8, %edi                  /* next 8-byte entry                             */
	loop fill_pd                   /* ECX--, repeat until all 512 written           */

	/* ---- Turn on long mode --------------------------------------------------
	 * The canonical four-step handshake. Order matters: CR3 must point at the
	 * PML4 before paging is enabled; PAE and LME must be set before CR0.PG, or
	 * enabling paging would give plain 32-bit paging instead of long mode. */
	mov  $PML4_ADDR, %eax
	mov  %eax, %cr3                /* CR3 = physical base of the PML4                */

	mov  %cr4, %eax
	or   $0x20, %eax               /* CR4.PAE (bit 5): 64-bit page-table format      */
	mov  %eax, %cr4

	mov  $0xC0000080, %ecx         /* ECX = the EFER model-specific register number */
	rdmsr                          /* EDX:EAX <- EFER                               */
	or   $0x100, %eax              /* EFER.LME (bit 8): Long Mode Enable            */
	wrmsr                          /* EFER <- EDX:EAX                               */

	mov  %cr0, %eax
	or   $0x80000000, %eax         /* CR0.PG (bit 31): enable paging -> ACTIVATE LME */
	mov  %eax, %cr0                /* the CPU is now in "compatibility" sub-mode     */

	/* One more far jump, this time to a code descriptor with the Long (L) bit
	 * set. THIS is what makes the CPU 64 bits wide; until now we were running
	 * 32-bit code with long mode merely "enabled". */
	ljmp $0x18, $long_entry        /* CS <- 0x18 (64-bit code) -> true 64-bit mode  */

/* ------------------------------------------------------------------------- */
	.code64                        /* finally: 64-bit instruction encodings         */
long_entry:
	/* Reload the data selectors once more. In long mode the CPU largely IGNORES
	 * segment bases/limits for DS/ES/SS, but the registers should still hold a
	 * valid (non-conflicting) selector; we use the DATA descriptor. */
	mov  $0x10, %ax
	mov  %ax, %ds
	mov  %ax, %es
	mov  %ax, %ss
	mov  %ax, %fs
	mov  %ax, %gs

	/* Jump to the kernel we loaded at physical 0x10000. It is identity-mapped
	 * (our PD covers 0..1 GiB) and executable (we set no NX bit), so a plain
	 * indirect jump transfers control to the kernel's _start (entry.S). */
	mov  $0x10000, %eax            /* EAX = 0x10000; writing EAX zero-extends RAX   */
	jmp  *%rax                     /* -> kernel/_start; we never come back          */

/* ------------------------------------------------------------------------- */
	.code16                        /* the error path runs before any mode switch    */
disk_error:
	/* INT 13h failed. Print 'E' via BIOS teletype (INT 10h, AH=0Eh) and hang so
	 * the failure is visible rather than a silent reboot. */
	mov  $'E', %al
	mov  $0x0E, %ah
	int  $0x10
.halt:
	cli
	hlt
	jmp  .halt

/* ============================ read-only data ============================== */

boot_drive:
	.byte 0                        /* filled in at runtime from DL                  */

	.align 4
dap:                               /* INT 13h Disk Address Packet (16 bytes)        */
	.byte 0x10                     /*  +0  size of this packet                      */
	.byte 0x00                     /*  +1  reserved, 0                              */
	.word KERNEL_SECTORS           /*  +2  number of sectors to transfer            */
	.word 0x0000                   /*  +4  destination offset  (0x1000:0x0000 ...)  */
	.word KERNEL_SEG               /*  +6  destination segment (... = phys 0x10000) */
	.quad 1                        /*  +8  starting LBA = sector 1 (just after MBR) */

	/* ---- The Global Descriptor Table ---------------------------------------
	 * Four 8-byte descriptors. In our "flat" model every code/data descriptor
	 * has base 0 and limit 4 GiB, so segmentation effectively disappears and
	 * only the type/mode bits matter. Selector = byte offset into this table:
	 * null=0x00, code32=0x08, data=0x10, code64=0x18. */
	.align 8
gdt:
	.quad 0                        /* 0x00: the mandatory null descriptor           */
gdt_code32:                        /* 0x08: 32-bit ring-0 code, base 0, limit 4GiB  */
	.word 0xFFFF                   /*   limit bits 0..15                            */
	.word 0x0000                   /*   base  bits 0..15                            */
	.byte 0x00                     /*   base  bits 16..23                           */
	.byte 0x9A                     /*   access: P=1,DPL=0,S=1,type=code exec/read   */
	.byte 0xCF                     /*   flags G=1,D=1(32-bit) | limit bits 16..19=F */
	.byte 0x00                     /*   base  bits 24..31                           */
gdt_data:                          /* 0x10: ring-0 data, base 0, limit 4GiB         */
	.word 0xFFFF
	.word 0x0000
	.byte 0x00
	.byte 0x92                     /*   access: P=1,DPL=0,S=1,type=data read/write  */
	.byte 0xCF                     /*   flags G=1,D=1 | limit 16..19                */
	.byte 0x00
gdt_code64:                        /* 0x18: 64-bit ring-0 code (the Long bit is set)*/
	.word 0xFFFF
	.word 0x0000
	.byte 0x00
	.byte 0x9A                     /*   access: code exec/read, present, ring 0     */
	.byte 0xAF                     /*   flags G=1,L=1(64-bit),D=0 | limit 16..19=F  */
	.byte 0x00
gdt_end:

gdtdesc:                           /* the 6-byte operand for `lgdt`                 */
	.word gdt_end - gdt - 1        /*   limit = table size minus one                */
	.long gdt                      /*   base  = linear address of the table         */

/* ---------------------------------------------------------------------------
 * The BIOS only treats a sector as bootable if its last two bytes are 0x55,0xAA.
 * The linker script (boot.ld) pads this section to offset 510 and appends that
 * signature, so we do NOT emit it here — keeping the padding math in one place.
 * ------------------------------------------------------------------------- */
