/* ===========================================================================
 * object.c — value objects (robj) and the doubly linked list.
 * ===========================================================================
 * Every value stored under a key is a `robj` tagged with its type. Construction
 * allocates the object plus its payload; freeObject tears both down. Ownership
 * is single: a value lives under exactly one key and is freed when that key is
 * deleted or overwritten (there is no shared-object refcounting in this core —
 * a deliberate simplification called out in the README).
 * =========================================================================== */
#include "server.h"
#include "zmalloc.h"

/* ---------------------------------------------------------------------------
 * String objects.
 * ------------------------------------------------------------------------- */

/* Copy `len` bytes into a new OBJ_STRING. Used when the source bytes are
 * borrowed (e.g. a literal) and we need our own owned copy. */
robj *createStringObject(const char *ptr, size_t len)
{
    robj *o = zmalloc(sizeof(*o));
    o->type   = OBJ_STRING;
    o->as.str = sdsnewlen(ptr, len);   /* owned copy of the bytes               */
    return o;
}

/* Wrap an already-owned sds without copying: the object ADOPTS `s`. Callers use
 * this to avoid a redundant copy when they already hold a fresh sds (e.g. the
 * value argument parsed from the client). After this call, `s` belongs to the
 * object and must not be freed by the caller. */
robj *createStringObjectFromSds(sds s)
{
    robj *o = zmalloc(sizeof(*o));
    o->type   = OBJ_STRING;
    o->as.str = s;                     /* takes ownership; no copy              */
    return o;
}

robj *createListObject(void)
{
    robj *o = zmalloc(sizeof(*o));
    o->type    = OBJ_LIST;
    o->as.list = listCreate();
    o->as.list->free = (void (*)(void *))sdsfree; /* list nodes hold owned sds  */
    return o;
}

robj *createHashObject(void)
{
    robj *o = zmalloc(sizeof(*o));
    o->type    = OBJ_HASH;
    o->as.hash = dictCreate(&hashDictType);        /* field(sds) -> value(sds)  */
    return o;
}

/* Free an object and everything it owns. The switch mirrors the union: each
 * type knows how to release its payload. */
void freeObject(robj *o)
{
    if (o == NULL) return;
    switch (o->type) {
    case OBJ_STRING: sdsfree(o->as.str);      break;
    case OBJ_LIST:   listRelease(o->as.list); break;   /* frees nodes + sds     */
    case OBJ_HASH:   dictRelease(o->as.hash); break;   /* frees fields + values */
    default:         break;                            /* unreachable            */
    }
    zfree(o);
}

/* ---------------------------------------------------------------------------
 * Doubly linked list. Elements are void*; the optional `free` destructor is
 * invoked on release. LPUSH/RPUSH map to head/tail insertion.
 * ------------------------------------------------------------------------- */
list *listCreate(void)
{
    list *l = zmalloc(sizeof(*l));
    l->head = l->tail = NULL;
    l->len  = 0;
    l->free = NULL;                    /* caller may set a destructor           */
    return l;
}

void listRelease(list *l)
{
    listNode *cur = l->head;
    while (cur) {
        listNode *next = cur->next;    /* save next BEFORE freeing cur          */
        if (l->free) l->free(cur->value);
        zfree(cur);
        cur = next;
    }
    zfree(l);
}

listNode *listAddNodeHead(list *l, void *value)
{
    listNode *node = zmalloc(sizeof(*node));
    node->value = value;
    node->prev  = NULL;
    node->next  = l->head;
    if (l->head) l->head->prev = node; /* patch old head's back-pointer         */
    else         l->tail = node;       /* list was empty: node is also the tail */
    l->head = node;
    l->len++;
    return node;
}

listNode *listAddNodeTail(list *l, void *value)
{
    listNode *node = zmalloc(sizeof(*node));
    node->value = value;
    node->next  = NULL;
    node->prev  = l->tail;
    if (l->tail) l->tail->next = node; /* patch old tail's forward-pointer      */
    else         l->head = node;       /* list was empty: node is also the head */
    l->tail = node;
    l->len++;
    return node;
}

void listDelNode(list *l, listNode *node)
{
    /* Splice `node` out by fixing its neighbours (or the head/tail handles). */
    if (node->prev) node->prev->next = node->next;
    else            l->head = node->next;
    if (node->next) node->next->prev = node->prev;
    else            l->tail = node->prev;
    if (l->free) l->free(node->value);
    zfree(node);
    l->len--;
}

/* ---------------------------------------------------------------------------
 * dictType vtables. These wire the generic dict to sds-keyed semantics and set
 * the ownership policy per table via the destructors.
 * ------------------------------------------------------------------------- */

/* Hash sds keys with the seeded MurmurHash from dict.c. */
static uint64_t sdsHash(const void *key, size_t len)
{
    return dictGenHashFunction(key, len);
}
/* dict passes us the key; we report its sds length for hashing/compare. */
static size_t sdsKeyLen(const void *key) { return sdslen((sds)key); }
/* Binary-safe key equality. */
static int sdsKeyCompare(const void *a, const void *b)
{
    return sdscmp((sds)a, (sds)b);     /* 0 == equal                            */
}
/* Free an sds key/value. */
static void sdsDestructor(void *v) { sdsfree((sds)v); }
/* Free a robj value. */
static void objDestructor(void *v) { freeObject((robj *)v); }

/* Keyspace: sds key owned by the dict, robj* value owned by the dict. */
dictType dbDictType = {
    sdsHash, sdsKeyLen, sdsKeyCompare, sdsDestructor, objDestructor
};

/* Hash-type value: sds field -> sds value, both owned by the dict. */
dictType hashDictType = {
    sdsHash, sdsKeyLen, sdsKeyCompare, sdsDestructor, sdsDestructor
};

/* expires table and pub/sub channel sets: sds key owned; the value is either an
 * integer packed into the pointer (expires) or NULL (a set), so there is NO
 * value destructor. */
dictType keyptrDictType = {
    sdsHash, sdsKeyLen, sdsKeyCompare, sdsDestructor, NULL
};

/* Free a channel's subscriber list. Note the list's own element destructor is
 * NULL, so releasing it drops the listNodes but NOT the client pointers they
 * hold — clients are owned by the server's client list, not by pub/sub. */
static void listValDestructor(void *v) { listRelease((list *)v); }

/* server.pubsub_channels: channel(sds) -> list* of subscriber clients. */
dictType pubsubDictType = {
    sdsHash, sdsKeyLen, sdsKeyCompare, sdsDestructor, listValDestructor
};
