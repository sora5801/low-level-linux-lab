/* ===========================================================================
 * printf.c — a small, varargs, buffered formatted printer.
 * ===========================================================================
 *
 * printf is where three ideas meet: (1) variadic arguments pulled off the
 * SysV ABI by va_arg, (2) integer-to-ASCII conversion by repeated divide, and
 * (3) buffered output so a 100-character line costs ONE write(2), not 100.
 *
 * Supported conversions: %d %i (signed), %u (unsigned), %x %X (hex),
 * %c (char), %s (string), %p (pointer), %% (literal percent), and an `l`
 * length modifier (%ld/%lu/%lx...) for 64-bit longs. No width/precision/flags
 * — those are a straightforward extension left to the reader (see musl's
 * src/stdio/vfprintf.c for the full state machine).
 *
 * SECURITY NOTE — the format string must be a trusted, constant literal.
 * `printf(user_input)` is the classic format-string vulnerability: an attacker
 * supplies "%x%x%x..." to leak stack memory or "%n" to WRITE to memory. Our
 * printf has no %n (removing the write primitive), and the demos always pass a
 * literal format with the data as separate arguments — the safe pattern. The
 * header tags these with __attribute__((format(printf,...))) so the compiler
 * warns when the format and arguments disagree.
 * ===========================================================================
 */
#include "minilibc.h"

/* ---------------------------------------------------------------------------
 * A tiny output buffer. We accumulate formatted bytes here and flush to the fd
 * with write_all() either when full or at the end of the format string. `total`
 * counts everything we have emitted, which becomes printf's return value.
 * --------------------------------------------------------------------------- */
typedef struct {
	int  fd;
	int  len;              /* bytes currently pending in buf                   */
	int  total;            /* bytes emitted overall (the return value)         */
	char buf[256];
} outbuf_t;

static void ob_flush(outbuf_t *o)
{
	if (o->len > 0) {
		write_all(o->fd, o->buf, (size_t)o->len);
		o->len = 0;
	}
}

/* Append one byte, flushing first if the buffer is full. */
static void ob_putc(outbuf_t *o, char c)
{
	if (o->len == (int)sizeof(o->buf))
		ob_flush(o);
	o->buf[o->len++] = c;
	o->total++;
}

static void ob_puts(outbuf_t *o, const char *s)
{
	while (*s)
		ob_putc(o, *s++);
}

/* ---------------------------------------------------------------------------
 * ob_uint — the heart of %u/%x/%p: convert an unsigned 64-bit value to ASCII
 * in the given base and emit it. The algorithm produces digits LEAST-
 * significant first (v % base), so we stash them in a scratch buffer and print
 * it in reverse. A 64-bit value is at most 64 binary digits, so tmp[64] can
 * never overflow — even base 2 fits. (This is the routine asm/demo.c isolates.)
 * --------------------------------------------------------------------------- */
static void ob_uint(outbuf_t *o, unsigned long v, unsigned base, int upper)
{
	static const char lo[] = "0123456789abcdef";
	static const char up[] = "0123456789ABCDEF";
	const char *digits = upper ? up : lo;

	char tmp[64];
	int i = 0;

	if (v == 0) {                 /* special-case 0: the loop below emits nothing */
		ob_putc(o, '0');
		return;
	}
	while (v != 0) {
		tmp[i++] = digits[v % base];   /* least-significant digit first          */
		v /= base;                     /* clang turns /10 into a magic multiply  */
	}
	while (i > 0)                       /* now print most-significant first       */
		ob_putc(o, tmp[--i]);
}

/* ob_int — signed decimal (%d/%i). Print a '-' for negatives, then the
 * magnitude as unsigned. We compute the magnitude as -(unsigned)v rather than
 * -v so that LONG_MIN (whose positive has no representable signed value) still
 * converts correctly: unsigned negation is well-defined modular arithmetic. */
static void ob_int(outbuf_t *o, long v)
{
	if (v < 0) {
		ob_putc(o, '-');
		ob_uint(o, -(unsigned long)v, 10, 0);
	} else {
		ob_uint(o, (unsigned long)v, 10, 0);
	}
}

/* ---------------------------------------------------------------------------
 * vdprintf — format `fmt` with args `ap` and write the result to fd.
 * Returns the number of bytes emitted. This is the engine; printf/dprintf are
 * thin wrappers that set up the va_list.
 * --------------------------------------------------------------------------- */
int vdprintf(int fd, const char *fmt, va_list ap)
{
	outbuf_t o = { .fd = fd, .len = 0, .total = 0 };

	for (const char *p = fmt; *p != '\0'; p++) {
		if (*p != '%') {           /* ordinary text: copy verbatim             */
			ob_putc(&o, *p);
			continue;
		}

		p++;                       /* consume the '%'; look at the conversion  */

		/* Optional 'l' length modifier(s): %ld, %lu, %lx read a 64-bit long
		 * instead of a 32-bit int. We just note "long or not"; %ll is treated
		 * the same since long and long long are both 64-bit here. */
		int is_long = 0;
		while (*p == 'l') {
			is_long = 1;
			p++;
		}

		switch (*p) {
		case 'd':
		case 'i':                  /* signed decimal                           */
			/* Default argument promotion already widened `int` to a full slot,
			 * but its VALUE is 32-bit unless the caller passed a long. */
			ob_int(&o, is_long ? va_arg(ap, long)
			                   : (long)va_arg(ap, int));
			break;

		case 'u':                  /* unsigned decimal                         */
			ob_uint(&o, is_long ? va_arg(ap, unsigned long)
			                    : (unsigned long)va_arg(ap, unsigned int),
			        10, 0);
			break;

		case 'x':                  /* lowercase hex                            */
			ob_uint(&o, is_long ? va_arg(ap, unsigned long)
			                    : (unsigned long)va_arg(ap, unsigned int),
			        16, 0);
			break;

		case 'X':                  /* uppercase hex                            */
			ob_uint(&o, is_long ? va_arg(ap, unsigned long)
			                    : (unsigned long)va_arg(ap, unsigned int),
			        16, 1);
			break;

		case 'p':                  /* pointer: "0x" then the address in hex    */
			ob_puts(&o, "0x");
			ob_uint(&o, (unsigned long)va_arg(ap, void *), 16, 0);
			break;

		case 'c':                  /* single char (promoted to int in varargs) */
			ob_putc(&o, (char)va_arg(ap, int));
			break;

		case 's': {                /* NUL-terminated string; guard against NULL */
			const char *s = va_arg(ap, const char *);
			ob_puts(&o, s ? s : "(null)");
			break;
		}

		case '%':                  /* a literal percent sign                   */
			ob_putc(&o, '%');
			break;

		case '\0':                 /* format ended right after '%': stop safely */
			p--;                   /* let the for-loop's *p==0 test end it      */
			break;

		default:                   /* unknown %? : print it literally, verbatim */
			ob_putc(&o, '%');
			ob_putc(&o, *p);
			break;
		}
	}

	ob_flush(&o);                  /* push whatever is pending out with write() */
	return o.total;
}

/* printf — format to stdout (fd 1). The workhorse the demos call. */
int printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);             /* ap now points at the first arg after fmt */
	int n = vdprintf(1, fmt, ap);
	va_end(ap);
	return n;
}

/* dprintf — like printf but to an arbitrary fd (e.g. 2 for stderr). */
int dprintf(int fd, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int n = vdprintf(fd, fmt, ap);
	va_end(ap);
	return n;
}
