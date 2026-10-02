#include "ast.h"
#include <stdio.h>

AstNode *ast_create_node(AstNodeType type, Arena *arena) {
    AstNode *node = arena_calloc(arena, sizeof(AstNode));
    if (node) {
        node->node_type = type;
        node->span = (Span){0, 0, 0, 0};
        node->type = NULL;
    }
    return node;
}

static void print_indent(int depth, bool is_last, bool *parent_is_last) {
    for (int i = 0; i < depth; i++) {
        if (parent_is_last[i]) printf("    ");
        else printf("│   ");
    }
    if (is_last) printf("└── ");
    else printf("├── ");
}

static const char *op_to_string(OpKind op) {
    switch (op) {
        case OP_NONE: return "none";
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_DIV: return "/";
        case OP_MOD: return "%";
        case OP_EQ: return "==";
        case OP_NEQ: return "!=";
        case OP_LT: return "<";
        case OP_LTE: return "<=";
        case OP_GT: return ">";
        case OP_GTE: return ">=";
        case OP_AND: return "and";
        case OP_OR: return "or";
        case OP_BIT_AND: return "&";
        case OP_BIT_OR: return "|";
        case OP_BIT_XOR: return "^";
        case OP_SHL: return "<<";
        case OP_SHR: return ">>";
        case OP_COALESCE: return "??";
        case OP_NEG: return "-";
        case OP_NOT: return "!";
        case OP_REF: return "&";
        case OP_REF_MUT: return "&mut";
        case OP_DEREF: return ".*";
        case OP_COMPTIME: return "comptime";
        case OP_PROPAGATE: return "try";
        case OP_FORCE_UNWRAP: return "catch unreachable";
        case OP_ASSIGN: return "=";
        case OP_ADD_ASSIGN: return "+=";
        case OP_SUB_ASSIGN: return "-=";
        case OP_MUL_ASSIGN: return "*=";
        case OP_DIV_ASSIGN: return "/=";
        case OP_MOD_ASSIGN: return "%=";
        case OP_BIT_AND_ASSIGN: return "&=";
        case OP_BIT_OR_ASSIGN: return "|=";
        case OP_BIT_XOR_ASSIGN: return "^=";
        case OP_SHL_ASSIGN: return "<<=";
        case OP_SHR_ASSIGN: return ">>=";
        default: return "?";
    }
}


static const char *token_kind_to_string(TokenKind kind) {
    switch (kind) {
#define X(name, str) case name: return str;
        EFT_ALL_TOKENS(X)
#undef X
        default: return "?";
    }
}

