/* stress_test.c - model-based stress tests for arena / dynarray / hashmap / interner.
 *
 * Build (Linux, gcc or clang; adjust -I so the includes below resolve):
 *   gcc -std=c11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer \
 *       -Wall -Wextra -Wl,--wrap=malloc -I. \
 *       stress_test.c arena.c dynamic_array.c hash_map.c dense_arena_interner.c -o stress
 *   ./stress
 *
 * -Wl,--wrap=malloc lets the OOM test make malloc fail on purpose. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>
#include <stdbool.h>

#include "arena.h"
#include "dynamic_array.h"
#include "datastructures/hash_map.h"
#include "dense_arena_interner.h"

#include "framework.h"
#define CHECK(c) TEST_ASSERT(c)

/* ---------- deterministic PRNG ---------- */
static uint64_t rng_s = 0x9E3779B97F4A7C15ull;
static uint64_t rnd(void) {
    rng_s ^= rng_s << 13; rng_s ^= rng_s >> 7; rng_s ^= rng_s << 17;
    return rng_s;
}

/* ---------- malloc fault injection (needs -Wl,--wrap=malloc) ---------- */
extern void *__real_malloc(size_t);
static unsigned g_fail_every = 0;
static unsigned g_calls = 0;


#include <dlfcn.h>
void *malloc(size_t size) {
    if (g_fail_every && (++g_calls % g_fail_every) == 0) return NULL;
    static void* (*real_malloc)(size_t) = NULL;
    if (!real_malloc) real_malloc = dlsym(RTLD_NEXT, "malloc");
    return real_malloc(size);
}
void *calloc(size_t n, size_t size) {
    static void* (*real_calloc)(size_t, size_t) = NULL;
    if (!real_calloc) real_calloc = dlsym(RTLD_NEXT, "calloc");
    return real_calloc(n, size);
}
void *realloc(void *ptr, size_t size) {
    static void* (*real_realloc)(void*, size_t) = NULL;
    if (!real_realloc) real_realloc = dlsym(RTLD_NEXT, "realloc");
    return real_realloc(ptr, size);
}

/* ---------- Slice hash / cmp (key is a Slice*) ---------- */
/* slice_hash removed, using utils.h */
/* slice_cmp removed, using utils.h */

/* ---------- arena ---------- */
static void test_arena(void) {
    Arena *a = arena_create(512);
    CHECK(a);
    enum { N = 20000 };
    struct { unsigned char *p; size_t n; unsigned char tag; } *rec = malloc(N * sizeof *rec);
    CHECK(rec);

    for (int i = 0; i < N; i++) {
        size_t n = rnd() % 5000;                        /* includes 0 */
        unsigned char *p = arena_alloc(a, n);
        CHECK(p);
        CHECK(((uintptr_t)p % alignof(max_align_t)) == 0);
        rec[i].p = p; rec[i].n = n; rec[i].tag = (unsigned char)(i * 31 + 7);
        memset(p, rec[i].tag, n);
    }
    for (int i = 0; i < N; i++)                         /* detects overlapping allocations */
        for (size_t j = 0; j < rec[i].n; j++) CHECK(rec[i].p[j] == rec[i].tag);

    CHECK(arena_alloc(a, SIZE_MAX) == NULL);
    CHECK(arena_alloc(a, SIZE_MAX - 8) == NULL);
    CHECK(arena_bytes_used(a) <= arena_bytes_capacity(a));
    CHECK(arena_block_count(a) > 1);

    arena_reset(a);
    CHECK(arena_bytes_used(a) == 0);
    CHECK(arena_block_count(a) == 1);

    unsigned char *dirty = arena_alloc(a, 256);         /* calloc must zero recycled memory */
    CHECK(dirty);
    memset(dirty, 0xAB, 256);
    arena_reset(a);
    unsigned char *z = arena_calloc(a, 256);
    CHECK(z);
    for (int i = 0; i < 256; i++) CHECK(z[i] == 0);

    arena_destroy(a);
    free(rec);
}

/* ---------- dynarray ---------- */
static void dynarray_exercise(DynArray *da) {
    for (int i = 0; i < 100000; i++) {
        int v = i;
        CHECK(dynarray_push_value(da, &v) == 0);
    }
    CHECK(da->count == 100000);
    for (int i = 0; i < 100000; i++) CHECK(*(int *)dynarray_get(da, i) == i);
    CHECK(dynarray_get(da, 100000) == NULL);

    /* push a pointer that points INTO the array, across growth boundaries */
    *(int *)dynarray_get(da, 0) = 42;
    size_t start = da->count;
    for (int i = 0; i < 5000; i++) {
        CHECK(dynarray_push_value(da, dynarray_get(da, 0)) == 0);
    }
    for (size_t i = start; i < da->count; i++) CHECK(*(int *)dynarray_get(da, i) == 42);

    dynarray_pop(da);
    CHECK(da->count == start + 4999);
    dynarray_remove(da, 1);
    CHECK(*(int *)dynarray_get(da, 1) == 2);
    CHECK(*(const int *)dynarray_get_const(da, 1) == 2);
}

