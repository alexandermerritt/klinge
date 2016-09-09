#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "murmur.h"
#include "mem.h"
#include "common.h"

#define INVALID_KEY 0ul

#define FNV_OFFSET_BASIS_64     14695981039346656037ul
#define FNV_PRIME_64            1099511628211ul

#define ENTRIES_PER_BUCKET  4

// 4 entries per bucket_t, each 16 bytes
// zero key is empty for simplicity
struct bucket {
    u64 version;
    u64 key[ENTRIES_PER_BUCKET];
    // pack the values (they are virtual addresses, which are limited
    // to 48-bits on x86) so we can squeeze 4 entries into a cacheline
    struct { ul v : 48; } __attribute__((packed)) value[4];
} __ALIGN64PACKED;
typedef struct bucket bucket_t;

struct hashtable {
    bucket_t *buckets;
    size_t nbuckets;
};
typedef struct hashtable hashtable_t;

static inline
u64 fnv(u64 val) {
    u64 hash = FNV_OFFSET_BASIS_64;
    uint8_t *p = (uint8_t*)&val;
    for (ul i = 0; i < 8; i++)
        hash = (hash ^ p[i]) * FNV_PRIME_64;
    return hash;
}

static inline
uint64_t m3(u64 val) {
    uint64_t buf[2];
    MurmurHash3_x64_128(&val, sizeof(val),
            FNV_OFFSET_BASIS_64 & ((1ul<<32)-1), buf);
    return buf[0] ^ buf[1];
}

static inline
uint64_t make_hash(u64 val) {
    //return m3(val); // slower than FNV by 2x
    return fnv(val);
}

static inline
ul table_idx(u64 hash, ul ntables) {
    return (hash >> 48) & (ntables-1);
}

hashtable_t* ht_alloc(int node, size_t entries);
bool ht_put_hash(hashtable_t *ht, ul key, ul hash, ul value);
bool ht_get_hash(hashtable_t *ht, ul key, ul hash, ul *value);

