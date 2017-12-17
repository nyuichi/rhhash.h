#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "hash.h"

struct entry {
    struct hash_entry entry;
    char *key;
    int val;
};

struct hash_entry **buckets;
int bits;

struct entry *get(const char *key) {
    struct entry *e;
    int k;
    long hash = hash_str(key);
    hash_for_each_possible_entry(e, k, hash, buckets, bits, entry) {
        if (strcmp(e->key, key) == 0)
            return e;
    }
    return NULL;
}

void add(const char *key, int val) {
    struct entry *e;
    int k;
    long hash = hash_str(key);
    hash_for_each_possible_entry(e, k, hash, buckets, bits, entry) {
        if (strcmp(e->key, key) == 0) {
            e->val = val;
            return;
        }
    }
    e = malloc(sizeof *e);
    e->key = strdup(key);
    e->val = val;
    INIT_HASH_ENTRY(&e->entry, hash);
    hash_add(buckets, bits, &e->entry);
}

void del(const char *key) {
    struct entry *e;
    int k;
    long hash = hash_str(key);
    hash_for_each_possible_entry(e, k, hash, buckets, bits, entry) {
        if (strcmp(e->key, key) == 0) {
            free(e->key);
            free(e);
            hash_del(buckets, bits, k);
            return;
        }
    }
}

void clear(void) {
    struct entry *e;
    int k;
    hash_for_each_entry (e, k, buckets, bits, entry) {
        free(e->key);
        free(e);
        buckets[k] = NULL;
    }
}

void show(void) {
    struct entry *e;
    int k;
    hash_for_each_entry (e, k, buckets, bits, entry) {
        printf("%s:\t\t%d\n", e->key, e->val);
    }
}

int main() {
    // Taken from course materials for "15-122 Principles of Imperative Computation" by Frank Pfenning
    bits = 10;
    int n = (1<<bits)-30;
    int num_tests = 10;

    printf("Testing array of size %d with %d values, %d times\n", (1 << bits), n, num_tests);
    for (int j = 0; j < num_tests; j++) {
        buckets = calloc(sizeof buckets[0], (1 << bits));
        char key[256];
        for (int i = 0; i < n; i++) {
            sprintf(key, "%d", j*n+i);
            int val = j*n+i;
            add(key, val);
        }
        for (int i = 0; i < n; i++) {
            sprintf(key, "%d", j*n+i);
            assert(get(key)->val == j*n+i); /* "missed existing element" */
        }
        for (int i = 0; i < n; i++) {
            sprintf(key, "%d", (j+1)*n+i);
            assert(get(key) == NULL); /* "found nonexistent element" */
        }
        for (int i = 0; i < n / 2; i++) {
            sprintf(key, "%d", j*n+i);
            del(key);
        }
        for (int i = 0; i < n / 2; i++) {
            sprintf(key, "%d", j*n+i);
            assert(get(key) == NULL); /* "found nonexistent element" */
        }
        for (int i = n / 2; i < n; i++) {
            sprintf(key, "%d", j*n+i);
            assert(get(key)->val == j*n+i); /* "missed existing element" */
        }
        clear();
        free(buckets);
    }
    printf("All tests passed!\n");
}