// arena.c - improved, safer arena with alignment + stats
#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#include <stdbool.h>

_Static_assert(offsetof(ArenaBlock, data) % alignof(max_align_t) == 0, "ArenaBlock::data must be aligned to max_align_t");

static size_t align_up(size_t v, size_t a) {
    return (v + a - 1) & ~(a - 1);
}

static ArenaBlock *arena_allocate_block(size_t capacity) {
    size_t header = sizeof(ArenaBlock);
    if (capacity > SIZE_MAX - header) return NULL;
    ArenaBlock *block = malloc(header + capacity);
    if (!block) return NULL;

    block->next = NULL;
    block->capacity = capacity;
    block->used = 0;
    return block;
}

static bool arena_grow(Arena *arena, size_t min_capacity) {
    size_t new_capacity = arena->block_size;
    while (new_capacity < min_capacity) {
        if (new_capacity > SIZE_MAX / 2) { 
            new_capacity = min_capacity; 
            break; 
        }
        new_capacity *= 2;
    }

    ArenaBlock *new_block = arena_allocate_block(new_capacity);
    if (!new_block) return false;

    new_block->next = arena->blocks;
    arena->blocks = new_block;
    return true;
}

Arena *arena_create(size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 1024;
    if (initial_capacity > (SIZE_MAX / 2)) return NULL;

    Arena *arena = malloc(sizeof(Arena));
    if (!arena) return NULL;

    ArenaBlock *block = arena_allocate_block(initial_capacity);
    if (!block) {
        free(arena);
        return NULL;
    }

    arena->blocks = block;
    arena->block_size = initial_capacity;
    return arena;
}

void arena_destroy(Arena *arena) {
    if (!arena) return;
    ArenaBlock *b = arena->blocks;
    while (b) {
        ArenaBlock *n = b->next;
        free(b);
        b = n;
    }
    free(arena);
}

void arena_reset(Arena *arena) {
    if (!arena || !arena->blocks) return;
    ArenaBlock *b = arena->blocks;
    while (b->next) {
        ArenaBlock *n = b->next;
        free(b);
        b = n;
    }
    b->used = 0;
    arena->blocks = b;
}

void *arena_alloc(Arena *arena, size_t size) {
    if (!arena) return NULL;
    
    const size_t align = alignof(max_align_t);
    if (size > SIZE_MAX - (align - 1)) return NULL;
    size_t aligned_size = align_up(size, align);

    ArenaBlock *block = arena->blocks;
    if (!block) return NULL;

    if (aligned_size > block->capacity - block->used) {
        if (!arena_grow(arena, aligned_size)) return NULL;
        block = arena->blocks;
    }

    void *ptr = (void*)(block->data + block->used);
    block->used += aligned_size;
    return ptr;
}

void *arena_calloc(Arena *arena, size_t size) {
    void *p = arena_alloc(arena, size);
    if (!p) return NULL;
    memset(p, 0, size);
    return p;
}

size_t arena_bytes_used(const Arena *arena) {
    if (!arena) return 0;
    size_t total = 0;
    for (const ArenaBlock *b = arena->blocks; b; b = b->next) total += b->used;
    return total;
}

size_t arena_bytes_capacity(const Arena *arena) {
    if (!arena) return 0;
    size_t total = 0;
    for (const ArenaBlock *b = arena->blocks; b; b = b->next) total += b->capacity;
    return total;
}

size_t arena_block_count(const Arena *arena) {
    if (!arena) return 0;
    size_t count = 0;
    for (const ArenaBlock *b = arena->blocks; b; b = b->next) count++;
    return count;
}
