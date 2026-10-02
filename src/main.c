#include <stdio.h>
#include <string.h>
#include "file.h"
#include "arena.h"
#include "lexer.h"
#include "parser.h"
#include "ast_print.h"

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <file.eft>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    char *source = read_file(filename);
    if (!source) {
        fprintf(stderr, "Failed to read file: %s\n", filename);
        return 1;
    }

    Arena *arena = arena_create(4 * 1024 * 1024); // 4MB arena
    Lexer *lexer = lexer_create(source, strlen(source), arena);
    if (!lexer) {
        fprintf(stderr, "Failed to create lexer\n");
        return 1;
    }

    if (!lexer_lex_all(lexer)) {
        fprintf(stderr, "Lexing failed.\n");
        arena_destroy(arena);
        free_file_content(source);
        return 1;
    }

    Parser *parser = parser_create(lexer->tokens, (char *)filename, arena);
    if (!parser) {
        fprintf(stderr, "Failed to create parser\n");
        arena_destroy(arena);
        free_file_content(source);
        return 1;
    }

    AstNode *program = parse_program(parser);
    if (!program) {
        // parse_program already printed the error via print_parse_error
        arena_destroy(arena);
        free_file_content(source);
        return 1;
    }

    ast_print(program, 0);

    arena_destroy(arena);
    free_file_content(source);
    return 0;
}
