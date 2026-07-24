/* ===========================================================================
 * checksum.h — the Internet Checksum (RFC 1071), used by IP, ICMP, UDP, TCP.
 * ===========================================================================
 * One routine, called from four layers. See checksum.c for the full derivation;
 * asm/demo.c is a self-contained copy whose compiled assembly we annotate.
 * ========================================================================= */
#ifndef USERSPACE_TCPIP_CHECKSUM_H
#define USERSPACE_TCPIP_CHECKSUM_H

#include "common.h"

/* Accumulate the ones'-complement 16-bit sum of `len` bytes at `data` into the
 * running 32-bit accumulator `sum` and return it (NOT yet folded/complemented).
 * Chaining lets us checksum a pseudo-header and a segment in two calls. */
u32 csum_accumulate(const void *data, size_t len, u32 sum);

/* Fold the 32-bit accumulator down to 16 bits (adding in the carries) and take
 * the ones'-complement. The result is ready to drop into a header's checksum
 * field (it is already in the right byte order — see checksum.c). */
u16 csum_fold(u32 sum);

/* Convenience: full checksum of one buffer with no seed. */
u16 inet_checksum(const void *data, size_t len);

#endif /* USERSPACE_TCPIP_CHECKSUM_H */
