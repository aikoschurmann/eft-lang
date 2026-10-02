// lexer_trivia.c
// Whitespace and comment skipping

#include "lexer_internal.h"

/* Consume a line comment; cur points at the first '/' of "//" */
static void lexer_skip_line_comment(Lexer *lexer) {
    /* consume '//' */
    lexer_advance(lexer);
    lexer_advance(lexer);

    while (lexer->cur < lexer->end && *lexer->cur != '\n') {
        lexer_advance(lexer);
    }
}

/* Consume a block comment; cur points at the '/' of "/<star>" */
static void lexer_skip_block_comment(Lexer *lexer) {
    lexer_advance(lexer); /* '/' */
    lexer_advance(lexer); /* '*' */

    while (lexer->cur < lexer->end) {
        if (*lexer->cur == '*' && lexer_peek_next(lexer) == '/') {
            lexer_advance(lexer); /* '*' */
            lexer_advance(lexer); /* '/' */
            break;
        }
        lexer_advance(lexer);
    }
}

/* Skip whitespace and comments (pointer-based) */
void lexer_skip_whitespace(Lexer *lexer) {
    while (lexer->cur < lexer->end) {
        char c = *lexer->cur;

        if (c == '/' && lexer_peek_next(lexer) == '/') {
            lexer_skip_line_comment(lexer);
            continue;
        }

        if (c == '/' && lexer_peek_next(lexer) == '*') {
            lexer_skip_block_comment(lexer);
            continue;
        }

        if (isspace((unsigned char)c)) {
            lexer_advance(lexer);
            continue;
        }

        break;
    }
}
