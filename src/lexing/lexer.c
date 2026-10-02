// lexer.c
// Lexer implementation using DynArray for token storage and the DenseArenaInterner
//
// Optimized: pointer-based scanning (cur/end), single-peek usage in branches,
// and usage of intern_peek for keywords (no accidental insertion).
//
// This file holds the keyword table, lifecycle (create/destroy/reset) and the
// top-level token dispatch. Scanning of each token class lives in:
//   lexer_trivia.c  whitespace and comments
//   lexer_ident.c   identifiers and keywords
//   lexer_number.c  numeric literals
//   lexer_text.c    string / char / byte literals
//   lexer_punct.c   operators and delimiters

#include "lexer_internal.h"
#include "dense_arena_interner.h"
#include "dynamic_array.h"
#include "arena.h"

/* Initial token array capacity (used by dynarray as growth hint) */
#define INITIAL_TOKEN_CAPACITY 256

/* Initial bucket counts for the interner hash maps */
#define KEYWORD_MAP_CAPACITY    32
#define IDENTIFIER_MAP_CAPACITY 128
#define STRING_MAP_CAPACITY     64


/* ══════════════════════════════════════════════════════════════════════════
   Keyword table
   ══════════════════════════════════════════════════════════════════════════ */

/* Keyword table generated via X-Macros */
typedef struct {
    TokenKind   type;
    const char *word;
} KeywordEntry;

// Define the macro cleanly outside the array
#define AS_KEYWORD(name, str) { name, str },

static const KeywordEntry KEYWORDS[] = {
    EFT_KEYWORDS(AS_KEYWORD)
    { TK_COUNT, NULL } // sentinel
};

#undef AS_KEYWORD

/* Pre-intern every keyword with its TokenKind stored as metadata */
void lexer_populate_default_keywords(DenseArenaInterner *keywords) {
    if (!keywords) return;

    for (size_t i = 0; KEYWORDS[i].word; i++) {
        Slice slice = {
            .ptr = (char *)KEYWORDS[i].word,
            .len = strlen(KEYWORDS[i].word)
        };
        intern(keywords, &slice, (void *)(uintptr_t)KEYWORDS[i].type);
    }
}


/* ══════════════════════════════════════════════════════════════════════════
   Construction / destruction
   ══════════════════════════════════════════════════════════════════════════ */

static DenseArenaInterner *create_interner(Arena *arena, size_t initial_capacity) {
    return intern_table_create(hashmap_create(arena, initial_capacity, slice_hash, slice_cmp), arena);
}

Lexer *lexer_create_ex(const char *source, size_t source_len, Arena *arena,
                       DenseArenaInterner *keywords,
                       DenseArenaInterner *identifiers,
                       DenseArenaInterner *strings) {
    if (!source || !arena) return NULL;
    if (!keywords || !identifiers || !strings) return NULL;

    Lexer *lexer = arena_alloc(arena, sizeof(Lexer));
    if (!lexer) return NULL;

    lexer->source     = source;
    lexer->source_len = source_len;
    lexer->pos        = 0;
    lexer->line       = 1;
    lexer->col        = 1;
    lexer->arena      = arena;

    /* pointer-based scanning state */
    lexer->cur = source;
    lexer->end = source + source_len;

    lexer->keywords    = keywords;
    lexer->identifiers = identifiers;
    lexer->strings     = strings;

    lexer->tokens = arena_alloc(arena, sizeof(DynArray));
    if (!lexer->tokens) return NULL;

    // Initialize the DynArray for tokens using arena-backed storage
    dynarray_init_in_arena(lexer->tokens, arena, sizeof(Token), INITIAL_TOKEN_CAPACITY);

    return lexer;
}

/* Create and initialize a new Lexer */
Lexer *lexer_create(const char *source, size_t source_len, Arena *arena) {
    /* arena must be valid before any interner can be created from it */
    if (!source || !arena) return NULL;

    DenseArenaInterner *keywords    = create_interner(arena, KEYWORD_MAP_CAPACITY);
    DenseArenaInterner *identifiers = create_interner(arena, IDENTIFIER_MAP_CAPACITY);
    DenseArenaInterner *strings     = create_interner(arena, STRING_MAP_CAPACITY);

    if (!keywords || !identifiers || !strings) return NULL;

    /* Pre-intern keywords with TokenKind as metadata */
    lexer_populate_default_keywords(keywords);

    return lexer_create_ex(source, source_len, arena, keywords, identifiers, strings);
}

/* Destroy lexer */
void lexer_destroy(Lexer *lexer) {
    if (!lexer) return;

    // Interners are not destroyed here as they might be shared or arena-owned.
    // They will be cleaned up when the arena is destroyed.
    if (lexer->tokens) {
        dynarray_free(lexer->tokens);
        lexer->tokens = NULL;
    }
}

void lexer_reset(Lexer *lexer) {
    if (!lexer) return;

    lexer->pos  = 0;
    lexer->line = 1;
    lexer->col  = 1;
    lexer->cur  = lexer->source;
    lexer->end  = lexer->source + lexer->source_len;

    dynarray_clear(lexer->tokens);
    /* Interners are intentionally NOT cleared: keywords and previously
       interned identifiers remain valid across resets. */
}


/* ══════════════════════════════════════════════════════════════════════════
   Tokenization
   ══════════════════════════════════════════════════════════════════════════ */

/* Check EOF */
bool lexer_at_end(const Lexer *lexer) {
    return lexer->cur >= lexer->end;
}

static Token make_eof_token(const Lexer *lexer) {
    return (Token){
        .type  = TK_EOF,
        .slice = { (char *)lexer->cur, 0 },
        .span  = { lexer->line, lexer->col, lexer->line, lexer->col }
    };
}

/* Produce next token (pointer-based): pure dispatch, one level of abstraction */
Token lexer_next_token(Lexer *lexer) {
    lexer_skip_whitespace(lexer);
    if (lexer_at_end(lexer)) return make_eof_token(lexer);

    const char *start = lexer->cur;
    size_t      line  = lexer->line;
    size_t      col   = lexer->col;
    char        c     = lexer_advance(lexer);

    if (is_alpha(c)) return lexer_token_word(lexer, c, start, line, col);
    if (is_digit(c)) return lexer_token_number(lexer, start, line, col);
    if (c == '"')    return lexer_token_string(lexer, start, line, col);
    if (c == '\'')   return lexer_token_rune(lexer, start, line, col);

    return lexer_token_punct(lexer, c, start, line, col);
}

/* Lex all tokens into the lexer's token array */
bool lexer_lex_all(Lexer *lexer) {
    if (!lexer) return false;

    Token tok;
    do {
        tok = lexer_next_token(lexer);
        if (dynarray_push_value(lexer->tokens, &tok) != 0) return false;
    } while (tok.type != TK_EOF);

    return true;
}

/* Return pointer to token array and count */
Token *lexer_get_tokens(Lexer *lexer, size_t *count) {
    if (!lexer || !count) return NULL;

    *count = lexer->tokens->count;
    return (Token *)lexer->tokens->data;
}
