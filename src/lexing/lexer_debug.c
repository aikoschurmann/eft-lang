// lexer_debug.c
// Token names and debug printing

#include "lexer_internal.h"

#include <stdio.h>


/* ══════════════════════════════════════════════════════════════════════════
   Token names
   ══════════════════════════════════════════════════════════════════════════ */

/* User-facing name: returns the literal string ("fn", "+", "identifier", …).
   Use this in error messages: "expected '+', got 'fn'". */
const char *token_type_to_string(TokenKind kind) {
    switch (kind) {
#define X(name, str) case name: return str;
        EFT_ALL_TOKENS(X)
#undef X
        default: return "unknown";
    }
}

/* Debug name: returns the enum identifier ("TK_FN", "TK_PLUS", …).
   Use this in AST dumps and compiler diagnostics. */
const char *token_kind_name(TokenKind kind) {
    switch (kind) {
#define X(name, str) case name: return #name;
        EFT_ALL_TOKENS(X)
#undef X
        default: return "TK_UNKNOWN";
    }
}


/* ══════════════════════════════════════════════════════════════════════════
   print_token
   ══════════════════════════════════════════════════════════════════════════ */

static void print_token_lexeme(const Token *tok) {
    if (tok->slice.ptr && tok->slice.len > 0) {
        printf("'%.*s'", (int)tok->slice.len, tok->slice.ptr);
    } else {
        printf("(no-lexeme)");
    }
}

static void print_token_annotation(const Token *tok) {
    if (tok->type == TK_RUNE_LIT && tok->record) {
        printf("  (char: U+%04X)", (uint32_t)(uintptr_t)tok->record);
    }
}

void print_token(const Token *tok) {
    printf("│ %3zu:%-3zu │ %-13s │ ",
           tok->span.start_line,
           tok->span.start_col,
           token_type_to_string(tok->type));

    print_token_lexeme(tok);
    print_token_annotation(tok);
    printf("\n");
}
