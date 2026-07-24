/* ===========================================================================
 * isr.c — the C side of interrupt handling: classify, service, acknowledge.
 * ===========================================================================
 *
 * isr.S funnels every vector here through isr_dispatch(&regs). Our job:
 *   - vectors  0..31  : a CPU exception. For a teaching kernel we print the
 *                       vector + error code and HALT — a fault means a bug, and
 *                       continuing would just fault again (or triple-fault).
 *   - vectors 32..47  : a hardware IRQ. Service it (timer/keyboard), then send
 *                       the PIC its End-Of-Interrupt so the next one can arrive.
 *   - anything else   : unexpected; note it and move on.
 *
 * The keyboard handler decodes IRQ1 scancodes; the timer handler counts IRQ0
 * ticks. Both are deliberately tiny — the point is to prove the whole path
 * (PIC -> IDT gate -> asm stub -> here -> EOI -> iretq) works end to end.
 * =========================================================================== */
#include "idt.h"
#include "io.h"
#include "pic.h"
#include "vga.h"
#include "serial.h"

/* Names for the 32 architectural exceptions, indexed by vector, for readable
 * panic output. Kept in .rodata; only ever read. */
static const char *const EXC_NAME[32] = {
    "#DE divide-by-zero", "#DB debug", "NMI", "#BP breakpoint",
    "#OF overflow", "#BR bound-range", "#UD invalid-opcode", "#NM no-FPU",
    "#DF double-fault", "coproc-overrun", "#TS invalid-TSS", "#NP seg-not-present",
    "#SS stack-fault", "#GP general-protection", "#PF page-fault", "reserved15",
    "#MF x87-fp", "#AC alignment", "#MC machine-check", "#XM simd-fp",
    "#VE virtualization", "#CP control-protection", "res22", "res23",
    "res24", "res25", "res26", "res27", "res28", "res29", "res30", "res31",
};

/* Monotonic timer tick count. `volatile` because it is written by the IRQ0
 * handler (asynchronously, from the CPU's point of view) and read by the main
 * loop; volatile stops the compiler from hoisting the read out of that loop.
 * On a uniprocessor with interrupt gates (IF cleared in-handler) a plain
 * increment is atomic enough — no other context can observe a torn value. */
volatile uint64_t g_ticks = 0;

/* A minimal US-QWERTY scancode -> ASCII table for the "key down" (make) codes
 * 0x00..0x39. 0 means "no printable character" (modifiers, unknown, releases).
 * Set 1 scancodes; a key RELEASE has bit 7 set (scancode | 0x80), which we drop.*/
static const char SCAN2ASCII[0x3A] = {
    0,    27,  '1', '2', '3', '4', '5', '6', '7', '8',    /* 0x00..0x09 */
    '9',  '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',  /* 0x0A..0x13 */
    't',  'y', 'u', 'i', 'o',  'p', '[', ']', '\n', 0,    /* 0x14..0x1D (0x1D=LCtrl) */
    'a',  's', 'd', 'f', 'g',  'h', 'j', 'k', 'l', ';',   /* 0x1E..0x27 */
    '\'', '`', 0,   '\\','z',  'x', 'c', 'v', 'b', 'n',   /* 0x28..0x31 (0x2A=LShift) */
    'm',  ',', '.', '/', 0,    '*', 0,   ' ',             /* 0x32..0x39 (0x38=LAlt,0x39=Space) */
};

/* Emit one character to BOTH consoles so the boot log is captured on serial. */
static void kputc(char c) { vga_putc(c); serial_putc(c); }
static void kputs(const char *s) { vga_puts(s); serial_puts(s); }

/* IRQ0: the programmable interval timer. Just bump the tick counter. We refresh
 * a small "uptime" readout every 100 ticks (~1 s at the 100 Hz we program the
 * PIT to) so the screen visibly proves interrupts keep firing. */
static void handle_timer(void)
{
    g_ticks++;
    if (g_ticks % 100 == 0) {
        kputs("[timer] ticks=");
        vga_put_dec(g_ticks); serial_puts("...");
        kputc('\n');
    }
}

/* IRQ1: the keyboard controller. One scancode is waiting in port 0x60. We must
 * read it whether or not we use it, or the controller will not deliver the next
 * key. Bit 7 set = key RELEASE, which we ignore; otherwise map make-codes to
 * ASCII and echo. */
static void handle_keyboard(void)
{
    uint8_t sc = inb(0x60);
    if (sc & 0x80)                 /* release event: consume and ignore         */
        return;
    if (sc < 0x3A && SCAN2ASCII[sc]) {
        char c = SCAN2ASCII[sc];
        kputc(c);
    }
}

/* THE DISPATCHER. isr.S passes a pointer to the on-stack register frame. */
void isr_dispatch(struct regs *r)
{
    if (r->int_no < 32) {
        /* ---- a CPU exception: print what and where, then stop the world ---- */
        kputs("\n*** EXCEPTION ");
        vga_put_dec(r->int_no); serial_puts("(");
        kputs(EXC_NAME[r->int_no]);
        kputs(") err=");
        vga_put_hex(r->err_code); serial_puts(" ");
        kputs(" rip=");
        vga_put_hex(r->rip);
        kputs(" — halting ***\n");
        for (;;) __asm__ volatile("cli; hlt");   /* freeze: never returns       */
    } else if (r->int_no < 48) {
        /* ---- a hardware IRQ: service it, then acknowledge the PIC ---------- */
        uint8_t irq = (uint8_t)(r->int_no - 32);
        if (irq == 0)      handle_timer();
        else if (irq == 1) handle_keyboard();
        /* other IRQ lines are masked, so we never expect them; EOI regardless. */
        pic_send_eoi(irq);   /* MUST happen or the PIC stops delivering IRQs    */
    }
    /* vectors >= 48 are the catch-all sentinel; nothing to do but return. */
}
