#include "datastructures/dense_arena_interner.h"
#include <string.h>

DenseArenaInterner* intern_table_create(HashMap *hashmap, Arena *arena) {
    if (!hashmap || !arena) return NULL;

    DenseArenaInterner *interner = arena_alloc(arena, sizeof(DenseArenaInterner));
    if (!interner) return NULL;

    interner->arena = arena;
    interner->hashmap = hashmap;
    
    interner->dense_array = arena_calloc(arena, sizeof(DynArray));
    if (!interner->dense_array) {
        /* Note: returning NULL leaves the half-built interner struct in the arena */
        return NULL;
    }

    if (!dynarray_init_in_arena(interner->dense_array, arena, sizeof(InternResult*), 0)) return NULL;

    return interner;
}

void intern_table_destroy(DenseArenaInterner *interner) {
    if (!interner) return;
    hashmap_destroy(interner->hashmap, NULL, NULL);
    interner->hashmap = NULL;
    dynarray_free(interner->dense_array);
    interner->dense_array = NULL;
}

typedef struct {
    Slice key_slice;
    InternResult res;
    Entry ent;
    char data[]; // flexible array for string bytes (always null-terminated)
} InternBlock;

static InternBlock *allocate_and_init_block(DenseArenaInterner *interner, Slice *slice, void *meta) {
    InternBlock *block = arena_calloc(interner->arena, sizeof(InternBlock) + slice->len + 1);
    if (!block) return NULL;

    if (slice->ptr && slice->len > 0) {
        memcpy(block->data, slice->ptr, slice->len);
    }
    block->data[slice->len] = '\0';

    block->key_slice.ptr = block->data;
    block->key_slice.len = slice->len;

    block->ent.meta = meta;
    block->ent.dense_index = interner->dense_array->count;

    block->res.entry = &block->ent;
    block->res.key = &block->key_slice;

    return block;
}

static bool register_block(DenseArenaInterner *interner, InternResult *res, KeyValue *slot) {
    if (dynarray_push_value(interner->dense_array, &res) != 0) {
        hashmap_cancel_slot(interner->hashmap, slot);
        return false;
    }
    slot->key = res->key;
    slot->value = res;
    return true;
}

InternResult* intern(DenseArenaInterner *interner, Slice *slice, void *meta) {
    if (!interner || !slice || (!slice->ptr && slice->len > 0)) return NULL;

    bool is_new;
    size_t h = interner->hashmap->hash(slice);
    KeyValue *slot = hashmap_find_slot(interner->hashmap, slice, h, &is_new);
    if (!slot) return NULL;
    
    if (!is_new) return (InternResult*)slot->value;

    InternBlock *block = allocate_and_init_block(interner, slice, meta);
    if (!block) {
        hashmap_cancel_slot(interner->hashmap, slot);
        return NULL;
    }
    
    if (!register_block(interner, &block->res, slot)) {
        return NULL;
    }

    return &block->res;
}

InternResult* intern_peek(DenseArenaInterner *interner, Slice *slice) {
    if (!interner || !slice || (!slice->ptr && slice->len > 0)) return NULL;
    return hashmap_get(interner->hashmap, slice);
}

InternResult* interner_get_result(DenseArenaInterner *interner, size_t idx) {
    if (!interner || !interner->dense_array) return NULL;
    if (idx >= interner->dense_array->count) return NULL;
    
    InternResult **res_ptr = dynarray_get(interner->dense_array, idx);
    return res_ptr ? *res_ptr : NULL;
}

const char* interner_get_cstr(DenseArenaInterner *interner, size_t idx) {
    InternResult *res = interner_get_result(interner, idx);
    if (!res || !res->key || !((Slice*)res->key)->ptr) return NULL;
    return ((Slice*)res->key)->ptr;
}
