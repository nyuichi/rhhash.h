# rhhash.h -- a Robin Hood hashing implementation in C

rhhash.h is a header-only library for C implementing Robin Hood hashing.
Robin Hood hashing is a quite clever strategy for open-addressing based hash tables.
It deals with hash collisions maintaining the variance of probe distances of hash chains.
As a result, it outperforms most open-addressing hash tables and works pretty well even when the load is very high (> 90%).
Maybe Robin Hood hashing is best known as the algorithm used in the standard hash library of Rust.

This header file provides a set of basic building blocks to implement a hash table for specific types e.g., string to int, pointer to struct.
Users are expected to implement their own hash table methods.
Functions defined in the header don't call any system calls and library functions invoking them (that is to say, no malloc/free involved!).
So it will be possible to use this library safely in say a kernel module.

A typical implementation should look like the following.
(See `sample.c` for the full source code.)

```c
struct entry {
    struct rh_head rh_head;
    char *key;
    int val;
};

struct rh_head **buckets;
int bits;

struct entry *get(const char *key) {
    struct entry *e;
    int k;
    long hash = rh_hash_str(key);
    rh_for_each_possible_entry(e, k, hash, buckets, bits, rh_head) {
        if (strcmp(e->key, key) == 0)
            return e;
    }
    return NULL;
}

void put(const char *key, int val) {
    struct entry *e;
    int k;
    long hash = rh_hash_str(key);
    rh_for_each_possible_entry(e, k, hash, buckets, bits, rh_head) {
        if (strcmp(e->key, key) == 0) {
            e->val = val;
            return;
        }
    }
    e = malloc(sizeof *e);
    e->key = strdup(key);
    e->val = val;
    INIT_RH_HEAD(&e->rh_head, hash);
    rh_add(buckets, bits, &e->rh_head);
}

void del(const char *key) {
    struct entry *e;
    int k;
    long hash = rh_hash_str(key);
    rh_for_each_possible_entry(e, k, hash, buckets, bits, rh_head) {
        if (strcmp(e->key, key) == 0) {
            free(e->key);
            free(e);
            rh_del(buckets, bits, k);
            return;
        }
    }
}
```

# License

2-clause BSD

# Author

Yuichi Nishiwaki (yuichi.nishiwaki@gmail.com)