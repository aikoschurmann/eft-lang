// lexer_internal.h
// Private header shared by the lexer_*.c translation units.
// Not part of the public API: include "lexer.h" from outside the lexer.

#ifndef LEXER_INTERNAL_H
#define LEXER_INTERNAL_H

#include "lexer.h"
#include "token.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>


/* ══════════════════════════════════════════════════════════════════════════
   Low-level cursor primitives
   ══════════════════════════════════════════════════════════════════════════ */

static inline char lexer_peek(const Lexer *lexer) {
    return lexer->cur < lexer->end ? *lexer->cur : '\0';
}

/* Peek one character past the current one ('\0' if out of range) */
static inline char lexer_peek_next(const Lexer *lexer) {
    return (lexer->cur + 1) < lexer->end ? *(lexer->cur + 1) : '\0';
}

/* Advance returns the char consumed (like before) and updates pos/cur/line/col */
static inline char lexer_advance(Lexer *lexer) {
    if (lexer->cur >= lexer->end) return '\0';

    char c = *lexer->cur++;
    lexer->pos++;

    if (c == '\n') {
        lexer->line++;
        lexer->col = 1;
    } else {
        lexer->col++;
    }
    return c;
}

/* Consume the next character only if it equals `expected` */
static inline bool lexer_match(Lexer *lexer, char expected) {
    if (lexer_peek(lexer) != expected) return false;
    lexer_advance(lexer);
    return true;
}

/* Jump cur to target in O(newlines).
   For identifiers and numbers (no embedded newlines) this is O(1). */
static inline void sync_to(Lexer *l, const char *target) {
    const char *p = l->cur;

    while (p < target) {
        const char *nl = (const char *)memchr(p, '\n', (size_t)(target - p));

        if (!nl) {
            l->col += (size_t)(target - p);
            break;
        }

        l->line++;
        l->col = 1;
        p = nl + 1;
    }

    l->pos += (size_t)(target - l->cur);
    l->cur  = target;
}


/* ══════════════════════════════════════════════════════════════════════════
   Character classification
   ══════════════════════════════════════════════════════════════════════════ */

/* Portable classification: treat '_' as alpha, use ctype for letters/digits */
static inline bool is_alpha(char c) {
    return c == '_' || isalpha((unsigned char)c);
}

static inline bool is_digit(char c) {
    return isdigit((unsigned char)c);
}

static inline bool is_hex_digit(char c) {
    return isxdigit((unsigned char)c);
}

static inline bool is_bin_digit(char c) {
    return c == '0' || c == '1';
}

static inline bool is_digit_separator(char c) {
    return c == '_';
}


/* ══════════════════════════════════════════════════════════════════════════
   Slices and token construction
   ══════════════════════════════════════════════════════════════════════════ */

/* Create slice from current lexer position using pointer arithmetic */
static inline Slice lexer_make_slice_from_ptrs(const char *start_ptr, const char *end_ptr) {
    return (Slice) {
        .ptr = (char *)start_ptr,
        .len = (size_t)(end_ptr - start_ptr)
    };
}

static inline Token make_token(Lexer *l, TokenKind type,
                               const char *start, size_t line, size_t col,
                               void *rec) {
    Slice slice = lexer_make_slice_from_ptrs(start, l->cur);
    Span  span  = { line, col, l->line, l->col };

    return (Token){
        .type   = type,
        .slice  = slice,
        .span   = span,
        .record = rec
    };
}


/* ══════════════════════════════════════════════════════════════════════════
   Internal entry points (implemented across the lexer_*.c files)
   ══════════════════════════════════════════════════════════════════════════ */

/* lexer_trivia.c: skip whitespace and comments */
void lexer_skip_whitespace(Lexer *lexer);

/* lexer_ident.c: identifiers, keywords (and b"..." / b'...' dispatch) */
Token lexer_token_word(Lexer *l, char c, const char *start, size_t line, size_t col);

/* lexer_number.c: integer and float literals */
Token lexer_token_number(Lexer *l, const char *start, size_t line, size_t col);

/* lexer_text.c: string, char and byte literals */
Token lexer_token_string(Lexer *l, const char *start, size_t line, size_t col);
Token lexer_token_rune(Lexer *l, const char *start, size_t line, size_t col);
Token lexer_token_byte_literal(Lexer *l, const char *start, size_t line, size_t col);

/* lexer_punct.c: operators and delimiters */
Token lexer_token_punct(Lexer *l, char c, const char *start, size_t line, size_t col);

#endif /* LEXER_INTERNAL_H */
