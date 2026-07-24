/* ===========================================================================
 * bench_client.c — a tiny closed-loop load generator for the echo servers.
 * ===========================================================================
 *
 * It opens N connections (one per worker thread), and each thread ping-pongs a
 * fixed-size message: send it, read the same number of bytes back, count one
 * round-trip, repeat until the deadline. At the end it prints total round-trips
 * and round-trips/sec. Because both echo_uring and epoll_echo speak the same
 * raw-TCP echo protocol, the SAME client measures both — an apples-to-apples
 * comparison.
 *
 * WHY BLOCKING SOCKETS + THREADS: the client's job is only to keep the server
 * busy, and a blocking ping-pong per thread is the simplest thing that is
 * obviously correct. It is a *closed-loop* (one outstanding request per conn)
 * benchmark, so it measures round-trip latency-bound throughput, not the
 * server's peak pipelined throughput. That is fine for the teaching comparison:
 * both servers face the identical load, and the headline result — the syscall
 * count from `strace -c` — is independent of how hard we push. See the README.
 *
 * PLATFORM: Linux/POSIX. Build: cc -O2 -pthread bench_client.c -o bench_client
 * ===========================================================================
 */
#define _GNU_SOURCE
#include "common.h"
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <signal.h>      /* signal, SIGPIPE, SIG_IGN                            */

static const char *g_host = "127.0.0.1";
static int         g_port = 8080;
static int         g_msgsize = 64;      /* bytes per ping                       */
static int         g_seconds = 5;       /* run duration                         */
static volatile int g_stop = 0;         /* set when the deadline passes         */

/* Each worker reports its own round-trip tally; main sums them. No shared
 * counter means no atomics and no false sharing in the hot loop. */
struct worker {
    pthread_t  thread;
    long       roundtrips;
    int        ok;                       /* did this worker connect?            */
};

/* now_sec — a monotonic clock in floating seconds. CLOCK_MONOTONIC never jumps
 * backward (unlike CLOCK_REALTIME under NTP/leap seconds), so it is the right
 * clock for measuring an elapsed interval. */
static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* read_full — read exactly `len` bytes (the echo may arrive in several TCP
 * segments). Returns len on success, or -1 if the peer closed/errored. This is
 * the reassembly every stream-socket client needs: TCP is a byte stream, so one
 * write() on the server can surface as several read()s here, and vice versa. */
static int read_full(int fd, char *buf, int len)
{
    int got = 0;
    while (got < len) {
        ssize_t n = read(fd, buf + got, (size_t)(len - got));
        if (n > 0) {
            got += (int)n;
            continue;
        }
        if (n < 0 && errno == EINTR)
            continue;                    /* interrupted; retry                   */
        return -1;                        /* n==0 EOF, or a hard error            */
    }
    return got;
}

/* write_full — write exactly `len` bytes (a single write() may be partial). */
static int write_full(int fd, const char *buf, int len)
{
    int sent = 0;
    while (sent < len) {
        ssize_t n = write(fd, buf + sent, (size_t)(len - sent));
        if (n > 0) {
            sent += (int)n;
            continue;
        }
        if (n < 0 && errno == EINTR)
            continue;
        return -1;
    }
    return sent;
}

/* connect_to — open one blocking TCP connection to the server. */
static int connect_to(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port   = htons((unsigned short)g_port);   /* network byte order       */
    /* inet_pton parses "127.0.0.1" into the 32-bit network-order address. */
    if (inet_pton(AF_INET, g_host, &sa.sin_addr) != 1) {
        close(fd);
        return -1;
    }
    if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        close(fd);
        return -1;
    }
    (void)set_nodelay(fd);               /* latency-bound loop: disable Nagle    */
    return fd;
}

/* worker_main — one connection's ping-pong loop. */
static void *worker_main(void *arg)
{
    struct worker *w = (struct worker *)arg;
    w->roundtrips = 0;
    w->ok = 0;

    int fd = connect_to();
    if (fd < 0)
        return NULL;                     /* leave w->ok = 0; main will notice    */
    w->ok = 1;

    /* A fixed payload; the server echoes it verbatim so we can verify length. */
    char *out = malloc((size_t)g_msgsize);
    char *in  = malloc((size_t)g_msgsize);
    if (!out || !in) { free(out); free(in); close(fd); return NULL; }
    memset(out, 'x', (size_t)g_msgsize);

    while (!g_stop) {
        if (write_full(fd, out, g_msgsize) < 0)
            break;
        if (read_full(fd, in, g_msgsize) < 0)
            break;
        w->roundtrips++;
    }

    free(out);
    free(in);
    close(fd);
    return NULL;
}

int main(int argc, char **argv)
{
    /* usage: bench_client [host] [port] [conns] [seconds] [msgsize] */
    int conns = 64;
    if (argc > 1) g_host    = argv[1];
    if (argc > 2) g_port    = atoi(argv[2]);
    if (argc > 3) conns     = atoi(argv[3]);
    if (argc > 4) g_seconds = atoi(argv[4]);
    if (argc > 5) g_msgsize = atoi(argv[5]);
    if (conns < 1) conns = 1;
    if (g_msgsize < 1) g_msgsize = 1;

    /* Ignore SIGPIPE: if the server closes a connection mid-write, we want the
     * write() to fail with EPIPE (handled in write_full) rather than kill us. */
    signal(SIGPIPE, SIG_IGN);

    struct worker *ws = calloc((size_t)conns, sizeof(*ws));
    if (!ws)
        die("calloc(workers)");

    fprintf(stderr, "bench: %s:%d  conns=%d  msgsize=%dB  duration=%ds\n",
            g_host, g_port, conns, g_msgsize, g_seconds);

    double t0 = now_sec();
    for (int i = 0; i < conns; i++) {
        if (pthread_create(&ws[i].thread, NULL, worker_main, &ws[i]) != 0)
            die("pthread_create");
    }

    /* Sleep out the measurement window, then signal every worker to stop. We use
     * nanosleep rather than sleep() so a signal-interrupted sleep resumes for the
     * remaining time instead of ending early. */
    struct timespec dur = { .tv_sec = g_seconds, .tv_nsec = 0 };
    while (nanosleep(&dur, &dur) < 0 && errno == EINTR)
        ; /* resume the remainder stored back into `dur` */
    g_stop = 1;

    long total = 0;
    int  connected = 0;
    for (int i = 0; i < conns; i++) {
        pthread_join(ws[i].thread, NULL);
        total += ws[i].roundtrips;
        connected += ws[i].ok;
    }
    double elapsed = now_sec() - t0;

    if (connected == 0) {
        fprintf(stderr, "bench: NO connections succeeded — is the server up on %s:%d?\n",
                g_host, g_port);
        free(ws);
        return 1;
    }

    /* Report. Each round-trip is one echo (one message up, one message back). */
    printf("connections : %d (of %d requested)\n", connected, conns);
    printf("elapsed     : %.2f s\n", elapsed);
    printf("round-trips : %ld\n", total);
    printf("throughput  : %.0f echoes/sec\n", (double)total / elapsed);
    printf("             (%.1f MiB/sec each way)\n",
           (double)total * g_msgsize / elapsed / (1024.0 * 1024.0));

    free(ws);
    return 0;
}