static void ast_print_internal(AstNode *node, int depth, bool *parent_is_last) {
    if (!node) return;

    int num_fields = 0;

#undef AST_FIELD_NODE
#undef AST_FIELD_INTERN
#undef AST_FIELD_DYNARRAY
#undef AST_FIELD_BOOL
#undef AST_FIELD_OP
#undef AST_FIELD_SLICE
#undef AST_FIELD_KIND
#undef AST_FIELD_TOKEN
#undef AST_FIELD_LITERAL_VALUE

    /* Pass 1: Count non-null fields */
#define AST_FIELD_NODE(f) if (_n_->f) num_fields++;
#define AST_FIELD_INTERN(f) if (_n_->f) num_fields++;
#define AST_FIELD_DYNARRAY(f) if (_n_->f && _n_->f->count > 0) num_fields++;
#define AST_FIELD_BOOL(f) num_fields++;
#define AST_FIELD_OP(f) num_fields++;
#define AST_FIELD_SLICE(f) num_fields++;
#define AST_FIELD_KIND(f) num_fields++;
#define AST_FIELD_TOKEN(f) num_fields++;
#define AST_FIELD_LITERAL_VALUE(f) num_fields++;

#define PRINT_CASE_COUNT(enum_val, name, fields) \
    case enum_val: { \
        Ast_##name *_n_ = &node->data.name; \
        (void)_n_; \
        fields \
        break; \
    }

    switch (node->node_type) {
        AST_NODES(PRINT_CASE_COUNT)
    }

    /* Pass 2: Print */
#undef AST_FIELD_NODE
#undef AST_FIELD_INTERN
#undef AST_FIELD_DYNARRAY
#undef AST_FIELD_BOOL
#undef AST_FIELD_OP
#undef AST_FIELD_SLICE
#undef AST_FIELD_KIND
#undef AST_FIELD_TOKEN
#undef AST_FIELD_LITERAL_VALUE

    int field_idx = 0;

#define AST_FIELD_NODE(f) \
    if (_n_->f) { \
        field_idx++; \
        bool is_last = (field_idx == num_fields); \
        print_indent(depth, is_last, parent_is_last); \
        printf("\x1b[90m" #f ": \x1b[0m"); \
        parent_is_last[depth] = is_last; \
        ast_print_internal(_n_->f, depth + 1, parent_is_last); \
    }

#define AST_FIELD_INTERN(f) \
    if (_n_->f) { \
        field_idx++; \
        bool is_last = (field_idx == num_fields); \
        print_indent(depth, is_last, parent_is_last); \
        printf("\x1b[90m" #f ": \x1b[33m'%s'\x1b[0m\n", (const char*)_n_->f->key); \
    }

#define AST_FIELD_DYNARRAY(f) \
    if (_n_->f && _n_->f->count > 0) { \
        field_idx++; \
        bool is_last = (field_idx == num_fields); \
        print_indent(depth, is_last, parent_is_last); \
        printf("\x1b[90m" #f ": [\x1b[0m\n"); \
        parent_is_last[depth] = is_last; \
        for (size_t i = 0; i < _n_->f->count; i++) { \
            void *ptr = ((void**)_n_->f->data)[i]; \
            bool is_last_elem = (i == _n_->f->count - 1); \
            print_indent(depth + 1, is_last_elem, parent_is_last); \
            parent_is_last[depth + 1] = is_last_elem; \
            if (node->node_type == AST_IMPORT || node->node_type == AST_PATH) { \
                InternResult *res = (InternResult*)ptr; \
                printf("\x1b[33m'%s'\x1b[0m\n", (const char*)res->key); \
            } else { \
                ast_print_internal((AstNode*)ptr, depth + 2, parent_is_last); \
            } \
        } \
    }

#define AST_FIELD_BOOL(f) \
    field_idx++; \
    print_indent(depth, field_idx == num_fields, parent_is_last); \
    printf("\x1b[90m" #f ": \x1b[33m%s\x1b[0m\n", _n_->f ? "true" : "false");

#define AST_FIELD_OP(f) \
    field_idx++; \
    print_indent(depth, field_idx == num_fields, parent_is_last); \
    printf("\x1b[90m" #f ": \x1b[33m'%s'\x1b[0m\n", op_to_string(_n_->f));

#define AST_FIELD_SLICE(f) \
    field_idx++; \
    print_indent(depth, field_idx == num_fields, parent_is_last); \
    printf("\x1b[90m" #f ": \x1b[33m'%.*s'\x1b[0m\n", (int)_n_->f.len, _n_->f.ptr);

#define AST_FIELD_KIND(f) \
    field_idx++; \
    print_indent(depth, field_idx == num_fields, parent_is_last); \
    printf("\x1b[90m" #f ": \x1b[33m%d\x1b[0m\n", _n_->f);

#define AST_FIELD_TOKEN(f) \
    field_idx++; \
    print_indent(depth, field_idx == num_fields, parent_is_last); \
    printf("\x1b[90m" #f ": \x1b[33m%s\x1b[0m\n", token_kind_to_string(_n_->f));

#define AST_FIELD_LITERAL_VALUE(f) \
    field_idx++; \
    print_indent(depth, field_idx == num_fields, parent_is_last); \
    printf("\x1b[90m" #f ": \x1b[0m"); \
    if (_n_->kind == TK_INT_LIT) printf("\x1b[33m%llu\x1b[0m\n", (unsigned long long)_n_->f.int_val); \
    else if (_n_->kind == TK_FLOAT_LIT) printf("\x1b[33m%f\x1b[0m\n", _n_->f.float_val); \
    else if (_n_->kind == TK_STR_LIT || _n_->kind == TK_C_STR_LIT) printf("\x1b[33m\"%.*s\"\x1b[0m\n", (int)_n_->f.str_val.len, _n_->f.str_val.ptr); \
    else if (_n_->kind == TK_RUNE_LIT) printf("\x1b[33m'%c'\x1b[0m\n", (char)_n_->f.char_val); \
    else if (_n_->kind == TK_TRUE || _n_->kind == TK_FALSE) printf("\x1b[33m%s\x1b[0m\n", _n_->f.bool_val ? "true" : "false"); \
    else printf("?\n");

#define PRINT_CASE_PRINT(enum_val, name, fields) \
    case enum_val: { \
        printf("\x1b[1;36m" #enum_val "\x1b[0m"); \
        if (node->span.start_line > 0) { \
            printf(" [%zu:%zu-%zu:%zu]", \
                (size_t)node->span.start_line, (size_t)node->span.start_col, \
                (size_t)node->span.end_line, (size_t)node->span.end_col); \
        } \
        if (node->type) { \
            printf(" \033[36m<type>\033[0m"); \
        } \
        printf("\n"); \
        Ast_##name *_n_ = &node->data.name; \
        (void)_n_; \
        fields \
        break; \
    }

    switch (node->node_type) {
        AST_NODES(PRINT_CASE_PRINT)
    }
}

void ast_print(AstNode *node, int indent) {
    bool parent_is_last[128] = {0};
    ast_print_internal(node, indent, parent_is_last);
}
