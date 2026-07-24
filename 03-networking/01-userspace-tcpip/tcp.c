/* ===========================================================================
 * tcp.c — a teaching TCP: handshake, data transfer, retransmission, teardown.
 * ===========================================================================
 *
 * WHAT THIS IMPLEMENTS (the teaching core)
 * ----------------------------------------
 *   - Passive open: LISTEN -> SYN_RCVD -> ESTABLISHED (the 3-way handshake).
 *   - Reliable, in-order data transfer with sequence/acknowledgement numbers.
 *   - An RTO (retransmission timeout) timer with Jacobson/Karels RTT estimation
 *     and exponential backoff (Karn's algorithm for ambiguous samples).
 *   - Flow control: we advertise a receive window; we honor the peer's window.
 *   - Connection teardown: the FIN exchange and the state machine around it,
 *     including TIME_WAIT's 2*MSL linger.
 *   - A trivial ECHO "application": bytes received are queued straight back.
 *
 * WHAT IT DELIBERATELY OMITS (be honest — see README "Going further")
 * ------------------------------------------------------------------
 *   - CONGESTION CONTROL. Real TCP also keeps a *congestion window* (cwnd) and
 *     grows/shrinks it with slow start + congestion avoidance (Reno) or CUBIC,
 *     reacting to loss/ECN to avoid collapsing a shared network. We send up to
 *     the peer's advertised (flow-control) window only. On an isolated tap link
 *     with one peer that is fine; on the Internet it is not.
 *   - Out-of-order buffering / SACK: we accept only the in-order segment and
 *     drop the rest (a duplicate-ACK prompts the peer to retransmit). SACK would
 *     let us hold the gaps and ack them selectively.
 *   - Window scaling, timestamps, PAWS, delayed ACKs, Nagle, zero-window persist.
 *
 * SEQUENCE-NUMBER ARITHMETIC
 * --------------------------
 * Sequence numbers are 32-bit and WRAP. "Is a before b?" cannot be a plain
 * a < b — near the wraparound that gives the wrong answer. The trick is to
 * compute the signed difference: (int32_t)(a - b) < 0 means a is "behind" b
 * within a half-sequence-space window. That is what seq_lt/seq_gt below do.
 * ========================================================================= */

#include "tcp.h"
#include "ip.h"
#include "checksum.h"

#include <string.h>
#include <time.h>      /* clock_gettime, CLOCK_MONOTONIC                       */

/* --- Tunables ------------------------------------------------------------- */
#define TCP_MSS       1460   /* max segment size we send (1500 MTU - 40 hdrs) */
#define TCP_MIN_RTO    200   /* clamp the RTO so we never busy-retransmit     */
#define TCP_MAX_RTO  60000   /* ...and never wait absurdly long               */
#define TCP_INIT_RTO  1000   /* first RTO, before we have an RTT sample        */
#define TCP_MSL       2000   /* Maximum Segment Lifetime (teaching value: 2s; a
                              *   real stack uses 30s-2min, so TIME_WAIT = 2*MSL
                              *   holds the 4-tuple for up to a couple minutes) */

#define MAX_TCB      8
#define MAX_LISTEN   4

static struct tcb g_tcbs[MAX_TCB];
static u16        g_listen_ports[MAX_LISTEN];  /* host order; 0 = unused       */

/* A rolling seed for initial sequence numbers. SECURITY: a real stack MUST
 * randomize the ISS (RFC 6528) so an off-path attacker cannot guess sequence
 * numbers and inject or reset connections. We use a coarse clock-derived value
 * plus a counter — good enough to avoid collisions between our own back-to-back
 * connections, but NOT a substitute for real randomization. */
static u32 g_iss_counter = 0;

/* --- Modular sequence comparisons (the wraparound-safe core) -------------- */
static inline int seq_lt(u32 a, u32 b) { return (int32_t)(a - b) <  0; }
static inline int seq_le(u32 a, u32 b) { return (int32_t)(a - b) <= 0; }
static inline int seq_gt(u32 a, u32 b) { return (int32_t)(a - b) >  0; }
static inline int seq_ge(u32 a, u32 b) { return (int32_t)(a - b) >= 0; }

