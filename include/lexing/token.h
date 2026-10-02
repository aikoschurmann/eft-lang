#pragma once
#include <stdint.h>  // for uint32_t
#include <stddef.h>  // for size_t
#include "utils.h"   // for Slice
#include "dense_arena_interner.h" // for InternResult


#define EFT_KEYWORDS(X) \
    /* Declarations */ \
    X(TK_FN,        "fn") \
    X(TK_STRUCT,    "struct") \
    X(TK_ENUM,      "enum") \
    X(TK_IMPL,      "impl") \
    X(TK_CONCEPT,   "concept") \
    X(TK_TYPE,      "type") \
    /* Control Flow */ \
    X(TK_IF,        "if") \
    X(TK_ELSE,      "else") \
    X(TK_FOR,       "for") \
    X(TK_WHILE,     "while") \
    X(TK_MATCH,     "match") \
    X(TK_BREAK,     "break") \
    X(TK_CONTINUE,  "continue") \
    X(TK_RETURN,    "return") \
    X(TK_DEFER,     "defer") \
    /* Modifiers */ \
    X(TK_COMPTIME,  "comptime") \
    X(TK_MUT,       "mut") \
    X(TK_CONST,     "const") \
    X(TK_PUB,       "pub") \
    /* Bindings & Modules */ \
    X(TK_AS,        "as") \
    X(TK_IN,        "in") \
    X(TK_IMPORT,    "import") \
    /* Literals */ \
    X(TK_TRUE,      "true") \
    X(TK_FALSE,     "false") \
    X(TK_NULL,      "null") \
    /* Built-in Types */ \
    X(TK_I8,        "i8") \
    X(TK_I16,       "i16") \
    X(TK_I32,       "i32") \
    X(TK_I64,       "i64") \
    X(TK_ISIZE,     "isize") \
    X(TK_U8,        "u8") \
    X(TK_U16,       "u16") \
    X(TK_U32,       "u32") \
    X(TK_U64,       "u64") \
    X(TK_USIZE,     "usize") \
    X(TK_F32,       "f32") \
    X(TK_F64,       "f64") \
    X(TK_BOOL,      "bool") \
    X(TK_VOID,      "void")

#define EFT_PUNCT(X) \
    /* Brackets */ \
    X(TK_LPAREN,    "(") \
    X(TK_RPAREN,    ")") \
    X(TK_LBRACE,    "{") \
    X(TK_RBRACE,    "}") \
    X(TK_LBRACKET,  "[") \
    X(TK_RBRACKET,  "]") \
    /* Punctuation */ \
    X(TK_COMMA,     ",") \
    X(TK_COLON,     ":") \
    X(TK_SEMICOLON, ";") \
    X(TK_DOT,       ".") \
    /* Arrows */ \
    X(TK_ARROW,     "->") \
    X(TK_FAT_ARROW, "=>") \
    /* Assignment */ \
    X(TK_EQ,        "=") \
    X(TK_COLON_EQ,  ":=") \
    /* Arithmetic */ \
    X(TK_PLUS,      "+") \
    X(TK_MINUS,     "-") \
    X(TK_STAR,      "*") \
    X(TK_SLASH,     "/") \
    X(TK_PERCENT,   "%") \
    /* Compound */ \
    X(TK_PLUS_EQ,   "+=") \
    X(TK_MINUS_EQ,  "-=") \
    X(TK_STAR_EQ,   "*=") \
    X(TK_SLASH_EQ,  "/=") \
    X(TK_PERCENT_EQ,"%=") \
    /* Comparison */ \
    X(TK_EQ_EQ,     "==") \
    X(TK_BANG_EQ,   "!=") \
    X(TK_LESS,      "<") \
    X(TK_LESS_EQ,   "<=") \
    X(TK_GREATER,   ">") \
    X(TK_GREATER_EQ,">=") \
    /* Logical */ \
    X(TK_AND_AND,   "&&") \
    X(TK_OR_OR,     "||") \
    /* Bitwise */ \
    X(TK_AMPERSAND, "&") \
    X(TK_PIPE,      "|") \
    X(TK_CARET,     "^") \
    X(TK_TILDE,     "~") \
    X(TK_SHL,       "<<") \
    X(TK_SHR,       ">>") \
    /* Eft-Specific */ \
    X(TK_BANG,      "!") \
    X(TK_QUESTION,  "?") \
    X(TK_DOT_DOT,   "..")

#define EFT_DYNAMIC(X) \
    /* Literals & Identifiers */ \
    X(TK_IDENT,         "identifier") \
    X(TK_INT_LIT,       "int_lit") \
    X(TK_FLOAT_LIT,     "float_lit") \
    X(TK_STR_LIT,       "str_lit") \
    X(TK_BYTE_STR_LIT,  "byte_str_lit") \
    X(TK_RUNE_LIT,      "rune_lit") \
    X(TK_BYTE_LIT,      "byte_lit") \
    /* System */ \
    X(TK_COMMENT,       "comment") \
    X(TK_ERROR,         "error") \
    X(TK_UNKNOWN,       "unknown") \
    X(TK_EOF,           "EOF")

#define EFT_ALL_TOKENS(X) \
    EFT_KEYWORDS(X) \
    EFT_PUNCT(X) \
    EFT_DYNAMIC(X)



// ------------------------------
// Token type enum
// ------------------------------
typedef enum {
    // Tell X to only emit the first argument (the enum name) followed by a comma
    #define X(name, str) name,
    EFT_ALL_TOKENS(X)
    #undef X
    TK_COUNT // sentinel
} TokenKind;


// ------------------------------
// Token struct
// ------------------------------
typedef struct {
    TokenKind type;   // what kind of token
    Slice slice;     /**< Non-owning view into the source buffer. Pointer remains valid as long as the source buffer (usually arena-owned) is alive. */
    Span span;        // position in source for error reporting
    InternResult *record;
} Token;
