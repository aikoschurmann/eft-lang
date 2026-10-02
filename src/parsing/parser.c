#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

Parser *parser_create(DynArray *tokens, char *filename, Arena *arena) {
    Parser *p = arena_alloc(arena, sizeof(Parser));
    if (!p) return NULL;
    p->tokens = tokens;
    p->current = 0;
    p->end = tokens->count;
    p->filename = filename;
    p->arena = arena;
    p->error.has_error = false;
    return p;
}

void parser_free(Parser *parser) {
    // Arena cleans up everything
}

Token *current_token(Parser *p) {
    if (p->current >= p->end) return NULL;
    return &((Token *)p->tokens->data)[p->current];
}

Token *peek(Parser *p, size_t offset) {
    if (p->current + offset >= p->end) return NULL;
    return &((Token *)p->tokens->data)[p->current + offset];
}

Token *parser_advance(Parser *p) {
    if (p->current >= p->end) return NULL;
    Token *t = current_token(p);
    p->current++;
    return t;
}

bool parser_match(Parser *p, TokenKind expected) {
    Token *t = current_token(p);
    if (!t) return false;
    if (t->type == expected) {
        parser_advance(p);
        return true;
    }
    return false;
}

void parser_throw(Parser *p, const char *message, Token *token) {
    p->error.message = message;
    p->error.token = token;
    p->error.has_error = true;
    longjmp(p->error_jmp, 1);
}

void print_parse_error(Parser *p) {
    if (!p->error.has_error) return;
    fprintf(stderr, "Parse error: %s\n", p->error.message);
}

Token *expect(Parser *p, TokenKind expected, const char *error_msg) {
    Token *t = current_token(p);
    if (!t || t->type != expected) {
        parser_throw(p, error_msg, t);
    }
    return parser_advance(p);
}

AstNode *new_node(Parser *p, AstNodeType kind) {
    AstNode *node = ast_create_node(kind, p->arena);
    if (!node) {
        parser_throw(p, "Out of memory allocating AST node", current_token(p));
    }
    return node;
}

DynArray *new_dynarray(Parser *p, size_t elem_size, int initial_capacity) {
    DynArray *arr = arena_alloc(p->arena, sizeof(DynArray));
    if (!arr) parser_throw(p, "Out of memory allocating dynamic array struct", current_token(p));
    if (!dynarray_init_in_arena(arr, p->arena, elem_size, initial_capacity)) {
        parser_throw(p, "Out of memory allocating dynamic array buffer", current_token(p));
    }
    return arr;
}

AstNode *parse_program(Parser *p) {
    // Set the landing zone for any parser_throw (longjmp) calls
    if (setjmp(p->error_jmp) == 0) {
        // Happy path: parse the entire program
        AstNode *program = new_node(p, AST_PROGRAM);
        program->data.program.program_items = new_dynarray(p, sizeof(AstNode *), 16);
        
        while (current_token(p) && current_token(p)->type != TK_EOF) {
            // TODO: parse_item(p);
            // AstNode *item = parse_item(p);
            // dynarray_push_ptr(program->data.program.program_items, item);
            parser_advance(p); // temporary stub
        }
        
        return program;
    } else {
        // Error path: longjmp brought us here!
        print_parse_error(p);
        return NULL;
    }
}
