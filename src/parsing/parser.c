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

Token *previous_token(Parser *p) {
    if (p->current == 0) return NULL;
    return &((Token *)p->tokens->data)[p->current - 1];
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
    Token *t = p->error.token;
    if (t) {
        fprintf(stderr, "%s:%zu:%zu: Parse error: %s\n",
            p->filename, t->span.start_line, t->span.start_col, p->error.message);
    } else {
        fprintf(stderr, "Parse error: %s\n", p->error.message);
    }
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

/* Allocates a node and sets its span from start_tok to the last consumed token */
AstNode *new_node_spanned(Parser *p, AstNodeType kind, Token *start_tok) {
    AstNode *node = new_node(p, kind);
    set_span(node, start_tok, previous_token(p));
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

// ---------------------------------------------------------
// Recursive Descent Parsing Functions
// ---------------------------------------------------------

/* Forward declarations */
static AstNode *parse_item(Parser *p);
static AstNode *parse_import_c(Parser *p);

AstNode *parse_program(Parser *p) {
    // Set the landing zone for any parser_throw (longjmp) calls
    if (setjmp(p->error_jmp) == 0) {
        // Happy path: parse the entire program
        Token *start_tok = current_token(p);
        AstNode *program = new_node(p, AST_PROGRAM);
        program->data.program.program_items = new_dynarray(p, sizeof(AstNode *), 16);

        while (current_token(p) && current_token(p)->type != TK_EOF) {
            AstNode *item = parse_item(p);
            dynarray_push_ptr(program->data.program.program_items, item);
        }

        // Program node span covers the entire file
        set_span(program, start_tok, previous_token(p));

        return program;
    } else {
        // Error path: longjmp brought us here!
        print_parse_error(p);
        return NULL;
    }
}

/* <ImportC> ::= IMPORT_C <String> SEMICOLON */
static AstNode *parse_import_c(Parser *p) {
    Token *start_tok = expect(p, TK_IMPORT_C, "Expected 'import_c' keyword");
    Token *str_tok   = expect(p, TK_STR_LIT, "Expected string literal for C import path");
    expect(p, TK_SEMICOLON, "Expected ';' after C import path");

    AstNode *node = new_node_spanned(p, AST_IMPORT_C, start_tok);
    // Use the slice directly from the token since InternResult is not required for ImportC
    node->data.import_c_decl.name = str_tok->slice;

    return node;
}

/* Returns the InternResult the lexer attached to an identifier token. */
static InternResult *ident_record(Parser *p, Token *tok) {
    if (!tok->record)
        parser_throw(p, "Internal error: identifier token has no intern record", tok);
    return tok->record;
}

/* Keywords are valid as path segments (e.g. import test.test.*) */
static bool is_ident_like(Token *t) {
    if (!t) return false;
    if (t->type == TK_IDENT) return true;
    // Any keyword token can appear as a module/path segment
#define X(name, str) if (t->type == name) return true;
    EFT_KEYWORDS(X)
#undef X
    return false;
}

static Token *expect_ident_like(Parser *p, const char *error_msg) {
    Token *t = current_token(p);
    if (!is_ident_like(t)) parser_throw(p, error_msg, t);
    return parser_advance(p);
}

// <Path> ::= <Ident> { DOT <Ident> }
static AstNode *parse_path(Parser *p) {
    Token *start_tok = expect_ident_like(p, "Expected an identifier in path");

    DynArray *segs = new_dynarray(p, sizeof(InternResult *), 4);
    dynarray_push_ptr(segs, ident_record(p, start_tok));

    // Only consume '.' if an identifier follows. Otherwise it belongs to
    // the import suffix: `import std.io.*;` or `import std.io.{a, b};`
    while (current_token(p) && current_token(p)->type == TK_DOT) {
        Token *next = peek(p, 1);
        if (!next || !is_ident_like(next)) break;
        parser_advance(p);                       // '.'
        Token *id = parser_advance(p);           // ident or keyword
        dynarray_push_ptr(segs, ident_record(p, id));
    }

    AstNode *node = new_node_spanned(p, AST_PATH, start_tok);
    node->data.path.segments = segs;
    
    return node;
}

// <ImportItem> ::= <Ident> [ AS <Ident> ]
static AstNode *parse_import_item(Parser *p) {
    Token *name_tok = expect(p, TK_IDENT, "Expected an identifier in import list");

    InternResult *alias = NULL;
    if (parser_match(p, TK_AS)) {
        Token *alias_tok = expect(p, TK_IDENT, "Expected an identifier after 'as'");
        alias = ident_record(p, alias_tok);
    }

    AstNode *node = new_node_spanned(p, AST_IMPORT_ITEM, name_tok);
    node->data.import_item.name = ident_record(p, name_tok);
    node->data.import_item.alias = alias;

    return node;
}

/* Helper for parsing: { A, B as C } */
static DynArray *parse_import_list(Parser *p) {
    expect(p, TK_LBRACE, "Expected '*' or '{' after '.' in import");
    
    DynArray *items = new_dynarray(p, sizeof(AstNode *), 4);
    do {
        dynarray_push_ptr(items, parse_import_item(p));
    } while (parser_match(p, TK_COMMA) && current_token(p) && current_token(p)->type != TK_RBRACE);
    
    expect(p, TK_RBRACE, "Expected '}' to close import list");
    return items;
}

// <Import> ::= IMPORT <Path> [ AS <Ident> | DOT ( STAR | L_BRACE <ImportItem> { COMMA <ImportItem> } [ COMMA ] R_BRACE ) ] SEMICOLON
static AstNode *parse_import(Parser *p) {
    Token *start_tok = expect(p, TK_IMPORT, "Expected 'import' keyword");
    AstNode *path = parse_path(p);

    InternResult *alias = NULL;
    DynArray *items = NULL;
    bool is_glob = false;

    if (parser_match(p, TK_AS)) {
        alias = ident_record(p, expect(p, TK_IDENT, "Expected identifier after 'as'"));
    } 
    else if (parser_match(p, TK_DOT)) {
        if (parser_match(p, TK_STAR)) {
            is_glob = true;
        } else {
            items = parse_import_list(p); // expect(LBRACE) handles the error checking inside here
        }
    }

    expect(p, TK_SEMICOLON, "Expected ';' after import");

    AstNode *node = new_node_spanned(p, AST_IMPORT, start_tok);
    node->data.import_decl.path = path;
    node->data.import_decl.alias = alias;
    node->data.import_decl.is_glob = is_glob;
    node->data.import_decl.items = items;

    return node;
}

/* <Item> ::= [ PUB ] ( <Import> | <ExternBlock> | <ImportC> | <Fn> | <Struct> | <Enum> | <Concept>
                      | <Impl> | <ConstDecl> | <TypeAlias> | <Test> ) */
static AstNode *parse_item(Parser *p) {
    Token *start_tok = current_token(p);
    bool is_pub = parser_match(p, TK_PUB);

    AstNode *item = NULL;
    Token *t = current_token(p);
    if (!t) parser_throw(p, "Unexpected EOF while parsing item", previous_token(p));

    switch (t->type) {
        // <Import>      ::= IMPORT <Path> ...
        case TK_IMPORT:
            item = parse_import(p);
            break;
        // <ExternBlock> ::= EXTERN <String> L_BRACE ...
        case TK_EXTERN:
            parser_throw(p, "unimplemented!", t);
            break;
        // <ImportC>    ::= IMPORT_C <String> SEMICOLON
        case TK_IMPORT_C:
            item = parse_import_c(p);
            break;
        // <Fn>          ::= [ COMPTIME ] <FnSig> ...
        // <FnSig>       ::= FN <Ident> ...
        case TK_COMPTIME:
        case TK_FN:
        // <Struct>      ::= [ PACKED ] STRUCT <Ident> ...
        case TK_PACKED:
        case TK_STRUCT:
        // <Enum>        ::= ENUM <Ident> ...
        case TK_ENUM:
        // <Concept>     ::= CONCEPT <Ident> ...
        case TK_CONCEPT:
        // <Impl>        ::= IMPL <Type> ...
        case TK_IMPL:
        // <ConstDecl>   ::= CONST <Ident> ...
        case TK_CONST:
        // <TypeAlias>   ::= TYPE <Ident> ...
        case TK_TYPE:
        // <Test>        ::= TEST <String> <Block>
        case TK_TEST:
            parser_throw(p, "unimplemented!", t);
            break;

        default:
            parser_throw(p, "Expected item declaration (fn, struct, etc.)", t);
            break;
    }

    if (is_pub) {
        // TODO: fix this in spec and EBNF
        if (item->node_type == AST_IMPL_DECL || item->node_type == AST_TEST_DECL) {
            parser_throw(p, "'pub' has no effect on this item", start_tok);
        }
        item->is_pub = true;
        // Expand the span backwards to include 'pub'
        span_expand_left(item, start_tok);
    }

    return item;
}