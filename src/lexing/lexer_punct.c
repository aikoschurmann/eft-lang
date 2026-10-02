// lexer_punct.c
// Operators and delimiters

#include "lexer_internal.h"

/* Operators that can only be a single character */
static TokenKind single_char_kind(char c) {
    switch (c) {
        case '^': return TK_CARET;
        case '~': return TK_TILDE;
        case '?': return TK_QUESTION;
        case '(': return TK_LPAREN;
        case ')': return TK_RPAREN;
        case '{': return TK_LBRACE;
        case '}': return TK_RBRACE;
        case '[': return TK_LBRACKET;
        case ']': return TK_RBRACKET;
        case ',': return TK_COMMA;
        case ';': return TK_SEMICOLON;
        default:  return TK_UNKNOWN;
    }
}

/* Operators that may extend into a two-character form. `c` is already consumed. */
static TokenKind punct_kind(Lexer *l, char c) {
    switch (c) {
        case '+':
            return lexer_match(l, '=') ? TK_PLUS_EQ : TK_PLUS;

        case '-':
            if (lexer_match(l, '=')) return TK_MINUS_EQ;
            if (lexer_match(l, '>')) return TK_ARROW;
            return TK_MINUS;

        case '*':
            return lexer_match(l, '=') ? TK_STAR_EQ : TK_STAR;

        case '/':
            return lexer_match(l, '=') ? TK_SLASH_EQ : TK_SLASH;

        case '%':
            return lexer_match(l, '=') ? TK_PERCENT_EQ : TK_PERCENT;

        case '=':
            return lexer_match(l, '=') ? TK_EQ_EQ : TK_EQ;

        case '!':
            return lexer_match(l, '=') ? TK_BANG_EQ : TK_BANG;

        case '<':
            if (lexer_match(l, '<')) return TK_SHL;
            if (lexer_match(l, '=')) return TK_LESS_EQ;
            return TK_LESS;

        case '>':
            if (lexer_match(l, '>')) return TK_SHR;
            if (lexer_match(l, '=')) return TK_GREATER_EQ;
            return TK_GREATER;

        case '&':
            return lexer_match(l, '&') ? TK_AND_AND : TK_AMPERSAND;

        case '|':
            return lexer_match(l, '|') ? TK_OR_OR : TK_PIPE;

        case ':':
            return lexer_match(l, '=') ? TK_COLON_EQ : TK_COLON;

        case '.':
            return lexer_match(l, '.') ? TK_DOT_DOT : TK_DOT;

        default:
            return single_char_kind(c);
    }
}

Token lexer_token_punct(Lexer *l, char c, const char *start, size_t line, size_t col) {
    TokenKind kind = punct_kind(l, c);
    return make_token(l, kind, start, line, col, NULL);
}