/* --- Monotonic milliseconds. CLOCK_MONOTONIC never jumps (unlike wall time),
 * which is exactly what a retransmit timer needs: a steadily increasing tick
 * unaffected by NTP steps or the user setting the clock. ------------------- */
static u32 now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);   /* cannot fail with a valid clockid */
    return (u32)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
}

/* ===========================================================================
 * TCB table helpers
 * ========================================================================= */

static int port_is_listening(u16 port)
{
    for (int i = 0; i < MAX_LISTEN; i++)
        if (g_listen_ports[i] == port)
            return 1;
    return 0;
}

/* Find an established/half-open TCB matching this segment's 4-tuple. */
static struct tcb *tcb_find(u32 remote_ip, u16 remote_port, u16 local_port)
{
    for (int i = 0; i < MAX_TCB; i++) {
        struct tcb *t = &g_tcbs[i];
        if (t->used && t->remote_ip == remote_ip &&
            t->remote_port == remote_port && t->local_port == local_port)
            return t;
    }
    return NULL;
}

static struct tcb *tcb_alloc(void)
{
    for (int i = 0; i < MAX_TCB; i++)
        if (!g_tcbs[i].used) {
            memset(&g_tcbs[i], 0, sizeof(g_tcbs[i]));
            g_tcbs[i].used = 1;
            return &g_tcbs[i];
        }
    return NULL;   /* table full: we'll drop the SYN, peer retransmits later   */
}

static void tcb_free(struct tcb *t)
{
    /* Mark the slot reusable. Zeroing on alloc (above) means we don't need to
     * scrub here, but flipping `used` is the one thing that matters. */
    t->used = 0;
    t->state = TCP_CLOSED;
}

/* ===========================================================================
 * Segment transmission
 * ========================================================================= */

/* Advertised receive window = free space in our receive buffer. Capped at
 * 65535 because we do not implement window scaling (the plain 16-bit field). */
static u16 tcp_recv_window(const struct tcb *t)
{
    u32 free = TCP_RCV_BUF - t->rcv_len;
    return (free > 0xffff) ? 0xffff : (u16)free;
}

/* ---------------------------------------------------------------------------
 * tcp_send — build and transmit one segment.
 *
 * `flags` is the control-bit set (TCP_SYN/ACK/FIN/RST/PSH). `seq` is this
 * segment's sequence number. `data`/`dlen` is the payload (may be NULL/0). When
 * TCP_SYN is set we append the MSS option so the peer learns our segment size.
 * The ACK number is always our rcv_nxt (valid whenever TCP_ACK is set).
 * ------------------------------------------------------------------------- */
static int tcp_send(struct netif *nif, struct tcb *t, u8 flags,
                    u32 seq, const void *data, size_t dlen)
{
    u8 buf[TCP_HDR_MIN_LEN + 4 + TCP_MSS];   /* header + MSS option + payload  */
    size_t optlen = 0;

    struct tcp_hdr *th = (struct tcp_hdr *)buf;
    th->src_port = htons(t->local_port);
    th->dst_port = htons(t->remote_port);
    th->seq      = htonl(seq);
    th->ack      = htonl(t->rcv_nxt);   /* meaningful only if TCP_ACK set      */
    th->flags    = flags;
    th->window   = htons(tcp_recv_window(t));
    th->checksum = 0;
    th->urg_ptr  = 0;

    /* On a SYN, advertise our MSS (kind=2, len=4, then the 16-bit value). This
     * tells the peer the largest segment we want to receive so it won't send
     * anything that would fragment. */
    if (flags & TCP_SYN) {
        u8 *opt = buf + TCP_HDR_MIN_LEN;
        opt[0] = 2;                    /* option kind: Maximum Segment Size    */
        opt[1] = 4;                    /* option length including these 2 bytes*/
        u16 mss = htons(TCP_MSS);
        memcpy(opt + 2, &mss, 2);
        optlen = 4;
    }

    /* data offset = (header + options) in 32-bit words, in the high nibble. */
    size_t hdrlen = TCP_HDR_MIN_LEN + optlen;
    th->data_off = (u8)((hdrlen / 4) << 4);

    /* Append the payload after the header+options. */
    if (dlen)
        memcpy(buf + hdrlen, data, dlen);

    size_t seglen = hdrlen + dlen;

    /* Checksum over pseudo-header + the whole segment (header+options+data),
     * with the checksum field zeroed. Same construction as UDP. */
    struct pseudo_hdr ph;
    ph.src    = nif->ip;
    ph.dst    = t->remote_ip;
    ph.zero   = 0;
    ph.proto  = IPPROTO_TCP_;
    ph.length = htons((u16)seglen);
    u32 sum = csum_accumulate(&ph, sizeof(ph), 0);
    sum = csum_accumulate(buf, seglen, sum);
    th->checksum = csum_fold(sum);

    return ip_output(nif, t->remote_ip, IPPROTO_TCP_, buf, seglen);
}

