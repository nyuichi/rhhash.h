/*-
 * rhhash.h -- a Robin Hood hashing implementation in C
 * Copyright (c) 2017 Yuichi Nishiwaki
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _RHHASH_H_
#define _RHHASH_H_

#include <stddef.h>
#include <limits.h>
#include <stdbool.h>

#undef container_of
#define container_of(ptr,type,field) ((type *) ((char *) (ptr) - offsetof(type, field)))

#undef typecheck
#define typecheck(type,var) ({ typeof(var) *__tmp; __tmp = (type *) NULL; })

struct rh_head {
    long hash;
};

static inline void INIT_RH_HEAD(struct rh_head *e, long hash) {
    e->hash = hash;
}

static int __rh_index(struct rh_head **buckets, int bits, long hash) {
    int size = 1 << bits, mask = size - 1;
    int init = hash & mask;
    int k;
    for (int i = 0, dib = 0; ; ++i, ++dib) {
        k = (init + i) & mask;
        struct rh_head *e_k = buckets[k];
        if (! e_k)
            break;
        int init_k = e_k->hash & mask;
        if (init_k == init)
            break;
        int dib_k = (k + size - init_k) & mask;
        if (dib_k < dib)
            break;
    }
    return k;
}

static int __rh_prove(struct rh_head **buckets, int bits, long hash, int index) {
    int size = 1 << bits, mask = size - 1;
    int k;
    for (int i = 0; ; ++i) {
        k = (index + i) & mask;
        struct rh_head *e_k = buckets[k];
        if (! e_k)
            break;
        long hash_k = e_k->hash;
        if (hash_k == hash)
            break;
        if ((hash_k & mask) != (hash & mask))
            break;
    }
    return k;
}

static inline int rh_get(struct rh_head **buckets, int bits, long hash) {
    return __rh_prove(buckets, bits, hash, __rh_index(buckets, bits, hash));
}

static inline int rh_next(struct rh_head **buckets, int bits, long hash, int index) {
    return __rh_prove(buckets, bits, hash, index + 1);
}

static inline bool rh_exist(struct rh_head **buckets, int bits, long hash, int index) {
    struct rh_head *e = buckets[index];
    return e && e->hash == hash;
}

static void __rh_reserve(struct rh_head **buckets, int bits, int index) {
    int size = 1 << bits, mask = size - 1;
    struct rh_head *e = NULL;
    for (int i = 0, dib = INT_MAX; i < size; ++i, ++dib) {
        int k = (index + i) & mask;
        struct rh_head *e_k = buckets[k];
        if (! e_k) {
            buckets[k] = e;
            return;
        }
        int init_k = e_k->hash & mask;
        int dib_k = (k + size - init_k) & mask;
        if (dib_k < dib) {
            buckets[k] = e;
            e = e_k;
            dib = dib_k;
        }
    }
    buckets[index] = e; // buckets are full, but keep the state of the table consistent
}

static inline void rh_add(struct rh_head **buckets, int bits, struct rh_head *e) {
    // REQUIRE(buckets has empty slots)
    int index = __rh_index(buckets, bits, e->hash);
    __rh_reserve(buckets, bits, index);
    buckets[index] = e;
}

static void rh_del(struct rh_head **buckets, int bits, int index) { // delete by backward shifting
    int size = 1 << bits, mask = size - 1;
    for (int i = 0; ; ++i) {
        int k_prev = (i + index) & mask;
        int k = (i + index + 1) & mask;
        struct rh_head *e_k = buckets[k];
        if (! e_k) {
            buckets[k_prev] = NULL;
            break;
        }
        int init_k = e_k->hash & mask;
        int dib_k = (k + size - init_k) & mask;
        if (dib_k == 0) {
            buckets[k_prev] = NULL;
            break;
        }
        buckets[k_prev] = e_k;
    }
}

#define	rh_entry(ptr,type,field) (typecheck(struct rh_head *, ptr), container_of(ptr, type, field))

#define rh_for_each_possible(k,hash,buckets,bits) \
    for (k = rh_get((buckets), (bits), (hash)); rh_exist((buckets), (bits), (hash), k); k = rh_next((buckets), (bits), (hash), k))

#define rh_for_each_possible_entry(e,k,hash,buckets,bits,field) \
    rh_for_each_possible(k,hash,buckets,bits) \
        if ((e = rh_entry(buckets[k], typeof(*e), field)), 1)

#define rh_for_each(k,buckets,bits) \
    for (k = 0; k < (1 << (bits)); ++k) \
        if (buckets[k])

#define rh_for_each_entry(e,k,buckets,bits,field) \
    rh_for_each(k,buckets,bits) \
        if ((e = rh_entry(buckets[k], typeof(*e), field)), 1)

long rh_hash_str(const char *str) { // djb2
    long hash = 5381, c;
    while ((c = (unsigned) *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

long rh_hash_int(int i) { // Thomas Wang's 32bit hash
    unsigned int key = i;
    key = ~key + (key << 15); // key = (key << 15) - key - 1;
    key = key ^ (key >> 12);
    key = key + (key << 2);
    key = key ^ (key >> 4);
    key = key * 2057; // key = (key + (key << 3)) + (key << 11);
    key = key ^ (key >> 16);
    return (long) key; // upper bits will be zero, but sufficient for our purpose
}

long rh_hash_long(long l) { // Thomas Wang's 64bit hash
    unsigned long key = l;
    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8); // key * 265
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4); // key * 21
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return key;
}

long rh_hash_ptr(void *ptr) {
    return rh_hash_long((long) ptr);
}

static inline bool rh_upper_bound(int count, int bits) {
    return count > ((1 << bits) * 29 + 29) / 32;
}

static void rh_copy(struct rh_head **new_buckets, int new_bits, struct rh_head **old_buckets, int old_bits) {
    int k;
    rh_for_each (k, old_buckets, old_bits) {
        struct rh_head *e = old_buckets[k];
        rh_add(new_buckets, new_bits, e);
    }
}

#endif