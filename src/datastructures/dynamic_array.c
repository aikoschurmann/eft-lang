#include "dynamic_array.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static size_t calculate_new_capacity(size_t current, size_t min_required) {
    size_t newcap = current ? current * 2 : 4;
    while (newcap > 0 && newcap < min_required) {
        newcap *= 2;
    }
    if (newcap == 0) {
        newcap = min_required; // overflowed doubling
    }
    return newcap;
}

static bool is_aliased(const DynArray *da, const void *value, size_t *out_index) {
    if (!value || da->capacity == 0) return false;
    uintptr_t v = (uintptr_t)value;
    uintptr_t start = (uintptr_t)da->data;
    uintptr_t end = start + da->capacity * da->elem_size;
    if (v >= start && v < end) {
        if (out_index) *out_index = (v - start) / da->elem_size;
        return true;
    }
    return false;
}

bool dynarray_init(DynArray *da, size_t elem_size) {
    if (!da || elem_size == 0) return false;
    da->data = NULL;
    da->elem_size = elem_size;
    da->count = 0;
    da->capacity = 0;
    da->arena = NULL;
    return true;
}

bool dynarray_init_in_arena(DynArray *da, Arena *arena, size_t elem_size, size_t initial_capacity) {
    if (!da || !arena || elem_size == 0) return false;
    da->data = NULL;
    da->elem_size = elem_size;
    da->count = 0;
    da->capacity = 0;
    da->arena = arena;
    if (initial_capacity > 0) {
        return dynarray_reserve_in_arena(da, initial_capacity) == 0;
    }
    return true;
}

void dynarray_free(DynArray *da) {
    if (!da) return;
    if (da->arena == NULL) {
        free(da->data);
    }
    da->data = NULL;
    da->count = 0;
    da->capacity = 0;
    da->elem_size = 0;
    da->arena = NULL;
}

int dynarray_reserve(DynArray *da, size_t min_capacity) {
    if (!da || da->elem_size == 0) return -1;
    if (da->capacity >= min_capacity) return 0;
    if (da->arena) return dynarray_reserve_in_arena(da, min_capacity);

    size_t newcap = calculate_new_capacity(da->capacity, min_capacity);
    if (newcap > SIZE_MAX / da->elem_size) return -1;

    void *newbuf = realloc(da->data, newcap * da->elem_size);
    if (!newbuf) return -1;
    
    da->data = newbuf;
    da->capacity = newcap;
    return 0;
}

int dynarray_reserve_in_arena(DynArray *da, size_t min_capacity) {
    if (!da || !da->arena || da->elem_size == 0) return -1;
    if (da->capacity >= min_capacity) return 0;

    size_t newcap = calculate_new_capacity(da->capacity, min_capacity);
    if (newcap > SIZE_MAX / da->elem_size) return -1;

    void *newbuf = arena_alloc(da->arena, newcap * da->elem_size);
    if (!newbuf) return -1;

    if (da->data && da->count > 0) {
        memcpy(newbuf, da->data, da->count * da->elem_size);
    }

    da->data = newbuf;
    da->capacity = newcap;
    return 0;
}

int dynarray_push_value(DynArray *da, const void *value) {
    if (!da) return -1;

    size_t alias_index = 0;
    bool aliased = is_aliased(da, value, &alias_index);

    if (dynarray_reserve(da, da->count + 1) != 0) return -1;

    if (aliased) {
        value = (const char *)da->data + alias_index * da->elem_size;
    }

    void *dst = (char*)da->data + da->count * da->elem_size;
    da->count += 1;
    
    if (value) {
        memcpy(dst, value, da->elem_size);
    } else {
        memset(dst, 0, da->elem_size);
    }
    return 0;
}

void *dynarray_push_uninit(DynArray *da) {
    if (!da) return NULL;
    if (dynarray_reserve(da, da->count + 1) != 0) return NULL;
    
    void *slot = (char*)da->data + da->count * da->elem_size;
    da->count += 1;
    return slot;
}

void dynarray_pop(DynArray *da) {
    if (!da || da->count == 0) return;
    da->count -= 1;
}

void dynarray_remove(DynArray *da, size_t index) {
    if (!da || index >= da->count) return;
    
    if (index < da->count - 1) {
        void *dst = (char*)da->data + index * da->elem_size;
        void *src = (char*)da->data + (index + 1) * da->elem_size;
        size_t n = da->count - index - 1;
        memmove(dst, src, n * da->elem_size);
    }
    da->count -= 1;
}

void *dynarray_get(DynArray *da, size_t index) {
    if (!da || index >= da->count) return NULL;
    return (char*)da->data + index * da->elem_size;
}

const void *dynarray_get_const(const DynArray *da, size_t index) {
    if (!da || index >= da->count) return NULL;
    return (const char*)da->data + index * da->elem_size;
}

int dynarray_set(DynArray *da, size_t index, const void *value) {
    if (!da || !value || index >= da->count) return -1;
    void *dst = (char*)da->data + index * da->elem_size;
    memcpy(dst, value, da->elem_size);
    return 0;
}
