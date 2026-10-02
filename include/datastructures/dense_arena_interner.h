#pragma once

#include "arena.h"
#include "hash_map.h"
#include "dynamic_array.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dense interner that stores canonical keys in a backing Arena and
 *        provides fast dense-index enumeration.
 *
 * The interner stores:
 *  - an Arena* (all keys and interner structures live in the arena),
 *  - a HashMap* for fast lookup (keys are pointers into the arena),
 *  - a DynArray* of InternResult* pointers in dense order.
 *
 * Note: This implementation assumes arena allocations are never individually freed.
 */
typedef struct {
    Arena *arena;
    HashMap *hashmap;
    DynArray *dense_array;
    
    /** Copy function that makes the canonical (arena-allocated) copy. */
    void* (*copy_func)(Arena *arena, const void *data, size_t len);
    
    /** Hash function for keys (receives pointer to key as void*). */
    size_t (*hash_func)(void *key);
    /** Comparison function for keys: 0 -> equal, non-zero -> not equal. */
    int (*cmp_func)(void *a, void *b);
} DenseArenaInterner;

/**
 * @brief Metadata stored for each interned element.
 */
typedef struct {
    void* meta;      /**< user-provided metadata */
    size_t dense_index; /**< dense index assigned when interned (0-based) */
} Entry;

/**
 * @brief Result returned when an element is interned or found.
 *
 * - key: canonical pointer (arena-owned)
 * - entry: pointer to Entry (arena-owned)
 */
typedef struct {
    void *key;
    Entry *entry;
} InternResult;


/* --- Copy function type & predefined implementations --- */

/**
 * @brief Copy function signature: returns an arena-allocated canonical copy.
 *
 * @param arena where the canonical copy should be allocated
 * @param data  source bytes
 * @param len   length of the data (bytes)
 * @return pointer to arena-allocated copy (NULL on allocation failure)
 */
/**
 * @brief Copy bytes and null-terminate (useful for C-strings).
 *
 * Allocates len+1 bytes in the arena, copies len bytes and appends '\0'.
 */
/**
 * @brief Copy bytes exactly (no null terminator).
 */

/* --- Construction / destruction --- */

/**
 * @brief Create an interner that stores all allocations inside `arena`.
 *
 * @param hashmap  Pointer to a HashMap instance (must be initialized)
 * @param arena    Arena where keys and intern structures will be allocated
 * @param copy_func function to produce canonical copies into the arena
 * @param hash_func hash function for keys (pointer passed to hash)
 * @param cmp_func  comparison function for keys
 * @return pointer to newly created DenseArenaInterner or NULL on error
 */
/**
 * @brief Create an interner that stores all allocations inside `arena`.
 * Takes ownership of the provided hashmap (it will be destroyed when the interner is destroyed).
 */
DenseArenaInterner* intern_table_create(HashMap *hashmap, Arena *arena);

/**
 * @brief Destroy the interner and the underlying hashmap.
 *
 * Note: Because the interner uses arena allocations, freeing individual keys
 * via free_key/free_value is rarely needed. Pass NULL for free_key/free_value
 * if you used arena allocations for keys/values.
 *
 * @param interner the interner to destroy
 * @param free_key  optional function to free keys (or NULL)
 * @param free_value optional function to free values (or NULL)
 */
void intern_table_destroy(DenseArenaInterner *interner);


/* --- Interning / lookup --- */

/**
 * @brief Intern the slice; returns an InternResult* on success, or NULL on failure.
 *
 * If the key already exists the existing InternResult* is returned (no insertion).
 *
 * @param interner the interner
 * @param slice    pointer+length describing the key to intern (non-null)
 * @param meta     user metadata associated with this key (stored in Entry)
 * @return InternResult* (arena-owned) or NULL on allocation/error
 */
InternResult *intern(DenseArenaInterner *interner, Slice *slice, void* meta);

/**
 * @brief Return canonical key pointer (arena-owned) for given slice (interns if needed).
 *
 * @return pointer to the canonical key (void*) or NULL on error.
 */
void *intern_ptr(DenseArenaInterner *interner, Slice *slice, void* meta);

/**
 * @brief Return dense index for given slice (interns if needed).
 *
 * @return dense index (>=0) or -1 on error.
 */
size_t intern_idx(DenseArenaInterner *interner, Slice *slice, void* meta);

/**
 * @brief Lookup without inserting. Returns existing InternResult* or NULL.
 */
InternResult* intern_peek(DenseArenaInterner *interner, Slice *slice);


/* --- Accessors --- */

/**
 * @brief Return canonical C string (null-terminated) for dense index, or NULL if invalid.
 *
 * The returned pointer is arena-owned and stable for the lifetime of the arena.
 */
const char *interner_get_cstr(DenseArenaInterner *interner, size_t idx);

/**
 * @brief Return the InternResult pointer for a given dense index.
 * 
 * @param interner The interner.
 * @param idx The dense index.
 * @return InternResult* The intern result at that index, or NULL if invalid.
 */
InternResult *interner_get_result(DenseArenaInterner *interner, size_t idx);


/**
 * @brief Iterate over all interned entries in dense order.
 *
 * The callback receives:
 *   idx   : dense index
 *   key   : canonical Slice* (arena-owned)
 *   meta  : user metadata
 *   user  : user-supplied context pointer
 */
typedef void (*InternerIterFn)(size_t idx, const Slice *key, void *meta, void *user);

void interner_foreach(const DenseArenaInterner *interner,
                      InternerIterFn cb,
                      void *user);

#ifdef __cplusplus
}
#endif
