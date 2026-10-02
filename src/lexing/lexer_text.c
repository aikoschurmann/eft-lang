// lexer_text.c
// String, char and byte literals, including escape handling

#include "lexer_internal.h"
#include "dense_arena_interner.h"
#include "arena.h"


/* ══════════════════════════════════════════════════════════════════════════
   Escapes
   ══════════════════════════════════════════════════════════════════════════ */

/* Translate the character following a backslash into the character it denotes.
   Unknown escapes map to themselves (so \" and \' work as expected). */
static char decode_escape(char c) {
    switch (c) {
        case 'n':  return '\n';
        case 't':  return '\t';
        case 'r':  return '\r';
        case '0':  return '\0';
        default:   return c;
    }
}

/* Unescape string content (raw slice excludes surrounding quotes) into arena and return slice
   pointing into arena. The arena allocation size is conservatively raw.len (since unescaped <= raw.len)
   plus one for NUL. */
static Slice unescape_string_into_arena(const Slice raw, Arena *arena) {
    if (raw.len == 0) return (Slice){ .ptr = NULL, .len = 0 };

    char *out = arena_alloc(arena, raw.len + 1);
    if (!out) return (Slice){ .ptr = NULL, .len = 0 };

    char       *w   = out;
    const char *r   = raw.ptr;
    const char *end = raw.ptr + raw.len;

    while (r < end) {
        if (*r != '\\') {
            *w++ = *r++;
            continue;
        }

        r++;
        if (r >= end) break;

        *w++ = decode_escape(*r);
        r++;
    }

    size_t n = (size_t)(w - out);
    out[n] = '\0';
    return (Slice){ .ptr = out, .len = n };
}


/* ══════════════════════════════════════════════════════════════════════════
   Raw scanning
   ══════════════════════════════════════════════════════════════════════════ */

/* String literal (handles escapes). Returns end pointer after closing quote. */
static TokenKind lexer_lex_string(const char **curptr, const char *endptr) {
    /* curptr points at the opening quote ('"') */
    const char *p = *curptr;
    p++; /* skip opening quote */

    while (p < endptr && *p != '"') {
        if (*p == '\\') {
            p++;
            if (p < endptr) p++; /* skip escaped char */
        } else {
            p++;
        }
    }

    if (p >= endptr) {
        *curptr = p;
        return TK_UNKNOWN; /* unterminated */
    }

    p++; /* skip closing quote */
    *curptr = p;
    return TK_STR_LIT;
}

/* Char literal */
static TokenKind lexer_lex_char(const char **curptr, const char *endptr, uint32_t *out_codepoint) {
    const char *p = *curptr;
    p++; /* skip opening '\'' */

    if (p >= endptr) {
        *curptr = p;
        return TK_UNKNOWN;
    }

    uint32_t cp = 0;

    if (*p == '\\') {
        p++;
        if (p >= endptr) {
            *curptr = p;
            return TK_UNKNOWN;
        }

        /* simple fallback: unknown escapes take the escaped char literally */
        cp = (unsigned char)decode_escape(*p);
        p++;
    } else {
        cp = (unsigned char)*p;
        p++;
    }

    if (p >= endptr || *p != '\'') {
        *curptr = p;
        return TK_UNKNOWN;
    }

    p++; /* skip closing '\'' */
    *curptr = p;
    *out_codepoint = cp;
    return TK_RUNE_LIT;
}


/* ══════════════════════════════════════════════════════════════════════════
   Token producers
   ══════════════════════════════════════════════════════════════════════════ */

Token lexer_token_string(Lexer *l, const char *start, size_t line, size_t col) {
    const char *p = start;
    TokenKind kind = lexer_lex_string(&p, l->end);
    sync_to(l, p);

    void *rec = NULL;
    if (kind == TK_STR_LIT) {
        Slice raw     = lexer_make_slice_from_ptrs(start + 1, l->cur - 1);
        Slice content = unescape_string_into_arena(raw, l->arena);
        if (content.ptr) rec = intern(l->strings, &content, NULL);
    }

    return make_token(l, kind, start, line, col, rec);
}

Token lexer_token_rune(Lexer *l, const char *start, size_t line, size_t col) {
    const char *p = start;
    uint32_t cp = 0;
    TokenKind kind = lexer_lex_char(&p, l->end, &cp);
    sync_to(l, p);

    void *rec = (kind == TK_RUNE_LIT) ? (void *)(uintptr_t)cp : NULL;
    return make_token(l, kind, start, line, col, rec);
}

/* Byte literals: b"..." and b'...'. The 'b' is already consumed. */
Token lexer_token_byte_literal(Lexer *l, const char *start, size_t line, size_t col) {
    char quote = *l->cur;
    lexer_advance(l);

    const char *p = l->cur - 1;

    if (quote == '"') {
        lexer_lex_string(&p, l->end);
        sync_to(l, p);
        return make_token(l, TK_BYTE_STR_LIT, start, line, col, NULL);
    }

    uint32_t cp = 0;
    lexer_lex_char(&p, l->end, &cp);
    sync_to(l, p);
    return make_token(l, TK_BYTE_LIT, start, line, col, (void *)(uintptr_t)cp);
}
