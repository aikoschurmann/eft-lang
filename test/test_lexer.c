#include "framework.h"
#include "lexing/lexer.h"
#include <string.h>

static void assert_token(Lexer *l, TokenKind expected_kind, const char *expected_text, uint32_t expected_line, uint32_t expected_col) {
    Token t = lexer_next_token(l);
    if (t.type != expected_kind) {
        fprintf(stderr, "\nDEBUG: Expected '%s' (kind %d) but got '%.*s' (kind %d) at %d:%d\n", expected_text, expected_kind, (int)t.slice.len, t.slice.ptr, t.type, (int)t.span.start_line, (int)t.span.start_col);
        fprintf(stderr, "\nExpected kind %d but got %d for '%.*s'\n", expected_kind, t.type, (int)t.slice.len, t.slice.ptr);
        TEST_ASSERT(t.type == expected_kind);
    }
    
    if (expected_text) {
        size_t expected_len = strlen(expected_text);
        if (t.slice.len != expected_len || strncmp((const char*)t.slice.ptr, expected_text, expected_len) != 0) {
            fprintf(stderr, "\nExpected text '%s' but got '%.*s'\n", expected_text, (int)t.slice.len, t.slice.ptr);
            TEST_ASSERT(t.slice.len == expected_len);
            TEST_ASSERT(strncmp((const char*)t.slice.ptr, expected_text, expected_len) == 0);
        }
    }
    
    if (expected_line > 0) TEST_ASSERT(t.span.start_line == expected_line);
    if (expected_col > 0) TEST_ASSERT(t.span.start_col == expected_col);
}

static void test_lexer_keywords_and_idents(void) {
    const char *src = "fn main() { if true { mut x := 42; } }";
    Arena *arena = arena_create(1024);
    
    Lexer *l = lexer_create(src, strlen(src), arena);
    
    assert_token(l, TK_FN, "fn", 1, 1);
    assert_token(l, TK_IDENT, "main", 1, 4);
    assert_token(l, TK_LPAREN, "(", 1, 8);
    assert_token(l, TK_RPAREN, ")", 1, 9);
    assert_token(l, TK_LBRACE, "{", 1, 11);
    assert_token(l, TK_IF, "if", 1, 13);
    assert_token(l, TK_TRUE, "true", 1, 16);
    assert_token(l, TK_LBRACE, "{", 1, 21);
    assert_token(l, TK_MUT, "mut", 1, 23);
    assert_token(l, TK_IDENT, "x", 1, 27);
    assert_token(l, TK_COLON_EQ, ":=", 1, 29);
    assert_token(l, TK_INT_LIT, "42", 1, 32);
    assert_token(l, TK_SEMICOLON, ";", 1, 34);
    assert_token(l, TK_RBRACE, "}", 1, 36);
    assert_token(l, TK_RBRACE, "}", 1, 38);
    assert_token(l, TK_EOF, NULL, 0, 0);

    arena_destroy(arena);
}

static void test_lexer_numbers(void) {
    const char *src = "42 0x1A 0b1010 3.14 1e5 2.5e-3 42_000 ";
    Arena *arena = arena_create(1024);
    
    Lexer *l = lexer_create(src, strlen(src), arena);
    
    assert_token(l, TK_INT_LIT, "42", 1, 1);
    assert_token(l, TK_INT_LIT, "0x1A", 1, 4);
    assert_token(l, TK_INT_LIT, "0b1010", 1, 9);
    assert_token(l, TK_FLOAT_LIT, "3.14", 1, 16);
    assert_token(l, TK_FLOAT_LIT, "1e5", 1, 21);
    assert_token(l, TK_FLOAT_LIT, "2.5e-3", 1, 25);
    assert_token(l, TK_INT_LIT, "42_000", 1, 32);
        assert_token(l, TK_EOF, NULL, 0, 0);

    arena_destroy(arena);
}

static void test_lexer_strings_and_runes(void) {
    const char *src = "\"hello\" b\"world\" 'a' b'c' \"line\\n1\" '\\n'";
    Arena *arena = arena_create(1024);
    
    Lexer *l = lexer_create(src, strlen(src), arena);
    
    assert_token(l, TK_STR_LIT, "\"hello\"", 1, 1);
    assert_token(l, TK_BYTE_STR_LIT, "b\"world\"", 1, 9);
    assert_token(l, TK_RUNE_LIT, "'a'", 1, 18);
    assert_token(l, TK_BYTE_LIT, "b'c'", 1, 22);
    assert_token(l, TK_STR_LIT, "\"line\\n1\"", 1, 27);
    assert_token(l, TK_RUNE_LIT, "'\\n'", 1, 37);
    assert_token(l, TK_EOF, NULL, 0, 0);

    arena_destroy(arena);
}