/* A bare ACK: no payload, seq = snd_nxt (a pure ACK consumes no sequence
 * space, so it carries the next seq we *would* send). Used to acknowledge data,
 * FINs, and to nudge a peer that sent something out of order. */
static void tcp_send_ack(struct netif *nif, struct tcb *t)
{
    tcp_send(nif, t, TCP_ACK, t->snd_nxt, NULL, 0);
}

/* ===========================================================================
 * RTT estimation & the retransmission timer
 * ========================================================================= */

/* Fold a fresh round-trip measurement R (ms) into the smoothed estimators and
 * recompute the RTO. RFC 6298:
 *     RTTVAR = 3/4 RTTVAR + 1/4 |SRTT - R|
 *     SRTT   = 7/8 SRTT   + 1/8 R
 *     RTO    = SRTT + 4*RTTVAR   (clamped)
 * The first sample seeds SRTT=R, RTTVAR=R/2. Keeping 4*RTTVAR in the RTO is what
 * makes the timer patient on a jittery path and snappy on a steady one. */
static void tcp_rtt_update(struct tcb *t, int r)
{
    if (t->srtt < 0) {
        t->srtt   = r;
        t->rttvar = r / 2;
    } else {
        int absdev = t->srtt - r;
        if (absdev < 0) absdev = -absdev;
        t->rttvar = (3 * t->rttvar + absdev) / 4;
        t->srtt   = (7 * t->srtt + r) / 8;
    }
    t->rto = t->srtt + 4 * t->rttvar;
    if (t->rto < TCP_MIN_RTO) t->rto = TCP_MIN_RTO;
    if (t->rto > TCP_MAX_RTO) t->rto = TCP_MAX_RTO;
}

static void tcp_rtx_start(struct tcb *t)
{
    t->rtx_active      = 1;
    t->rtx_deadline_ms = now_ms() + (u32)t->rto;
}

static void tcp_rtx_stop(struct tcb *t)
{
    t->rtx_active = 0;
}

/* ===========================================================================
 * The output engine: push as much as the window allows, then a FIN if queued.
 * ========================================================================= */
