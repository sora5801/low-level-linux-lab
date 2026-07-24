/* ===========================================================================
 * tcp.h — the TCP state machine, sequence bookkeeping, and timers.
 * ===========================================================================
 * This header declares the Transmission Control Block (TCB) — the per-connection
 * state RFC 793 calls out — and the three entry points the rest of the stack
 * uses: feed an inbound segment, drive the timers, and open a listener.
 * ========================================================================= */
#ifndef USERSPACE_TCPIP_TCP_H
#define USERSPACE_TCPIP_TCP_H

#include "netif.h"

/* ---------------------------------------------------------------------------
 * The eleven TCP states (RFC 793, figure 6). We reach LISTEN -> SYN_RCVD ->
 * ESTABLISHED on a passive open, and ESTABLISHED -> CLOSE_WAIT -> LAST_ACK ->
 * CLOSED on a passive close (the peer initiates teardown). The active-close
 * states (FIN_WAIT_1/2, CLOSING, TIME_WAIT) are implemented too so the machine
 * is complete, and TIME_WAIT is reachable if our side calls tcp_close().
 * ------------------------------------------------------------------------- */
enum tcp_state {
    TCP_CLOSED = 0,   /* no connection                                        */
    TCP_LISTEN,       /* waiting for a SYN                                    */
    TCP_SYN_SENT,     /* sent a SYN, awaiting SYN+ACK (active open)           */
    TCP_SYN_RCVD,     /* got a SYN, sent SYN+ACK, awaiting the final ACK      */
    TCP_ESTABLISHED,  /* handshake done; data may flow both ways             */
    TCP_FIN_WAIT_1,   /* we sent FIN, awaiting its ACK                        */
    TCP_FIN_WAIT_2,   /* our FIN acked; awaiting the peer's FIN               */
    TCP_CLOSE_WAIT,   /* peer sent FIN; we owe it a FIN once our app closes   */
    TCP_CLOSING,      /* simultaneous close: both FINs crossed                */
    TCP_LAST_ACK,     /* we (passive side) sent our FIN, awaiting its ACK     */
    TCP_TIME_WAIT     /* both closed; linger 2*MSL to absorb stray segments   */
};

#define TCP_SND_BUF 8192   /* per-connection send buffer (unacked+unsent)     */
#define TCP_RCV_BUF 8192   /* per-connection receive buffer / advertised wnd  */

/* ---------------------------------------------------------------------------
 * struct tcb — one connection's complete state.
 *
 * The send/receive "sequence variables" are RFC 793's SND.* / RCV.* kept in
 * HOST byte order (all arithmetic is modular over 2^32; we only byte-swap when
 * a value crosses the wire). Their relationships, for the send side:
 *
 *     snd_una      snd_nxt              snd_una+window
 *       |             |                     |
 *   ....#############-------------------.....   sndbuf timeline
 *       <--in flight--><---may send--->
 *
 *   snd_una .. snd_nxt  = sent but unacknowledged (retransmit candidates)
 *   snd_nxt .. buffered = queued by the app, not yet sent (window permitting)
 * ------------------------------------------------------------------------- */
struct tcb {
    int used;                 /* 0 = free slot                               */
    enum tcp_state state;

    /* Connection identity (the 4-tuple). IPs stay in NETWORK order to match
     * the wire directly; ports in HOST order for readable comparisons. */
    u32 local_ip, remote_ip;
    u16 local_port, remote_port;

    /* --- Send sequence space --- */
    u32 iss;          /* initial send sequence (the SYN's seq)               */
    u32 snd_una;      /* oldest unacknowledged sequence number               */
    u32 snd_nxt;      /* next sequence number we will send                   */
    u32 snd_wnd;      /* peer's advertised receive window (flow control)     */
    u32 snd_data_start; /* sequence number that sndbuf[0] represents         */
    u8  sndbuf[TCP_SND_BUF];
    u32 snd_len;      /* valid bytes in sndbuf (from snd_data_start)         */

    /* FIN bookkeeping: a FIN consumes one sequence number after the data. */
    int fin_queued;   /* app has closed; send a FIN after buffered data      */
    int fin_sent;     /* our FIN has been transmitted                        */
    u32 fin_seq;      /* the sequence number our FIN occupies                */

    /* --- Receive sequence space --- */
    u32 irs;          /* initial receive sequence (peer's SYN seq)           */
    u32 rcv_nxt;      /* next sequence number we expect from the peer        */
    u8  rcvbuf[TCP_RCV_BUF];
    u32 rcv_len;      /* bytes buffered for the app (drained immediately by
                       *   our echo "application", so usually 0)             */

    /* --- RTT estimation (Jacobson/Karels, RFC 6298), milliseconds --- */
    int srtt;         /* smoothed RTT, -1 until the first sample             */
    int rttvar;       /* RTT variation                                       */
    int rto;          /* current retransmission timeout                      */
    int rtt_active;   /* are we timing a segment right now?                  */
    u32 rtt_seq;      /* the last sequence number of the timed segment       */
    u32 rtt_start_ms; /* when we sent it                                     */

    /* --- Retransmission timer --- */
    int rtx_active;
    u32 rtx_deadline_ms;

    /* --- TIME_WAIT 2*MSL timer --- */
    u32 timewait_deadline_ms;
};

/* Zero the connection and listener tables. Call once at startup. */
void tcp_init(void);

/* Mark `port` (host order) as accepting passive opens (an echo service). */
int tcp_listen(u16 port);

/* Receive path: a TCP segment (IPv4 header already parsed as `ip`). */
void tcp_input(struct netif *nif, const struct ip_hdr *ip,
               u8 *payload, size_t len);

/* Drive retransmission and TIME_WAIT timers; call every event-loop tick. */
void tcp_timer_tick(struct netif *nif);

#endif /* USERSPACE_TCPIP_TCP_H */
