// lexer_ident.c
// Identifiers and keywords

#include "lexer_internal.h"
#include "dense_arena_interner.h"

/* Identifier or keyword (uses intern_peek to avoid insertion on keyword check) */
static InternResult *lexer_lex_identifier(Lexer *lexer, const char *start_ptr, const char *end_ptr, TokenKind *out_type) {
    Slice slice = lexer_make_slice_from_ptrs(start_ptr, end_ptr);

    /* Lookup keyword WITHOUT inserting. */
    InternResult *kwres = intern_peek(lexer->keywords, &slice);
    if (kwres) {
        *out_type = (TokenKind)(uintptr_t)kwres->entry->meta;
        return kwres;
    }

    /* Not a keyword: insert/return identifier record */
    InternResult *idres = intern(lexer->identifiers, &slice, NULL);
    if (!idres) {
        *out_type = TK_UNKNOWN;
        return NULL;
    }

    *out_type = TK_IDENT;
    return idres;
}

/* True if the already-consumed 'b' is directly followed by a quote */
static bool starts_byte_literal(const Lexer *l, char c) {
    return c == 'b'
        && l->cur < l->end
        && (*l->cur == '"' || *l->cur == '\'');
}

/* Word token: byte literal prefix, keyword, or identifier. `c` is already consumed. */
Token lexer_token_word(Lexer *l, char c, const char *start, size_t line, size_t col) {
    if (starts_byte_literal(l, c)) {
        return lexer_token_byte_literal(l, start, line, col);
    }

    const char *p = l->cur;
    while (p < l->end && (is_alpha(*p) || is_digit(*p))) p++;
    sync_to(l, p);

    TokenKind     kind = TK_UNKNOWN;
    InternResult *rec  = lexer_lex_identifier(l, start, l->cur, &kind);
    return make_token(l, kind, start, line, col, rec);
}