static void test_dynarray(void) {
    DynArray h;
    CHECK(dynarray_init(&h, sizeof(int)));
    dynarray_exercise(&h);
    dynarray_free(&h);
    CHECK(dynarray_push_value(&h, &(int){1}) != 0);     /* push-after-free errors, no SIGFPE */

    Arena *a = arena_create(1024);
    DynArray d;
    CHECK(dynarray_init_in_arena(&d, a, sizeof(int), 0));
    dynarray_exercise(&d);
    dynarray_free(&d);
    arena_destroy(a);

    DynArray bad;
    CHECK(!dynarray_init(&bad, 0));
}

/* ---------- hash map vs. reference model (heap-backed so ASan watches it) ---------- */
#define KEYS 4096
static int g_keys[KEYS];
static size_t int_hash(const void *p) {
    uint64_t x = (uint64_t)*(const int *)p;
    x *= 0x9E3779B97F4A7C15ull; x ^= x >> 32;
    return (size_t)x;
}
static int int_cmp(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}
static void count_cb(const void *k, void *v, void *user) { (void)k; (void)v; (*(size_t *)user)++; }

static void test_hashmap_model(void) {
    for (int i = 0; i < KEYS; i++) g_keys[i] = i;
    HashMap *m = hashmap_create(NULL, 8, int_hash, int_cmp);
    CHECK(m);

    static bool present[KEYS];
    static uintptr_t model[KEYS];
    size_t live = 0;

    for (long op = 0; op < 2000000; op++) {
        int k = (int)(rnd() % KEYS);
        switch (rnd() % 10) {
        case 0: case 1: case 2: case 3: case 4: {
            uintptr_t v = (rnd() % 1000000) + 1;        /* non-zero: NULL means "missing" */
            CHECK(hashmap_put(m, &g_keys[k], (void *)v));
            if (!present[k]) live++;
            present[k] = true; model[k] = v;
            break;
        }
        case 5: case 6: {
            bool r = hashmap_remove(m, &g_keys[k], NULL, NULL);
            CHECK(r == present[k]);
            if (present[k]) live--;
            present[k] = false;
            break;
        }
        default: {
            void *v = hashmap_get(m, &g_keys[k]);
            CHECK((uintptr_t)v == (present[k] ? model[k] : 0));
            CHECK(hashmap_has(m, &g_keys[k]) == present[k]);
        }
        }
        if (op % 1000 == 0) CHECK(hashmap_size(m) == live);
    }
    for (int k = 0; k < KEYS; k++)
        CHECK((uintptr_t)hashmap_get(m, &g_keys[k]) == (present[k] ? model[k] : 0));
    size_t n = 0;
    hashmap_foreach(m, count_cb, &n);
    CHECK(n == live && hashmap_size(m) == live);
    hashmap_destroy(m, NULL, NULL);
}

/* ---------- interner ---------- */
static size_t make_str(char *buf, size_t cap, unsigned id) {
    int n = snprintf(buf, cap, "id%u", id);
    size_t len = (size_t)n;
    if (id % 7 == 0) {                                  /* some long strings */
        size_t extra = id % 200;
        if (len + extra + 1 > cap) extra = cap - len - 1;
        memset(buf + len, 'x', extra);
        len += extra;
        buf[len] = '\0';
    }
    return len;
}

static void test_interner_stress(void) {
    enum { IDS = 200000, OPS = 600000 };
    Arena *a = arena_create(1 << 16);
    HashMap *m = hashmap_create(a, 16, slice_hash, slice_cmp);
    DenseArenaInterner *I = intern_table_create(m, a);
    CHECK(a && m && I);

    size_t *idx_of = malloc(IDS * sizeof *idx_of);
    InternResult **res_of = calloc(IDS, sizeof *res_of);
    CHECK(idx_of && res_of);
    for (size_t i = 0; i < IDS; i++) idx_of[i] = SIZE_MAX;

    size_t count = 0;
    char buf[512];
    for (long i = 0; i < OPS; i++) {
        unsigned id = (unsigned)(rnd() % IDS);
        size_t n = make_str(buf, sizeof buf, id);
        Slice s = { .ptr = buf, .len = n };
        InternResult *r;
        if (idx_of[id] == SIZE_MAX) {
            r = intern(I, &s, (void *)(uintptr_t)(id + 1));
            CHECK(r);
            CHECK(r->entry->dense_index == count);      /* indices are dense, in insertion order */
            idx_of[id] = count; res_of[id] = r; count++;
        } else {
            r = intern(I, &s, (void *)0xdead);          /* different meta must be ignored */
            CHECK(r == res_of[id]);
            CHECK(r->entry->dense_index == idx_of[id]);
        }
        CHECK(r->entry->meta == (void *)(uintptr_t)(id + 1));
        memset(buf, '#', sizeof buf);                   /* canonical copy must not alias caller's buffer */
        if (i % 50000 == 0) CHECK(I->dense_array->count == count);
    }
    CHECK(I->dense_array->count == count);
    CHECK(hashmap_size(m) == count);

    for (unsigned id = 0; id < IDS; id++) {
        if (idx_of[id] == SIZE_MAX) continue;
        size_t n = make_str(buf, sizeof buf, id);
        Slice s = { .ptr = buf, .len = n };
        CHECK(intern_peek(I, &s) == res_of[id]);
        CHECK(interner_get_result(I, idx_of[id]) == res_of[id]);
        const char *c = interner_get_cstr(I, idx_of[id]);
        CHECK(c && strlen(c) == n && memcmp(c, buf, n) == 0);
        const Slice *ks = (const Slice *)res_of[id]->key;
        CHECK(ks->len == n && (const void *)ks->ptr == (const void *)c);
    }
    CHECK(interner_get_cstr(I, count) == NULL);
    CHECK(interner_get_result(I, count) == NULL);

    char nope[] = "never_seen_string";                  /* peek must not insert */
    Slice ns = { .ptr = nope, .len = strlen(nope) };
    CHECK(intern_peek(I, &ns) == NULL);
    CHECK(I->dense_array->count == count);

    arena_destroy(a);
    free(idx_of); free(res_of);
}