static void tcp_output(struct netif *nif, struct tcb *t)
{
    /* Bytes already in flight (sent, unacked). Flow control caps total in-flight
     * at the peer's advertised window. NOTE: a full TCP would take the MINIMUM
     * of this and the congestion window (cwnd); we have no cwnd (see header). */
    for (;;) {
        u32 in_flight = t->snd_nxt - t->snd_una;
        if (in_flight >= t->snd_wnd)
            break;                                  /* window full             */

        /* Unsent bytes sit in sndbuf between snd_nxt and the end of buffered
         * data. sndbuf[0] == snd_data_start, so snd_nxt maps to this offset: */
        u32 offset = t->snd_nxt - t->snd_data_start;
        if (offset > t->snd_len)                    /* nothing new to send     */
            break;
        u32 unsent = t->snd_len - offset;
        if (unsent == 0)
            break;

        /* Send min(unsent, remaining window, MSS) in one segment. */
        u32 usable = t->snd_wnd - in_flight;
        u32 chunk  = unsent;
        if (chunk > usable)  chunk = usable;
        if (chunk > TCP_MSS) chunk = TCP_MSS;
        if (chunk == 0)
            break;

        /* PSH tells the peer to hand this to its app promptly (we always push,
         * since we have no Nagle coalescing). ACK is always on in ESTABLISHED. */
        tcp_send(nif, t, TCP_ACK | TCP_PSH, t->snd_nxt,
                 &t->sndbuf[offset], chunk);

        /* Start an RTT sample if we aren't already timing one (Karn: never time
         * a retransmit). We time the LAST byte of this segment; the sample ends
         * when an ACK covers rtt_seq. */
        if (!t->rtt_active) {
            t->rtt_active   = 1;
            t->rtt_seq      = t->snd_nxt + chunk - 1;
            t->rtt_start_ms = now_ms();
        }

        t->snd_nxt += chunk;

        /* Any unacked data means the retransmit timer must be running. */
        if (!t->rtx_active)
            tcp_rtx_start(t);
    }

    /* After all queued data has been sent, emit the FIN if the app has closed.
     * The FIN occupies the sequence number right after the last data byte. */
    if (t->fin_queued && !t->fin_sent) {
        u32 end = t->snd_data_start + t->snd_len;   /* seq after last data byte */
        if (t->snd_nxt == end) {                    /* all data is out          */
            t->fin_seq = t->snd_nxt;
            tcp_send(nif, t, TCP_ACK | TCP_FIN, t->snd_nxt, NULL, 0);
            t->snd_nxt += 1;                        /* FIN consumes one seq     */
            t->fin_sent = 1;
            if (!t->rtx_active)
                tcp_rtx_start(t);
        }
    }
}

/* ===========================================================================
 * The "application": an echo server, wired through a real receive buffer.
 *
 * tcp_deliver() puts in-order bytes into the connection's RECEIVE buffer — this
 * is the queue a real socket's recv() would read from, and its free space is
 * exactly the window we advertise. app_echo() then plays the application: it
 * reads everything available and writes it straight back into the SEND buffer.
 * Splitting the two makes the data path honest: bytes are received, buffered,
 * then produced by an app, rather than teleported from wire to wire.
 * ========================================================================= */
static void tcp_deliver(struct tcb *t, const u8 *data, u32 len)
{
    /* Accept at most the free receive space — i.e. never more than the window
     * we advertised to the peer. With immediate draining below this is always
     * the full segment, but the clamp is the flow-control invariant made
     * explicit. rcv_nxt advances only by what we actually buffered, so our ACK
     * never claims bytes we dropped. */
    u32 rfree = TCP_RCV_BUF - t->rcv_len;
    if (len > rfree) len = rfree;
    memcpy(&t->rcvbuf[t->rcv_len], data, len);
    t->rcv_len += len;
    t->rcv_nxt += len;
}

static void app_echo(struct tcb *t)
{
    /* Copy the receive buffer into the send buffer. If the send buffer can't
     * hold it all we drop the excess here — a real application would instead
     * leave the bytes unread, which keeps rcv_len high, shrinks our advertised
     * window, and throttles the sender (true flow control / back-pressure). */
    u32 sfree = TCP_SND_BUF - t->snd_len;
    u32 n = t->rcv_len;
    if (n > sfree) n = sfree;
    memcpy(&t->sndbuf[t->snd_len], t->rcvbuf, n);
    t->snd_len += n;
    t->rcv_len = 0;   /* drained (any excess dropped, per the comment above)   */
}

/* ===========================================================================
 * Acknowledgement processing: advance snd_una, free acked bytes, sample RTT.
 * ========================================================================= */
