#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "rhhash.h"

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

void clear(void) {
    struct entry *e;
    int k;
    rh_for_each_entry (e, k, buckets, bits, rh_head) {
        free(e->key);
        free(e);
        buckets[k] = NULL;
    }
}

void show(void) {
    struct entry *e;
    int k;
    rh_for_each_entry (e, k, buckets, bits, rh_head) {
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
            put(key, val);
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