static void test_lexer_punctuators(void) {
    const char *src = "+ - * / % += -= *= /= %= == != < <= > >= && || & | ^ ~ << >> ! ? .. . -> =>";
    Arena *arena = arena_create(1024);
    
    Lexer *l = lexer_create(src, strlen(src), arena);
    
    assert_token(l, TK_PLUS, "+", 1, 1);
    assert_token(l, TK_MINUS, "-", 1, 3);
    assert_token(l, TK_STAR, "*", 1, 5);
    assert_token(l, TK_SLASH, "/", 1, 7);
    assert_token(l, TK_PERCENT, "%", 1, 9);
    assert_token(l, TK_PLUS_EQ, "+=", 1, 11);
    assert_token(l, TK_MINUS_EQ, "-=", 1, 14);
    assert_token(l, TK_STAR_EQ, "*=", 1, 17);
    assert_token(l, TK_SLASH_EQ, "/=", 1, 20);
    assert_token(l, TK_PERCENT_EQ, "%=", 1, 23);
    assert_token(l, TK_EQ_EQ, "==", 1, 26);
    assert_token(l, TK_BANG_EQ, "!=", 1, 29);
    assert_token(l, TK_LESS, "<", 1, 32);
    assert_token(l, TK_LESS_EQ, "<=", 1, 34);
    assert_token(l, TK_GREATER, ">", 1, 37);
    assert_token(l, TK_GREATER_EQ, ">=", 1, 39);
    assert_token(l, TK_AND_AND, "&&", 1, 42);
    assert_token(l, TK_OR_OR, "||", 1, 45);
    assert_token(l, TK_AMPERSAND, "&", 1, 48);
    assert_token(l, TK_PIPE, "|", 1, 50);
    assert_token(l, TK_CARET, "^", 1, 52);
    assert_token(l, TK_TILDE, "~", 1, 54);
    assert_token(l, TK_SHL, "<<", 1, 56);
    assert_token(l, TK_SHR, ">>", 1, 59);
    assert_token(l, TK_BANG, "!", 1, 62);
    assert_token(l, TK_QUESTION, "?", 1, 64);
    assert_token(l, TK_DOT_DOT, "..", 1, 66);
    assert_token(l, TK_DOT, ".", 1, 69);
    assert_token(l, TK_ARROW, "->", 1, 71);
    assert_token(l, TK_FAT_ARROW, "=>", 1, 74);
    
    assert_token(l, TK_EOF, NULL, 0, 0);

    arena_destroy(arena);
}

static void test_lexer_comments(void) {
    const char *src = "x // single line\ny /* block */ z /* another block */ w";
    Arena *arena = arena_create(1024);
    
    Lexer *l = lexer_create(src, strlen(src), arena);
    
    assert_token(l, TK_IDENT, "x", 1, 1);
    // comments are skipped by lexer_next_token
    assert_token(l, TK_IDENT, "y", 2, 1);
    assert_token(l, TK_IDENT, "z", 2, 15);
    assert_token(l, TK_IDENT, "w", 2, 0); 
    assert_token(l, TK_EOF, NULL, 0, 0); 

    arena_destroy(arena);
}

static void test_lexer_errors(void) {
    const char *src = "\"unterminated \n ' 'a \n 0xZZ \n";
    Arena *arena = arena_create(1024);
    Lexer *l = lexer_create(src, strlen(src), arena);
    
    assert_token(l, TK_UNKNOWN, NULL, 1, 1);
    assert_token(l, TK_EOF, NULL, 0, 0);

    arena_destroy(arena);
}



void suite_lexer(void);
void suite_lexer(void) {
    RUN_TEST(test_lexer_keywords_and_idents);
    RUN_TEST(test_lexer_numbers);
    RUN_TEST(test_lexer_strings_and_runes);
    RUN_TEST(test_lexer_punctuators);
    RUN_TEST(test_lexer_comments);
    RUN_TEST(test_lexer_errors); 
}