static void tcp_process_ack(struct tcb *t, u32 ack)
{
    /* Only ACKs that advance snd_una within the outstanding range are useful.
     * ack == snd_una is a duplicate ACK (ignored here — a full stack would use
     * three of them to trigger fast retransmit). ack > snd_nxt is nonsense. */
    if (!(seq_gt(ack, t->snd_una) && seq_le(ack, t->snd_nxt)))
        return;

    /* How many bytes of buffered DATA does this ACK free? The acknowledged span
     * is [snd_una, ack). Some of it may be the phantom SYN (seq iss) or FIN
     * (seq fin_seq), which occupy sequence space but no sndbuf byte. Intersect
     * [snd_una, ack) with the data region [snd_data_start, snd_data_start+len). */
    u32 data_lo = t->snd_una;
    if (seq_lt(data_lo, t->snd_data_start))
        data_lo = t->snd_data_start;
    u32 data_hi = ack;
    u32 data_end = t->snd_data_start + t->snd_len;
    if (seq_gt(data_hi, data_end))
        data_hi = data_end;

    if (seq_lt(data_lo, data_hi)) {
        u32 ndata = data_hi - data_lo;
        /* Slide the freed bytes out of the front of sndbuf and advance the base
         * sequence number that sndbuf[0] represents. memmove (not memcpy)
         * because source and destination overlap. */
        memmove(t->sndbuf, t->sndbuf + ndata, t->snd_len - ndata);
        t->snd_len        -= ndata;
        t->snd_data_start += ndata;
    }

    t->snd_una = ack;

    /* RTT sample (Karn's algorithm): only if we were timing and this ACK covers
     * the timed sequence number. rtt_active was cleared on any retransmit, so an
     * ambiguous (retransmitted) segment never produces a sample. */
    if (t->rtt_active && seq_gt(ack, t->rtt_seq)) {
        int r = (int)(now_ms() - t->rtt_start_ms);
        if (r < 0) r = 0;
        tcp_rtt_update(t, r);
        t->rtt_active = 0;
    }

    /* Retransmit timer: if everything sent is now acked, stop it; otherwise
     * restart it for the still-outstanding data (RFC 6298 step 5.3). A fresh
     * cumulative ACK also resets exponential backoff, which happens implicitly
     * because we recompute the deadline from the RTT-derived rto. */
    if (t->snd_una == t->snd_nxt)
        tcp_rtx_stop(t);
    else
        tcp_rtx_start(t);
}

/* ===========================================================================
 * The receive path: one segment through the state machine.
 * ========================================================================= */
