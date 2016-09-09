#pragma once

#include <sched.h>

#include <assert.h>
#include <time.h>
#include <stdlib.h>

#include "hashtable.h"
#include "common.h"

#define __ALIGN64       __attribute__((aligned(64)))
#define __ALIGN64PACKED __attribute__((packed,aligned(64)))
#define __UNUSED        __attribute__((unused))

static inline
uint64_t rdtscp(void)
{
    uint64_t rax = 0, rdx = 0;
    __asm__ volatile ("rdtscp"
            : "=a" (rax), "=d" (rdx)
            : : );
    return ((rdx << 32) + rax);
}

static inline
bool is_pow2(ul value) {
    return 1 == __builtin_popcountl(value);
}

static inline
size_t roundup_align(size_t len, size_t align) {
    assert(is_pow2(align));
    return (len + align - 1) / align;
}

static inline
bool is_even(ul v) {
    return 0x0 == (v & 0x1);
}

static inline
bool is_odd(ul v) {
    return !is_even(v);
}

static inline
ul now() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (t.tv_sec*1000000000ul + t.tv_nsec);
}

// knuth
static inline
void shuffle(ul *idx, ul n) {
    for (ul _x = 0; _x < 2; _x++) {
        for (ul i = 0; i < n-1; i++) {
            ul v = idx[i];
            ul ii = (i+1) + (lrand48() % (n-(i+1)));
            idx[i] = idx[ii];
            idx[ii] = v;
        }
    }
}

static inline
void shuffle_u32(u32 *idx, ul n) {
    for (ul _x = 0; _x < 2; _x++) {
        for (ul i = 0; i < n-1; i++) {
            ul v = idx[i];
            ul ii = (i+1) + (lrand48() % (n-(i+1)));
            idx[i] = idx[ii];
            idx[ii] = v;
        }
    }
}

static inline
void shuffle_u8(u8 *idx, ul n) {
    for (ul _x = 0; _x < 2; _x++) {
        for (ul i = 0; i < n-1; i++) {
            ul v = idx[i];
            ul ii = (i+1) + (lrand48() % (n-(i+1)));
            idx[i] = idx[ii];
            idx[ii] = v;
        }
    }
}

static inline
void atomic_store(ul *where, ul value) {
    __atomic_store_n(where, value, __ATOMIC_SEQ_CST);
}

static inline
ul atomic_load(ul *where) {
    return __atomic_load_n(where, __ATOMIC_SEQ_CST);
}

static inline
ul atomic_sub_fetch(ul *where, ul value) {
    return __atomic_sub_fetch(where, value, __ATOMIC_SEQ_CST);
}

static inline
ul atomic_add_fetch(ul *where, ul value) {
    return __atomic_add_fetch(where, value, __ATOMIC_SEQ_CST);
}

static inline
bool atomic_tas(uint8_t *where) {
    return __atomic_test_and_set(where, __ATOMIC_ACQUIRE);
}

static inline
void atomic_clear(uint8_t *where) {
    __atomic_clear(where, __ATOMIC_RELEASE);
}

static inline
void pin_cpu(int cpu) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    assert( 0 == sched_setaffinity(0, /*me*/
                sizeof(mask), &mask) );
}

