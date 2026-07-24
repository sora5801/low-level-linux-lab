/* ===========================================================================
 * pubsub.c — publish/subscribe messaging.
 * ===========================================================================
 * Two data structures cooperate:
 *
 *   server.pubsub_channels : channel(sds) -> list of subscriber clients
 *   client->pubsub_channels : set of channels THIS client is subscribed to
 *
 * They are the two directions of the same relation. SUBSCRIBE adds an edge in
 * both; UNSUBSCRIBE (and client disconnect) removes it in both. PUBLISH walks
 * the forward map (channel -> clients) and appends a "message" frame to each
 * subscriber's output buffer.
 *
 * A subtlety worth its own note: PUBLISH writes into clients OTHER than the one
 * currently being serviced. Those clients are not in a read handler right now,
 * so after queueing bytes we must arm their socket for EPOLLOUT ourselves (via
 * clientInstallWriteHandler) or the reply would sit in the buffer forever.
 * =========================================================================== */
#include "server.h"
#include "zmalloc.h"

/* Create the global channel table. Called once during server startup. */
void pubsubInit(void)
{
    server.pubsub_channels = dictCreate(&pubsubDictType);
}

/* Reply frame shared by SUBSCRIBE/UNSUBSCRIBE: a 3-element array
 *   [ <kind>, <channel-or-nil>, <subscription-count> ].  */
static void addSubscribeReply(client *c, const char *kind, sds channel,
                              long count)
{
    addReplyArrayLen(c, 3);
    addReplyBulkCString(c, kind);
    if (channel) addReplyBulkSds(c, channel);
    else         addReplyNull(c);       /* UNSUBSCRIBE with no active channels   */
    addReplyLongLong(c, count);
}

/* Subscribe `c` to `channel`. Returns 1 if this was a new subscription. */
static int subscribeChannel(client *c, sds channel)
{
    /* Client side: record the channel in the client's set. If dictAdd fails the
     * client was already subscribed, so free the duplicate we made and stop —
     * there is nothing new to wire up. */
    sds copy = sdsdup(channel);
    if (dictAdd(c->pubsub_channels, copy, NULL) != DICT_OK) {
        sdsfree(copy);
        return 0;
    }

    /* Server side: find or create the channel's subscriber list, then append
     * this client. The list stores borrowed client pointers (free method NULL). */
    list *clients = dictFetchValue(server.pubsub_channels, channel);
    if (clients == NULL) {
        clients = listCreate();                 /* free method stays NULL         */
        dictAdd(server.pubsub_channels, sdsdup(channel), clients);
    }
    listAddNodeTail(clients, c);
    return 1;
}

/* Unsubscribe `c` from `channel`. Returns 1 if a subscription was removed. */
static int unsubscribeChannel(client *c, sds channel)
{
    /* Client side: drop it from the client's set. If it wasn't there, done. */
    if (dictDelete(c->pubsub_channels, channel) != DICT_OK)
        return 0;

    /* Server side: remove this client from the channel's subscriber list. */
    list *clients = dictFetchValue(server.pubsub_channels, channel);
    if (clients) {
        listNode *node = clients->head;
        while (node) {
            if (node->value == c) { listDelNode(clients, node); break; }
            node = node->next;
        }
        /* Last subscriber gone: drop the channel entirely. dictDelete frees the
         * sds key and (via pubsubDictType's valDestructor) releases the list. */
        if (listLength(clients) == 0)
            dictDelete(server.pubsub_channels, channel);
    }
    return 1;
}

void subscribeCommand(client *c)
{
    for (int j = 1; j < c->argc; j++) {
        subscribeChannel(c, c->argv[j]);
        addSubscribeReply(c, "subscribe", c->argv[j],
                          (long)dictSize(c->pubsub_channels));
    }
}

void unsubscribeCommand(client *c)
{
    if (c->argc == 1) {
        /* UNSUBSCRIBE with no arguments: leave every channel. We repeatedly pull
         * one channel from the client's set and remove it — re-fetching each
         * iteration keeps us from mutating a structure we are iterating. */
        if (dictSize(c->pubsub_channels) == 0) {
            addSubscribeReply(c, "unsubscribe", NULL, 0);
            return;
        }
        while (dictSize(c->pubsub_channels) > 0) {
            dictEntry *de = dictGetRandomKey(c->pubsub_channels);
            sds channel = sdsdup(de->key);      /* copy: unsubscribe frees de->key*/
            unsubscribeChannel(c, channel);
            addSubscribeReply(c, "unsubscribe", channel,
                              (long)dictSize(c->pubsub_channels));
            sdsfree(channel);
        }
        return;
    }
    for (int j = 1; j < c->argc; j++) {
        unsubscribeChannel(c, c->argv[j]);
        addSubscribeReply(c, "unsubscribe", c->argv[j],
                          (long)dictSize(c->pubsub_channels));
    }
}

void publishCommand(client *c)
{
    sds channel = c->argv[1];
    sds message = c->argv[2];
    long receivers = 0;

    list *clients = dictFetchValue(server.pubsub_channels, channel);
    if (clients) {
        for (listNode *node = clients->head; node; node = node->next) {
            client *sub = (client *)node->value;
            /* Deliver a 3-element "message" frame to each subscriber. */
            addReplyArrayLen(sub, 3);
            addReplyBulkCString(sub, "message");
            addReplyBulkSds(sub, channel);
            addReplyBulkSds(sub, message);
            /* The subscriber is not in its own read handler right now, so make
             * sure the event loop will flush what we just queued. */
            clientInstallWriteHandler(sub);
            receivers++;
        }
    }
    addReplyLongLong(c, receivers);   /* PUBLISH returns the delivery count      */
}

/* On disconnect: tear down every subscription this client holds. Returns the
 * number of channels it was subscribed to. Called from freeClient. */
int pubsubUnsubscribeAll(client *c)
{
    int count = 0;
    while (dictSize(c->pubsub_channels) > 0) {
        dictEntry *de = dictGetRandomKey(c->pubsub_channels);
        sds channel = sdsdup(de->key);
        unsubscribeChannel(c, channel);         /* no reply: the client is gone   */
        sdsfree(channel);
        count++;
    }
    return count;
}
