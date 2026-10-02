#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "arena.h"
#include "core/utils.h"

/* -----------------------------
   HashMap API (Open Addressing)
   ----------------------------- */

typedef struct {
    void *key;
    void *value;
    uint64_t hash;         /* cached hash of the key */
} KeyValue;

typedef struct {
    Arena *arena;          /* If NULL, uses malloc/free */
    KeyValue *entries;     /* Contiguous array of KeyValue pairs */
    size_t capacity;       /* Number of slots in entries */
    size_t size;           /* Number of active elements */
    size_t tombstones;     /* Number of deleted slots */
    size_t (*hash)(const void*); /* Hash function */
    int (*cmp)(const void*, const void*); /* Compare function */
} HashMap;

/* Constructor / Destructor */
HashMap* hashmap_create(Arena *arena, size_t initial_capacity, size_t (*hash)(const void*), int (*cmp)(const void*, const void*));

void hashmap_destroy(
    HashMap* map,
    void (*free_key)(void*),
    void (*free_value)(void*)
);

/* Insert or update */
bool hashmap_put(
    HashMap* map,
    const void* key,
    void* value
);

/* Lookup */
void hashmap_cancel_slot(HashMap *map, KeyValue *slot);
KeyValue* hashmap_find_slot(HashMap *map, const void *key, size_t hash_val, bool *out_is_new);
bool hashmap_has(HashMap *map, const void *key);
void* hashmap_get(
    HashMap* map,
    const void* key
);

/* Remove */
bool hashmap_remove(
    HashMap* map,
    const void* key,
    void (*free_key)(void*),
    void (*free_value)(void*)
);

/* Resize / Rehash */
bool hashmap_rehash(
    HashMap* map,
    size_t new_capacity
);

/* Utility */
size_t hashmap_size(HashMap* map);

void hashmap_foreach(
    HashMap* map,
    void (*func)(const void* key, void* value, void* user),
    void *user
);
