#include "datastructures/hash_map.h"
#include <stdlib.h>
#include <string.h>

#define TOMBSTONE ((void*)-1)

static bool should_grow(HashMap *map) {
    return map->size + map->tombstones >= (map->capacity * 3) / 4;
}

static size_t calculate_new_capacity(HashMap *map) {
    return (map->size >= (map->capacity * 3) / 8) ? map->capacity * 2 : map->capacity;
}

static KeyValue* hashmap_probe(HashMap *map, const void *key, size_t h, size_t *out_tombstone, bool *out_is_new) {
    size_t index = h % map->capacity;
    size_t first_tombstone = (size_t)-1;

    for (size_t i = 0; i < map->capacity; i++) {
        void *k = map->entries[index].key;
        if (k == NULL) {
            break;
        } else if (k == TOMBSTONE) {
            if (first_tombstone == (size_t)-1) first_tombstone = index;
        } else if (map->entries[index].hash == h && map->cmp(k, key) == 0) {
            if (out_is_new) *out_is_new = false;
            return &map->entries[index];
        }
        if (++index == map->capacity) index = 0;
    }

    if (out_tombstone) *out_tombstone = first_tombstone;
    if (out_is_new) *out_is_new = true;
    return &map->entries[index]; // Return the NULL slot or where we stopped
}

static KeyValue* reserve_slot(HashMap *map, size_t h, size_t first_tombstone, KeyValue *empty_slot) {
    size_t index;
    if (first_tombstone != (size_t)-1) {
        index = first_tombstone;
        map->tombstones--;
    } else {
        index = empty_slot - map->entries;
    }
    
    map->size++;
    map->entries[index].hash = h;
    return &map->entries[index];
}

HashMap* hashmap_create(Arena *arena, size_t initial_capacity, size_t (*hash)(const void*), int (*cmp)(const void*, const void*)) {
    if (!hash || !cmp) return NULL;
    if (initial_capacity < 8) initial_capacity = 8;
    
    HashMap *map;
    if (arena) {
        map = arena_alloc(arena, sizeof(HashMap));
    } else {
        map = calloc(1, sizeof(HashMap));
    }
    if (!map) return NULL;

    map->arena = arena;
    map->capacity = initial_capacity;
    map->size = 0;
    map->tombstones = 0;
    map->hash = hash;
    map->cmp = cmp;

    if (arena) {
        map->entries = arena_calloc(arena, sizeof(KeyValue) * initial_capacity);
    } else {
        map->entries = calloc(initial_capacity, sizeof(KeyValue));
    }

    if (!map->entries) {
        if (!arena) free(map);
        return NULL;
    }

    return map;
}

void hashmap_destroy(HashMap* map, void (*free_key)(void*), void (*free_value)(void*)) {
    if (!map) return;

    if (free_key || free_value) {
        for (size_t i = 0; i < map->capacity; i++) {
            void *k = map->entries[i].key;
            if (k != NULL && k != TOMBSTONE) {
                if (free_key) free_key(k);
                if (free_value && map->entries[i].value) free_value(map->entries[i].value);
            }
        }
    }

    if (!map->arena) {
        free(map->entries);
        free(map);
    }
}

bool hashmap_rehash(HashMap* map, size_t new_capacity) {
    if (!map || new_capacity == 0) return false;

    KeyValue *new_entries;
    if (map->arena) {
        new_entries = arena_calloc(map->arena, sizeof(KeyValue) * new_capacity);
    } else {
        new_entries = calloc(new_capacity, sizeof(KeyValue));
    }
    if (!new_entries) return false;

    for (size_t i = 0; i < map->capacity; i++) {
        void *k = map->entries[i].key;
        if (k != NULL && k != TOMBSTONE) {
            size_t h = map->entries[i].hash;
            size_t index = h % new_capacity;
            
            while (new_entries[index].key != NULL) {
                if (++index == new_capacity) index = 0;
            }
            new_entries[index] = map->entries[i];
        }
    }

    if (!map->arena) {
        free(map->entries);
    }

    map->entries = new_entries;
    map->capacity = new_capacity;
    map->tombstones = 0;
    return true;
}

KeyValue* hashmap_find_slot(HashMap *map, const void *key, size_t h, bool *is_new) {
    if (!map || !key) return NULL;
    
    size_t first_tombstone;
    KeyValue *slot = hashmap_probe(map, key, h, &first_tombstone, is_new);
    
    if (!*is_new) {
        return slot;
    }
    
    if (should_grow(map)) {
        if (hashmap_rehash(map, calculate_new_capacity(map))) {
            slot = hashmap_probe(map, key, h, &first_tombstone, is_new);
        } else if (map->size == map->capacity) {
            return NULL;
        }
    }
    
    return reserve_slot(map, h, first_tombstone, slot);
}

void hashmap_cancel_slot(HashMap *map, KeyValue *slot) {
    if (!map || !slot) return;
    map->size--;
    if (slot->key == TOMBSTONE) {
        map->tombstones++;
    }
}

bool hashmap_put(HashMap* map, const void* key, void* value) {
    if (!map || !key) return false;
    bool is_new;
    KeyValue *slot = hashmap_find_slot(map, key, map->hash(key), &is_new);
    if (!slot) return false;
    slot->key = (void*)key;
    slot->value = value;
    return true;
}

bool hashmap_has(HashMap* map, const void* key) {
    if (!map || !key) return false;
    bool is_new;
    hashmap_probe(map, key, map->hash(key), NULL, &is_new);
    return !is_new;
}

void* hashmap_get(HashMap* map, const void* key) {
    if (!map || !key) return NULL;
    bool is_new;
    KeyValue *slot = hashmap_probe(map, key, map->hash(key), NULL, &is_new);
    return is_new ? NULL : slot->value;
}

bool hashmap_remove(HashMap* map, const void* key, void (*free_key)(void*), void (*free_value)(void*)) {
    if (!map || !key) return false;
    bool is_new;
    KeyValue *slot = hashmap_probe(map, key, map->hash(key), NULL, &is_new);
    if (is_new) return false;

    if (free_key) free_key(slot->key);
    if (free_value && slot->value) free_value(slot->value);
    
    size_t index = slot - map->entries;
    size_t next_index = (index + 1 == map->capacity) ? 0 : index + 1;
    
    if (map->entries[next_index].key == NULL) {
        slot->key = NULL;
        map->size--;
    } else {
        slot->key = TOMBSTONE;
        slot->value = NULL;
        map->size--;
        map->tombstones++;
    }
    return true;
}

void hashmap_foreach(HashMap* map, void (*callback)(const void*, void*, void*), void *user) {
    if (!map || !callback) return;
    for (size_t i = 0; i < map->capacity; i++) {
        void *k = map->entries[i].key;
        if (k != NULL && k != TOMBSTONE) {
            callback(k, map->entries[i].value, user);
        }
    }
}

size_t hashmap_size(HashMap* map) {
    return map ? map->size : 0;
}