void tcp_input(struct netif *nif, const struct ip_hdr *ip,
               u8 *payload, size_t len)
{
    if (len < TCP_HDR_MIN_LEN) {
        LOGF("tcp: runt (%zu)\n", len);
        return;
    }
    struct tcp_hdr *th = (struct tcp_hdr *)payload;

    /* Verify the checksum over pseudo-header + segment. A correct segment sums
     * to 0xFFFF, so recomputing with the field left in place yields 0. */
    struct pseudo_hdr ph;
    ph.src = ip->src; ph.dst = ip->dst; ph.zero = 0;
    ph.proto = IPPROTO_TCP_; ph.length = htons((u16)len);
    u32 sum = csum_accumulate(&ph, sizeof(ph), 0);
    sum = csum_accumulate(payload, len, sum);
    if (csum_fold(sum) != 0) {
        LOGF("tcp: bad checksum\n");
        return;
    }

    /* Header length from data_off (high nibble, in 32-bit words). Options sit
     * between the fixed header and the payload; we skip them to find the data
     * but otherwise ignore them in this core (we don't act on peer MSS/SACK). */
    size_t hdrlen = (size_t)(th->data_off >> 4) * 4;
    if (hdrlen < TCP_HDR_MIN_LEN || hdrlen > len) {
        LOGF("tcp: bad data offset\n");
        return;
    }
    u8    *data    = payload + hdrlen;
    size_t datalen = len - hdrlen;

    u16 sport = ntohs(th->src_port);   /* peer's port                          */
    u16 dport = ntohs(th->dst_port);   /* our port                             */
    u32 seg_seq = ntohl(th->seq);
    u32 seg_ack = ntohl(th->ack);
    u16 seg_wnd = ntohs(th->window);
    u8  flags   = th->flags;

    struct tcb *t = tcb_find(ip->src, sport, dport);

    /* ---- No connection yet: this must be a SYN to a listening port -------- */
    if (!t) {
        if ((flags & TCP_SYN) && !(flags & TCP_ACK) && port_is_listening(dport)) {
            t = tcb_alloc();
            if (!t) {
                LOGF("tcp: no TCB free, dropping SYN\n");
                return;   /* peer will retransmit the SYN                      */
            }
            /* Passive open: record the peer, initialize both sequence spaces. */
            t->state       = TCP_SYN_RCVD;
            t->local_ip    = ip->dst;   t->remote_ip   = ip->src;
            t->local_port  = dport;     t->remote_port = sport;

            t->irs     = seg_seq;             /* peer's initial sequence        */
            t->rcv_nxt = seg_seq + 1;         /* +1: the SYN consumes one seq   */
            t->snd_wnd = seg_wnd ? seg_wnd : 1;

            /* Our initial send sequence (see the g_iss_counter security note). */
            t->iss           = (now_ms() << 8) + (g_iss_counter += 0x9e37);
            t->snd_una       = t->iss;
            t->snd_nxt       = t->iss;        /* SYN will bump this to iss+1     */
            t->snd_data_start= t->iss + 1;    /* first data byte lives after SYN */
            t->snd_len       = 0;

            t->srtt = -1; t->rttvar = 0; t->rto = TCP_INIT_RTO;

            /* Send SYN+ACK: seq = iss, ack = rcv_nxt. The SYN consumes seq iss,
             * so snd_nxt becomes iss+1. Arm the retransmit timer — the SYN+ACK
             * itself must be retransmitted if the final ACK is lost. */
            tcp_send(nif, t, TCP_SYN | TCP_ACK, t->iss, NULL, 0);
            t->snd_nxt = t->iss + 1;
            tcp_rtx_start(t);
            LOGF("tcp: %u: LISTEN -> SYN_RCVD (peer port %u)\n", dport, sport);
            return;
        }
        /* A stray segment to a port we aren't listening on. RFC 793 says answer
         * with a RST so the peer stops waiting; our teaching core simply drops.
         * (Adding RST generation here is a good exercise — see README.) */
        return;
    }

    /* ---- RST: abort immediately, wherever we are. ------------------------ */
    if (flags & TCP_RST) {
        LOGF("tcp: %u: RST received, connection reset\n", t->local_port);
        tcb_free(t);
        return;
    }

    /* ---- Basic acceptability: we only handle IN-ORDER segments. ----------
     * If the segment doesn't start exactly where we expect (seg_seq==rcv_nxt),
     * it is either a retransmission of already-received data or an out-of-order
     * arrival. We drop it and (re)send an ACK advertising rcv_nxt, which tells
     * the peer what we still need. A SACK-capable stack would instead buffer the
     * gap. Pure ACKs (no data, no SYN/FIN) carry seg_seq == rcv_nxt too. */
    int has_syn = (flags & TCP_SYN) != 0;
    if (!has_syn && t->state != TCP_SYN_RCVD && seg_seq != t->rcv_nxt) {
        tcp_send_ack(nif, t);
        return;
    }

    /* ---- Process the ACK field (advances our send window). --------------- */
    if (flags & TCP_ACK) {
        tcp_process_ack(t, seg_ack);
        t->snd_wnd = seg_wnd ? seg_wnd : t->snd_wnd;  /* update peer's window   */
    }

    /* ---- Per-state handling of SYN_RCVD -> ESTABLISHED. ------------------ */
    if (t->state == TCP_SYN_RCVD) {
        /* The handshake completes when the peer's ACK acknowledges our SYN, i.e.
         * snd_una has advanced past iss. */
        if (seq_gt(t->snd_una, t->iss)) {
            t->state = TCP_ESTABLISHED;
            LOGF("tcp: %u: SYN_RCVD -> ESTABLISHED\n", t->local_port);
            /* fall through so any piggybacked data/FIN is processed below */
        } else {
            return;   /* not the ACK we need yet                              */
        }
    }

    /* ---- Deliver in-order data (ESTABLISHED / CLOSE_WAIT-ish). ----------- */
    if (datalen > 0 &&
        (t->state == TCP_ESTABLISHED || t->state == TCP_FIN_WAIT_1 ||
         t->state == TCP_FIN_WAIT_2)) {
        /* seg_seq == rcv_nxt was enforced above. Buffer the bytes (advancing
         * rcv_nxt), then run the echo app to queue them back. */
        tcp_deliver(t, data, (u32)datalen);
        app_echo(t);
        /* Send whatever the app just queued, then acknowledge the data we
         * consumed (tcp_output emits ACK-bearing segments; if it sent nothing,
         * we still owe a bare ACK). */
        u32 before = t->snd_nxt;
        tcp_output(nif, t);
        if (t->snd_nxt == before)
            tcp_send_ack(nif, t);
    }

    /* ---- FIN: the peer is done sending. --------------------------------- */
    if (flags & TCP_FIN) {
        /* The FIN sits at seg_seq + datalen; we've consumed the data, so the FIN
         * is exactly at rcv_nxt now. Consume its one sequence number and ACK. */
        t->rcv_nxt += 1;
        tcp_send_ack(nif, t);

        switch (t->state) {
        case TCP_ESTABLISHED:
            /* Passive close: peer closed first. We move to CLOSE_WAIT and, since
             * our echo app has nothing more to say once the peer is done, we
             * immediately "close" too: queue a FIN and send it (-> LAST_ACK). */
            t->state = TCP_CLOSE_WAIT;
            LOGF("tcp: %u: ESTABLISHED -> CLOSE_WAIT\n", t->local_port);
            t->fin_queued = 1;
            tcp_output(nif, t);
            if (t->fin_sent) {
                t->state = TCP_LAST_ACK;
                LOGF("tcp: %u: CLOSE_WAIT -> LAST_ACK\n", t->local_port);
            }
            break;
        case TCP_FIN_WAIT_1:
            /* Simultaneous close: our FIN is still unacked but the peer's FIN
             * arrived. If our FIN was also acked (handled above), we'd be in
             * FIN_WAIT_2 already; here it wasn't, so go CLOSING. */
            t->state = TCP_CLOSING;
            LOGF("tcp: %u: FIN_WAIT_1 -> CLOSING\n", t->local_port);
            break;
        case TCP_FIN_WAIT_2:
            /* Normal active close: we already sent+acked our FIN, now the peer's
             * FIN arrives. Enter TIME_WAIT and start the 2*MSL linger. */
            t->state = TCP_TIME_WAIT;
            t->timewait_deadline_ms = now_ms() + 2 * TCP_MSL;
            LOGF("tcp: %u: FIN_WAIT_2 -> TIME_WAIT\n", t->local_port);
            break;
        default:
            break;
        }
        return;
    }

    /* ---- State transitions driven purely by an ACK (no FIN in this seg). -- */
    switch (t->state) {
    case TCP_FIN_WAIT_1:
        /* Our FIN got acked (snd_una passed fin_seq) -> FIN_WAIT_2. */
        if (t->fin_sent && seq_gt(t->snd_una, t->fin_seq)) {
            t->state = TCP_FIN_WAIT_2;
            LOGF("tcp: %u: FIN_WAIT_1 -> FIN_WAIT_2\n", t->local_port);
        }
        break;
    case TCP_CLOSING:
        /* Both FINs exchanged; our FIN now acked -> TIME_WAIT. */
        if (t->fin_sent && seq_gt(t->snd_una, t->fin_seq)) {
            t->state = TCP_TIME_WAIT;
            t->timewait_deadline_ms = now_ms() + 2 * TCP_MSL;
            LOGF("tcp: %u: CLOSING -> TIME_WAIT\n", t->local_port);
        }
        break;
    case TCP_LAST_ACK:
        /* Passive close finished: the peer acked our FIN -> CLOSED. Free it. */
        if (t->fin_sent && seq_gt(t->snd_una, t->fin_seq)) {
            LOGF("tcp: %u: LAST_ACK -> CLOSED\n", t->local_port);
            tcb_free(t);
        }
        break;
    default:
        break;
    }
}

