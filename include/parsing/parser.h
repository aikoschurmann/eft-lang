#pragma once

#include <stddef.h>
#include <setjmp.h>
#include <stdbool.h>
#include "token.h"
#include "dynamic_array.h"
#include "arena.h"
#include "ast.h"

/* Parser Error Info */
typedef struct {
    const char *message;
    Token *token;
    bool has_error;
} ParseError;

/* Parser structure */
typedef struct {
    DynArray   *tokens;   /* borrowed: DynArray of Token (Tokens stored in tokens.data) */
    size_t      current;  /* next token index to consume (0..tokens.count) */
    size_t      end;      /* tokens.count (one-past-last) */
    char       *filename; /* pointer to filename string in arena */
    Arena      *arena;    /* arena that owns parser/filename/messages */
    
    jmp_buf     error_jmp; /* setjmp buffer for error unwinding */
    ParseError  error;     /* last error recorded before jump */
} Parser;

/* ----------------------- Setup ----------------------- */
Parser *parser_create(DynArray *tokens, char *filename, Arena *arena);
void parser_free(Parser *parser);

/* ----------------------- Error Handling ----------------------- */
/* Throw an error and longjmp to the top-level catch handler. */
void parser_throw(Parser *p, const char *message, Token *token);
void print_parse_error(Parser *p);

/* ----------------------- Tokens ----------------------- */
Token *current_token(Parser *p);
Token *previous_token(Parser *p);
Token *peek(Parser *p, size_t offset);
Token *parser_advance(Parser *p);
bool   parser_match(Parser *p, TokenKind expected);

/* Consume a token of expected kind. If it doesn't match, throws a parser error. */
Token *expect(Parser *p, TokenKind expected, const char *error_msg);

/* ----------------------- Allocators ----------------------- */
/* Creates a node. Throws if OOM. */
AstNode *new_node(Parser *p, AstNodeType kind);
DynArray *new_dynarray(Parser *p, size_t elem_size, int initial_capacity);

/* ----------------------- Entry Point ----------------------- */
AstNode *parse_program(Parser *p);

