#include <stdint.h>
#include <limits.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "murmur.h"
#include "mem.h"
#include "util.h"
#include "hashtable.h"

hashtable_t*
ht_alloc(int node, size_t entries) {
    assert(is_pow2(entries));

    hashtable_t *ht = calloc(1, sizeof(*ht));
    assert(ht);

    ht->nbuckets = 2 * entries / ENTRIES_PER_BUCKET;
    assert(is_pow2(ht->nbuckets));

    ht->buckets = alloc_node(entries * sizeof(bucket_t), node);
    assert(ht->buckets);

    for (ul b = 0; b < entries; b++) {
        ht->buckets[b].version = 0ul;
        for (ul e = 0; e < ENTRIES_PER_BUCKET; e++)
            ht->buckets[b].key[e] = INVALID_KEY;
    }

    return ht;
}

bool ht_put_hash(hashtable_t *ht, ul key, ul hash, ul value) {
    assert(ht && ht->buckets);
    assert(key > 0);

    u64 bidx = hash & (ht->nbuckets-1);
    volatile bucket_t *b = &ht->buckets[bidx];

    assert(is_even(b->version));
    b->version++;
    mmfence();

    ul idx = ULONG_MAX, inv = ULONG_MAX;
    for (ul i = 0; i < ENTRIES_PER_BUCKET; i++)
        if (b->key[i] == key)
            idx = i;
        else if (b->key[i] == INVALID_KEY)
            inv = i;

    // if exists, overwrite
    if (idx != ULONG_MAX) {
        assert(b->key[idx] == key);
        b->value[idx].v = value;
        mmfence();
        goto good;
    }
    // else if there is an empty slot, use that
    else if (inv != ULONG_MAX) {
        b->key[inv] = key;
        b->value[inv].v = value;
        mmfence();
        goto good;
    }

    // else, no space to insert. fall through
    b->version++;
    return false;

good:
    b->version++;
    return true;
}

bool ht_get_hash(hashtable_t *ht, ul key, ul hash, ul *value) {
    assert(ht && ht->buckets && value);
    assert(key > 0);

    u64 bidx;

    bidx = hash & (ht->nbuckets-1);
    bucket_t *b = &ht->buckets[bidx];
    volatile ul *version = &b->version;

    while (true) {
retry:; ul v = *version;
        if (is_odd(v))
            continue;
        for (ul i = 0; i < ENTRIES_PER_BUCKET; i++) {
            if (b->key[i] == key) {
                *value = b->value[i].v;
                if (v != *version)
                    goto retry;
                return true;
            }
        }
        if (v != *version)
            goto retry;
        return false;
    }
}

void test_hash(void) {
    volatile ul v;
    const ul iters = 1ul<<26;
    ul start = now();
    for (ul i = 0; i < iters; i++) {
        v = make_hash(i);
    }
    ul dur = now()-start;

    float sec = (float)dur / 1000000000ul;
    float ops = ((float)iters/sec) / 1e3;
    printf("kops %.2f\n", ops);
}