/* ===========================================================================
 * Timers: retransmission and TIME_WAIT. Called every event-loop tick.
 * ========================================================================= */
void tcp_timer_tick(struct netif *nif)
{
    u32 now = now_ms();

    for (int i = 0; i < MAX_TCB; i++) {
        struct tcb *t = &g_tcbs[i];
        if (!t->used)
            continue;

        /* TIME_WAIT: once 2*MSL has elapsed, the 4-tuple is safe to reuse. This
         * linger is what absorbs a delayed duplicate of the peer's FIN so it
         * can't disrupt a fresh connection reusing the same ports. */
        if (t->state == TCP_TIME_WAIT) {
            if (seq_ge(now, t->timewait_deadline_ms)) {
                LOGF("tcp: %u: TIME_WAIT expired -> CLOSED\n", t->local_port);
                tcb_free(t);
            }
            continue;
        }

        /* Retransmission timeout. When the timer fires we resend the OLDEST
         * unacknowledged segment (the one at snd_una), back off the RTO
         * exponentially (RFC 6298 step 5.5), and — per Karn — do NOT take an RTT
         * sample from the retransmitted data. */
        if (t->rtx_active && seq_ge(now, t->rtx_deadline_ms)) {
            t->rtt_active = 0;                 /* Karn: this sample is ambiguous */
            t->rto *= 2;                       /* exponential backoff            */
            if (t->rto > TCP_MAX_RTO) t->rto = TCP_MAX_RTO;

            /* What lives at snd_una? The SYN, a data byte, or the FIN. */
            if (t->state == TCP_SYN_RCVD && t->snd_una == t->iss) {
                /* Resend SYN+ACK. */
                tcp_send(nif, t, TCP_SYN | TCP_ACK, t->iss, NULL, 0);
                LOGF("tcp: %u: retransmit SYN+ACK (rto now %d ms)\n",
                     t->local_port, t->rto);
            } else if (t->fin_sent && t->snd_una == t->fin_seq) {
                /* Only the FIN is outstanding; resend it. */
                tcp_send(nif, t, TCP_ACK | TCP_FIN, t->fin_seq, NULL, 0);
                LOGF("tcp: %u: retransmit FIN\n", t->local_port);
            } else {
                /* Resend one MSS of data starting at snd_una. */
                u32 offset = t->snd_una - t->snd_data_start;
                if (offset <= t->snd_len) {
                    u32 avail = t->snd_len - offset;
                    u32 chunk = avail > TCP_MSS ? TCP_MSS : avail;
                    if (chunk > 0) {
                        tcp_send(nif, t, TCP_ACK | TCP_PSH, t->snd_una,
                                 &t->sndbuf[offset], chunk);
                        LOGF("tcp: %u: retransmit %u data bytes @seq %u\n",
                             t->local_port, chunk, t->snd_una);
                    }
                }
            }
            /* Rearm the timer with the (now larger) rto. */
            tcp_rtx_start(t);
        }
    }
}

/* ===========================================================================
 * Public setup
 * ========================================================================= */
void tcp_init(void)
{
    memset(g_tcbs, 0, sizeof(g_tcbs));
    memset(g_listen_ports, 0, sizeof(g_listen_ports));
}

int tcp_listen(u16 port)
{
    for (int i = 0; i < MAX_LISTEN; i++) {
        if (g_listen_ports[i] == 0) {
            g_listen_ports[i] = port;
            LOGF("tcp: listening on port %u\n", port);
            return 0;
        }
    }
    LOGF("tcp: listen table full\n");
    return -1;
}
