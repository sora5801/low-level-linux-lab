/* ===========================================================================
 * kmain.c — the C kernel's entry point: bring the machine to life, then idle.
 * ===========================================================================
 *
 * By the time we get here the boot sector has already done the hard part: we
 * are in 64-bit long mode, the first 1 GiB is identity-mapped, a flat GDT is
 * loaded, and entry.S has given us a stack and called us. Everything the CPU
 * needs to RUN is set up. What is NOT set up is everything the machine needs to
 * be USEFUL: a console, a way to receive interrupts, and the two devices
 * (timer, keyboard) that make the kernel interactive. That is this file's job,
 * in the exact order the hardware demands:
 *
 *     serial + VGA  ->  we can print (debugging is possible from here on)
 *     PIC remap     ->  IRQs will arrive at safe vectors (32..47), not clash
 *     IDT load      ->  the CPU knows where each handler is
 *     PIT program   ->  the timer starts ticking at ~100 Hz
 *     unmask + sti  ->  interrupts are finally allowed to fire
 *     hlt loop      ->  sleep the CPU; wake only to service an interrupt
 *
 * ORDER MATTERS: we must remap the PIC and load the IDT BEFORE `sti`, or the
 * first timer tick would dispatch through a bogus/empty vector and fault.
 * =========================================================================== */
#include "types.h"
#include "io.h"
#include "vga.h"
#include "serial.h"
#include "pic.h"
#include "idt.h"

/* Print to both consoles at once (VGA for the screen, serial for the log). */
static void kputs(const char *s) { vga_puts(s); serial_puts(s); }

/* ---------------------------------------------------------------------------
 * pit_init — program the 8253/8254 Programmable Interval Timer, channel 0, to
 * fire IRQ0 at `hz` interrupts per second.
 *
 * The PIT counts down from a 16-bit divisor at a fixed 1193182 Hz input clock;
 * each time it reaches zero it raises IRQ0 and reloads. So divisor = base/hz.
 * We select channel 0, access mode "lobyte then hibyte", mode 3 (square wave)
 * via the command byte 0x36, then write the divisor's two halves to port 0x40.
 * ------------------------------------------------------------------------- */
static void pit_init(uint32_t hz)
{
    uint32_t divisor = 1193182u / hz;
    outb(0x43, 0x36);                          /* channel 0, lo/hi, mode 3      */
    outb(0x40, (uint8_t)(divisor & 0xFF));     /* low byte of the reload value  */
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF)); /* high byte                  */
}

/* The kernel entry proper. `void` in, never returns. Declared here; entry.S
 * calls it by name. */
void kmain(void)
{
    /* 1. Consoles first so every later step can announce itself / a fault can
     *    be reported. serial_init programs COM1; vga_init clears the screen. */
    serial_init();
    vga_init();
    vga_set_color(VGA_LGREEN, VGA_BLACK);
    kputs("from-scratch-os: reached 64-bit C kernel (long mode active)\n");
    vga_set_color(VGA_LGREY, VGA_BLACK);
    kputs("  [ok] serial COM1 @ 38400 8N1, VGA text @ 0xB8000\n");

    /* 2. Relocate the PICs off the CPU's reserved exception vectors. Until this
     *    runs, IRQ0 would arrive as vector 8 (= #DF). */
    pic_remap();
    kputs("  [ok] 8259 PIC remapped to vectors 0x20..0x2F\n");

    /* 3. Install the IDT so the CPU can find our handlers, then it is safe to
     *    let interrupts happen. */
    idt_init();
    kputs("  [ok] IDT loaded (256 gates: 32 exceptions + 16 IRQs)\n");

    /* 4. Start the timer and enable only the two IRQ lines we service. Every
     *    other line stays masked so a spurious device cannot surprise us. */
    pit_init(100);                 /* 100 Hz -> a tick every 10 ms              */
    pic_clear_mask(0);             /* unmask IRQ0 (timer)                       */
    pic_clear_mask(1);             /* unmask IRQ1 (keyboard)                    */
    kputs("  [ok] PIT @ 100 Hz; IRQ0/IRQ1 unmasked\n");

    /* 5. THE moment interrupts go live: set IF in RFLAGS. From here the timer
     *    and keyboard handlers run asynchronously between our `hlt`s. */
    __asm__ volatile("sti");
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    kputs("\ninterrupts enabled — type on the keyboard; timer prints each second\n");
    vga_set_color(VGA_LGREY, VGA_BLACK);

    /* 6. The idle loop. `hlt` stops the CPU clock until the NEXT interrupt,
     *    which is the correct, power-friendly way to "wait for work" — a busy
     *    spin would just cook the core. Each timer/keyboard IRQ wakes us, runs
     *    its handler, `iretq`s back here, and we `hlt` again. This loop is where
     *    a real kernel's scheduler would instead pick the next task to run. */
    for (;;)
        __asm__ volatile("hlt");
}