static void test_interner_edge_cases(void) {
    Arena *a = arena_create(1024);
    HashMap *m = hashmap_create(a, 8, slice_hash, slice_cmp);
    DenseArenaInterner *I = intern_table_create(m, a);
    CHECK(I);

    /* empty slices: NULL ptr and "" ptr must be the same entry */
    Slice e0 = { .ptr = NULL, .len = 0 };
    Slice e1 = { .ptr = "", .len = 0 };
    InternResult *r0 = intern(I, &e0, NULL);
    InternResult *r1 = intern(I, &e1, NULL);
    CHECK(r0 && r0 == r1);
    CHECK(intern_peek(I, &e0) == r0);
    const char *c = interner_get_cstr(I, r0->entry->dense_index);
    CHECK(c && c[0] == '\0');

    /* embedded NULs and prefixes are distinct keys */
    char b1[] = { 'a', '\0', 'b' }, b2[] = { 'a', '\0', 'c' }, b3[] = { 'a' };
    Slice s1 = { .ptr = b1, .len = 3 }, s2 = { .ptr = b2, .len = 3 }, s3 = { .ptr = b3, .len = 1 };
    InternResult *x1 = intern(I, &s1, NULL), *x2 = intern(I, &s2, NULL), *x3 = intern(I, &s3, NULL);
    CHECK(x1 && x2 && x3 && x1 != x2 && x1 != x3 && x2 != x3);
    CHECK(intern(I, &s1, NULL) == x1);

    /* invalid input */
    Slice bad = { .ptr = NULL, .len = 5 };
    CHECK(intern(I, &bad, NULL) == NULL);
    CHECK(intern(NULL, &s1, NULL) == NULL);

    arena_destroy(a);
}

/* Make malloc fail repeatedly. After every failure the interner must be exactly
 * as it was: dense count == map size == number of successes, no gaps in indices. */
static void test_interner_oom(void) {
    Arena *a = arena_create(256);                       /* small blocks -> frequent malloc */
    HashMap *m = hashmap_create(a, 8, slice_hash, slice_cmp);
    DenseArenaInterner *I = intern_table_create(m, a);
    CHECK(I);

    enum { N = 5000 };
    size_t count = 0, failures = 0;
    char buf[32];

    g_calls = 0; g_fail_every = 3;
    for (unsigned id = 0; id < N; id++) {
        size_t n = (size_t)snprintf(buf, sizeof buf, "oom_%u", id);
        Slice s = { .ptr = buf, .len = n };
        InternResult *r = NULL;
        for (int attempt = 0; attempt < 8 && !r; attempt++) {
            r = intern(I, &s, NULL);
            if (!r) {
                failures++;
                CHECK(I->dense_array->count == count);
                CHECK(hashmap_size(m) == count);
            }
        }
        CHECK(r);
        CHECK(r->entry->dense_index == count);
        count++;
        CHECK(I->dense_array->count == count);
        CHECK(hashmap_size(m) == count);
    }
    g_fail_every = 0;
    CHECK(failures > 0);                                /* the injection actually fired */

    for (unsigned id = 0; id < N; id++) {
        size_t n = (size_t)snprintf(buf, sizeof buf, "oom_%u", id);
        Slice s = { .ptr = buf, .len = n };
        InternResult *r = intern_peek(I, &s);
        CHECK(r && r->entry->dense_index == id);
        const char *c = interner_get_cstr(I, id);
        CHECK(c && strcmp(c, buf) == 0);
    }
    printf("  oom: %zu injected failures survived\n", failures);
    arena_destroy(a);
}

void suite_datastructures(void);
void suite_datastructures(void) {
    RUN_TEST(test_arena);
    RUN_TEST(test_dynarray);
    RUN_TEST(test_hashmap_model);
    RUN_TEST(test_interner_edge_cases);
    RUN_TEST(test_interner_stress);
    RUN_TEST(test_interner_oom);
}