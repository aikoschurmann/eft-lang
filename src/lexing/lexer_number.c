// lexer_number.c
// Numeric literals: decimal, 0x hex, 0b binary, floats with optional exponent

#include "lexer_internal.h"

static const char *skip_digits(const char *p, const char *end) {
    while (p < end && (is_digit(*p) || is_digit_separator(*p))) p++;
    return p;
}

static const char *skip_hex_digits(const char *p, const char *end) {
    while (p < end && (is_hex_digit(*p) || is_digit_separator(*p))) p++;
    return p;
}

static const char *skip_bin_digits(const char *p, const char *end) {
    while (p < end && (is_bin_digit(*p) || is_digit_separator(*p))) p++;
    return p;
}

/* Optional exponent: [eE] [+-]? digits
   Only consumed if at least one digit follows; otherwise returns p unchanged
   so that "1e+" or "1.5e" leave the 'e' for the next token. */
static const char *skip_exponent(const char *p, const char *end) {
    if (p >= end || (*p != 'e' && *p != 'E')) return p;

    const char *q = p + 1;
    if (q < end && (*q == '+' || *q == '-')) q++;

    if (q >= end || !is_digit(*q)) return p;

    return skip_digits(q, end);
}

/* 0x... / 0b... integer literals. Returns false if no radix prefix is present. */
static bool lexer_lex_prefixed_int(const char *start_ptr, const char *end, const char **out_end_ptr) {
    if (*start_ptr != '0' || (start_ptr + 1) >= end) return false;

    char prefix = start_ptr[1];
    const char *body = start_ptr + 2;

    if (prefix == 'x' || prefix == 'X') {
        *out_end_ptr = skip_hex_digits(body, end);
        return true;
    }

    if (prefix == 'b' || prefix == 'B') {
        *out_end_ptr = skip_bin_digits(body, end);
        return true;
    }

    return false;
}

/* Number literal (pointer-based scan). Handles basic integer/float forms. */
static TokenKind lexer_lex_number(Lexer *lexer, const char *start_ptr, const char **out_end_ptr) {
    const char *end = lexer->end;

    if (lexer_lex_prefixed_int(start_ptr, end, out_end_ptr)) {
        return TK_INT_LIT;
    }

    /* Decimal digits (including those starting with 0) */
    const char *p = skip_digits(start_ptr, end);

    /* Exponent on a bare integer: 1e5, 2E-3, etc. */
    const char *after_exp = skip_exponent(p, end);
    if (after_exp > p) {
        *out_end_ptr = after_exp;
        return TK_FLOAT_LIT;
    }

    /* No fractional part */
    if (p >= end || *p != '.') {
        *out_end_ptr = p;
        return TK_INT_LIT;
    }

    /* 10. — stop if '.' is not followed by a digit (could be member access) */
    const char *after_dot = p + 1;
    if (after_dot >= end || !is_digit(*after_dot)) {
        *out_end_ptr = p;
        return TK_INT_LIT;
    }

    /* Fractional part + optional exponent */
    p = skip_digits(after_dot, end);
    p = skip_exponent(p, end);

    *out_end_ptr = p;
    return TK_FLOAT_LIT;
}

Token lexer_token_number(Lexer *l, const char *start, size_t line, size_t col) {
    const char *end = NULL;
    TokenKind kind = lexer_lex_number(l, start, &end);

    sync_to(l, end);
    return make_token(l, kind, start, line, col, NULL);
}
