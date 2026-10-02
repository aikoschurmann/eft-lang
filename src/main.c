#include <stdio.h>
#include <string.h>
#include "file.h"
#include "arena.h"
#include "lexer.h"

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <file.eft>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    char *source = read_file(filename);
    if (!source) {
        printf("Failed to read file: %s\n", filename);
        return 1;
    }

    Arena *arena = arena_create(1024 * 1024); // 1MB arena
    Lexer *lexer = lexer_create(source, strlen(source), arena);
    if (!lexer) {
        printf("Failed to create lexer\n");
        return 1;
    }

    printf("Lexing file: %s\n", filename);
    if (!lexer_lex_all(lexer)) {
        printf("Lexing failed with an error.\n");
    }

    size_t count = 0;
    Token *tokens = lexer_get_tokens(lexer, &count);
    
    printf("Generated %zu tokens:\n", count);
    for (size_t i = 0; i < count; i++) {
        print_token(&tokens[i]);
    }

    lexer_destroy(lexer);
    arena_destroy(arena);
    free_file_content(source);

    return 0;
}
