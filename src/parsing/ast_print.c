#include "ast.h"
#include "ast_print.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Tree-drawing helpers                                                 */
/* ------------------------------------------------------------------ */

#define MAX_DEPTH 128

static void print_indent(int depth, bool is_last, bool *pil) {
    for (int i = 0; i < depth - 1; i++) {
        printf(pil[i] ? "    " : "\xe2\x94\x82   "); // "    " or "│   "
    }
    if (depth > 0)
        printf(is_last ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 " : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 "); // "└── " or "├── "
}

/* Print the node name line: "NodeType [span] pub */
static void print_node_header(AstNode *node) {
    printf("\x1b[1;36m%s\x1b[0m", ast_node_type_name(node->node_type));
    if (node->span.start_line > 0)
        printf(" \x1b[90m[%zu:%zu-%zu:%zu]\x1b[0m",
            (size_t)node->span.start_line, (size_t)node->span.start_col,
            (size_t)node->span.end_line,   (size_t)node->span.end_col);
    if (node->is_pub)
        printf(" \x1b[35mpub\x1b[0m");
    printf("\n");
}

/* Intern key -> printable string using the Slice stored at key */
static void print_intern(InternResult *r) {
    if (!r || !r->key) { printf("?"); return; }
    Slice *sl = (Slice *)r->key;
    printf("\x1b[33m'%.*s'\x1b[0m", (int)sl->len, sl->ptr);
}

/* Count non-null children so we know who gets └── */
typedef struct { int n; int cur; } Seq;
static Seq seq_of(int count) { return (Seq){count, 0}; }
static bool seq_last(Seq *s) { return ++(s->cur) == s->n; }

/* Forward declare the recursive printer */
static void print_node(AstNode *node, int depth, bool *pil);

/* Print a labelled child node */
#define CHILD(label, child_node) do { \
    if (child_node) { \
        bool _last = seq_last(&sq); \
        print_indent(depth + 1, _last, pil); \
        printf("\x1b[90m" label ":\x1b[0m "); \
        pil[depth] = _last; \
        print_node((child_node), depth + 1, pil); \
    } \
} while(0)

/* Print a labelled DynArray of AstNode* */
#define CHILDREN(label, arr) do { \
    if ((arr) && (arr)->count > 0) { \
        bool _last_arr = seq_last(&sq); \
        print_indent(depth + 1, _last_arr, pil); \
        printf("\x1b[90m" label ": [\x1b[0m\n"); \
        pil[depth] = _last_arr; \
        for (size_t _i = 0; _i < (arr)->count; _i++) { \
            AstNode *_ch = ((AstNode **)((arr)->data))[_i]; \
            bool _last_ch = (_i == (arr)->count - 1); \
            print_indent(depth + 2, _last_ch, pil); \
            pil[depth + 1] = _last_ch; \
            print_node(_ch, depth + 2, pil); \
        } \
    } \
} while(0)

/* Print a labelled DynArray of InternResult* (e.g. path segments) */
#define INTERN_LIST(label, arr) do { \
    if ((arr) && (arr)->count > 0) { \
        bool _last_arr = seq_last(&sq); \
        print_indent(depth + 1, _last_arr, pil); \
        printf("\x1b[90m" label ": [\x1b[0m\n"); \
        pil[depth] = _last_arr; \
        for (size_t _i = 0; _i < (arr)->count; _i++) { \
            InternResult *_r = ((InternResult **)((arr)->data))[_i]; \
            bool _last_ch = (_i == (arr)->count - 1); \
            print_indent(depth + 2, _last_ch, pil); \
            print_intern(_r); printf("\n"); \
        } \
    } \
} while(0)

/* Print a labelled intern (single identifier) */
#define INTERN(label, r) do { \
    if (r) { \
        print_indent(depth + 1, seq_last(&sq), pil); \
        printf("\x1b[90m" label ": \x1b[0m"); \
        print_intern(r); printf("\n"); \
    } \
} while(0)

/* Print a bool field — only when true */
#define FLAG(label, val) do { \
    if (val) { \
        print_indent(depth + 1, seq_last(&sq), pil); \
        printf("\x1b[90m" label ": \x1b[33mtrue\x1b[0m\n"); \
    } \
} while(0)

/* Print a string slice field */
#define SLICE_FIELD(label, sl) do { \
    print_indent(depth + 1, seq_last(&sq), pil); \
    printf("\x1b[90m" label ": \x1b[33m'%.*s'\x1b[0m\n", (int)(sl).len, (sl).ptr); \
} while(0)

/* Print an op field */
#define OP_FIELD(label, op) do { \
    print_indent(depth + 1, seq_last(&sq), pil); \
    printf("\x1b[90m" label ": \x1b[33m'%s'\x1b[0m\n", ast_op_name(op)); \
} while(0)

/* ------------------------------------------------------------------ */
/* Op name helper                                                       */
/* ------------------------------------------------------------------ */

const char *ast_op_name(OpKind op) {
    switch (op) {
        case OP_ADD: return "+";    case OP_SUB: return "-";
        case OP_MUL: return "*";    case OP_DIV: return "/";
        case OP_MOD: return "%";    case OP_EQ:  return "==";
        case OP_NEQ: return "!=";   case OP_LT:  return "<";
        case OP_LTE: return "<=";   case OP_GT:  return ">";
        case OP_GTE: return ">=";   case OP_AND: return "and";
        case OP_OR:  return "or";   case OP_NOT: return "!";
        case OP_NEG: return "-";    case OP_REF: return "&";
        case OP_REF_MUT:    return "&mut";
        case OP_DEREF:      return ".*";
        case OP_ASSIGN:     return "=";
        case OP_ADD_ASSIGN: return "+=";  case OP_SUB_ASSIGN: return "-=";
        case OP_MUL_ASSIGN: return "*=";  case OP_DIV_ASSIGN: return "/=";
        case OP_MOD_ASSIGN: return "%=";
        case OP_BIT_AND:    return "&";   case OP_BIT_OR:    return "|";
        case OP_BIT_XOR:    return "^";   case OP_SHL:       return "<<";
        case OP_SHR:        return ">>";
        case OP_BIT_AND_ASSIGN: return "&="; case OP_BIT_OR_ASSIGN: return "|=";
        case OP_BIT_XOR_ASSIGN: return "^="; case OP_SHL_ASSIGN:    return "<<=";
        case OP_SHR_ASSIGN: return ">>=";
        case OP_COALESCE:   return "??";
        case OP_COMPTIME:   return "comptime";
        case OP_PROPAGATE:  return "?";
        case OP_FORCE_UNWRAP: return "!";
        default: return "?";
    }
}

/* ------------------------------------------------------------------ */
/* Node type name                                                       */
/* ------------------------------------------------------------------ */

const char *ast_node_type_name(AstNodeType t) {
#define X(e, n, f) case e: return #e;
    switch (t) { AST_NODES(X) }
#undef X
    return "AST_UNKNOWN";
}

/* ------------------------------------------------------------------ */
/* Count visible children for a given node                              */
/* ------------------------------------------------------------------ */

static int count_children(AstNode *node) {
    if (!node) return 0;
    int n = 0;

/* Node pointer: count if not NULL */
#define N(f)        if (f) n++

/* Node Array (DynArray): count if not NULL and has elements */
#define NA(f)       if ((f) && (f)->count > 0) n++

/* Boolean flag: count if true */
#define B(f)        if (f) n++

/* Slice/String field: always count */
#define S(f)        n++

/* Operator field: always count */
#define OP(f)       n++

    switch (node->node_type) {
        case AST_PROGRAM:       { Ast_program       *d = &node->data.program;        NA(d->program_items); break; }
        case AST_IMPORT:        { Ast_import_decl   *d = &node->data.import_decl;    N(d->path); N(d->alias); NA(d->items); B(d->is_glob); break; }
        case AST_IMPORT_ITEM:   { Ast_import_item   *d = &node->data.import_item;    N(d->name); N(d->alias); break; }
        case AST_IMPORT_C:      { n = 1; break; }
        case AST_EXTERN_BLOCK:  { Ast_extern_block  *d = &node->data.extern_block;   NA(d->items); break; }
        case AST_TYPE_ALIAS:    { Ast_type_alias    *d = &node->data.type_alias;     N(d->name); NA(d->generic_params); N(d->type); break; }
        case AST_CONST_DECL:    { Ast_const_decl    *d = &node->data.const_decl;     N(d->name); N(d->type); N(d->expr); break; }
        case AST_FN_DECL:       { Ast_fn_decl       *d = &node->data.fn_decl;        N(d->name); B(d->is_comptime); NA(d->generic_params); NA(d->params); N(d->return_type); N(d->where_clauses); N(d->body); break; }
        case AST_STRUCT_DECL:   { Ast_struct_decl   *d = &node->data.struct_decl;    N(d->name); B(d->is_packed); NA(d->generic_params); NA(d->where_clauses); NA(d->fields); break; }
        case AST_ENUM_DECL:     { Ast_enum_decl     *d = &node->data.enum_decl;      N(d->name); NA(d->generic_params); NA(d->variants); break; }
        case AST_CONCEPT_DECL:  { Ast_concept_decl  *d = &node->data.concept_decl;   N(d->name); NA(d->generic_params); NA(d->bounds); NA(d->items); break; }
        case AST_IMPL_DECL:     { Ast_impl_decl     *d = &node->data.impl_decl;      NA(d->generic_params); N(d->type); NA(d->bounds); NA(d->where_clauses); NA(d->items); break; }
        case AST_TEST_DECL:     { n = 2; break; }
        case AST_LET_STMT:      { Ast_let_stmt      *d = &node->data.let_stmt;       B(d->is_mut); N(d->pattern); N(d->type); N(d->expr); break; }
        case AST_ASSIGN_STMT:   { n = 3; break; }
        case AST_EXPR_STMT:     { Ast_expr_stmt     *d = &node->data.expr_stmt;      N(d->expr); B(d->is_comptime); break; }
        case AST_DEFER_STMT:    { n = 1; break; }
        case AST_ERRDEFER_STMT: { n = 1; break; }
        case AST_RETURN_STMT:   { Ast_return_stmt   *d = &node->data.return_stmt;    N(d->expr); break; }
        case AST_BREAK_STMT:    { Ast_break_stmt    *d = &node->data.break_stmt;     N(d->label); N(d->expr); break; }
        case AST_CONTINUE_STMT: { Ast_continue_stmt *d = &node->data.continue_stmt;  N(d->label); break; }
        case AST_ASSERT_STMT:   { n = 1; break; }
        case AST_UNSAFE_BLOCK:  { n = 1; break; }
        case AST_BLOCK:         { Ast_block         *d = &node->data.block;          N(d->label); NA(d->stmts); break; }
        case AST_RANGE_EXPR:    { Ast_range_expr    *d = &node->data.range_expr;     N(d->start); N(d->end); B(d->inclusive); break; }
        case AST_BINARY_EXPR:   { n = 3; break; }
        case AST_UNARY_EXPR:    { n = 2; break; }
        case AST_POSTFIX_EXPR:  { n = 2; break; }
        case AST_CAST_EXPR:     { n = 2; break; }
        case AST_CALL_EXPR:     { Ast_call_expr     *d = &node->data.call_expr;      N(d->callee); NA(d->args); break; }
        case AST_INDEX_EXPR:    { n = 2; break; }
        case AST_MEMBER_EXPR:   { Ast_member_expr   *d = &node->data.member_expr;    N(d->target); N(d->member); NA(d->generic_args); break; }
        case AST_IDENTIFIER:    { Ast_identifier    *d = &node->data.identifier;     N(d->name); NA(d->generic_args); break; }
        case AST_PATH:          { Ast_path          *d = &node->data.path;           NA(d->segments); break; }
        case AST_STRUCT_LITERAL:{ Ast_struct_literal*d = &node->data.struct_literal; N(d->path); NA(d->fields); break; }
        case AST_ARRAY_TUPLE_LITERAL: { Ast_array_tuple_literal *d = &node->data.array_tuple_literal; NA(d->elements); break; }
        case AST_LAMBDA_EXPR:   { Ast_lambda_expr   *d = &node->data.lambda_expr;    NA(d->captures); NA(d->params); N(d->expr); break; }
        case AST_IF_EXPR:       { Ast_if_expr       *d = &node->data.if_expr;        N(d->cond); N(d->then_block); N(d->else_node); break; }
        case AST_WHILE_EXPR:    { Ast_while_expr    *d = &node->data.while_expr;     N(d->label); N(d->cond); N(d->body); break; }
        case AST_FOR_EXPR:      { Ast_for_expr      *d = &node->data.for_expr;       N(d->label); N(d->pattern); N(d->iterable); N(d->body); break; }
        case AST_LOOP_EXPR:     { Ast_loop_expr     *d = &node->data.loop_expr;      N(d->label); N(d->body); break; }
        case AST_MATCH_EXPR:    { Ast_match_expr    *d = &node->data.match_expr;     N(d->expr); NA(d->arms); break; }
        case AST_SPAWN_EXPR:    { n = 1; break; }
        case AST_TYPE_NODE:     { n = 1 + (node->data.type_node.is_mut ? 1 : 0) + (node->data.type_node.target ? 1 : 0); break; }
        case AST_PARAM:         { Ast_param         *d = &node->data.param;          N(d->name); N(d->type); N(d->default_val); break; }
        case AST_FIELD:         { Ast_field         *d = &node->data.field;          N(d->name); N(d->type); N(d->default_val); break; }
        case AST_MATCH_ARM:     { Ast_match_arm     *d = &node->data.match_arm;      NA(d->patterns); N(d->guard); N(d->expr); break; }
        case AST_LITERAL:       { n = 2; break; }
        default: break;
    }
#undef N
#undef NA
#undef B
#undef S
#undef OP
    return n;
}

/* ------------------------------------------------------------------ */
/* Main recursive printer                                               */
/* ------------------------------------------------------------------ */

static void print_node(AstNode *node, int depth, bool *pil) {
    if (!node) return;
    print_node_header(node);

    int nc = count_children(node);
    Seq sq = seq_of(nc);

    switch (node->node_type) {
        case AST_PROGRAM: {
            Ast_program *d = &node->data.program;
            CHILDREN("program_items", d->program_items);
            break;
        }
        case AST_IMPORT: {
            Ast_import_decl *d = &node->data.import_decl;
            CHILD("path",  d->path);
            INTERN("alias", d->alias);
            FLAG("is_glob", d->is_glob);
            CHILDREN("items", d->items);
            break;
        }
        case AST_IMPORT_ITEM: {
            Ast_import_item *d = &node->data.import_item;
            INTERN("name",  d->name);
            INTERN("alias", d->alias);
            break;
        }
        case AST_IMPORT_C: {
            Ast_import_c_decl *d = &node->data.import_c_decl;
            SLICE_FIELD("path", d->name);
            break;
        }
        case AST_EXTERN_BLOCK: {
            Ast_extern_block *d = &node->data.extern_block;
            CHILDREN("items", d->items);
            break;
        }
        case AST_TYPE_ALIAS: {
            Ast_type_alias *d = &node->data.type_alias;
            INTERN("name", d->name);
            CHILDREN("generics", d->generic_params);
            CHILD("type", d->type);
            break;
        }
        case AST_CONST_DECL: {
            Ast_const_decl *d = &node->data.const_decl;
            INTERN("name", d->name);
            CHILD("type", d->type);
            CHILD("expr", d->expr);
            break;
        }
        case AST_FN_DECL: {
            Ast_fn_decl *d = &node->data.fn_decl;
            INTERN("name", d->name);
            FLAG("comptime", d->is_comptime);
            CHILDREN("generics", d->generic_params);
            CHILDREN("params",   d->params);
            CHILD("return_type", d->return_type);
            CHILDREN("where", d->where_clauses);
            CHILD("body",        d->body);
            break;
        }
        case AST_STRUCT_DECL: {
            Ast_struct_decl *d = &node->data.struct_decl;
            INTERN("name", d->name);
            FLAG("packed", d->is_packed);
            CHILDREN("generics", d->generic_params);
            CHILDREN("where",    d->where_clauses);
            CHILDREN("fields",   d->fields);
            break;
        }
        case AST_ENUM_DECL: {
            Ast_enum_decl *d = &node->data.enum_decl;
            INTERN("name", d->name);
            CHILDREN("generics",  d->generic_params);
            CHILDREN("variants",  d->variants);
            break;
        }
        case AST_CONCEPT_DECL: {
            Ast_concept_decl *d = &node->data.concept_decl;
            INTERN("name", d->name);
            CHILDREN("generics", d->generic_params);
            CHILDREN("bounds",   d->bounds);
            CHILDREN("items",    d->items);
            break;
        }
        case AST_IMPL_DECL: {
            Ast_impl_decl *d = &node->data.impl_decl;
            CHILDREN("generics", d->generic_params);
            CHILD("type",        d->type);
            CHILDREN("bounds",   d->bounds);
            CHILDREN("where",    d->where_clauses);
            CHILDREN("items",    d->items);
            break;
        }
        case AST_TEST_DECL: {
            Ast_test_decl *d = &node->data.test_decl;
            SLICE_FIELD("name", d->name);
            CHILD("body", d->block);
            break;
        }
        case AST_LET_STMT: {
            Ast_let_stmt *d = &node->data.let_stmt;
            FLAG("mut",     d->is_mut);
            CHILD("pattern", d->pattern);
            CHILD("type",    d->type);
            CHILD("expr",    d->expr);
            break;
        }
        case AST_ASSIGN_STMT: {
            Ast_assign_stmt *d = &node->data.assign_stmt;
            OP_FIELD("op", d->op);
            CHILD("lvalue", d->lvalue);
            CHILD("rvalue", d->rvalue);
            break;
        }
        case AST_EXPR_STMT: {
            Ast_expr_stmt *d = &node->data.expr_stmt;
            FLAG("comptime", d->is_comptime);
            CHILD("expr", d->expr);
            break;
        }
        case AST_DEFER_STMT: {
            CHILD("expr", node->data.defer_stmt.expr);
            break;
        }
        case AST_ERRDEFER_STMT: {
            CHILD("expr", node->data.errdefer_stmt.expr);
            break;
        }
        case AST_RETURN_STMT: {
            CHILD("expr", node->data.return_stmt.expr);
            break;
        }
        case AST_BREAK_STMT: {
            Ast_break_stmt *d = &node->data.break_stmt;
            INTERN("label", d->label);
            CHILD("expr",   d->expr);
            break;
        }
        case AST_CONTINUE_STMT: {
            INTERN("label", node->data.continue_stmt.label);
            break;
        }
        case AST_ASSERT_STMT: {
            CHILD("expr", node->data.assert_stmt.expr);
            break;
        }
        case AST_UNSAFE_BLOCK: {
            CHILD("block", node->data.unsafe_block.block);
            break;
        }
        case AST_BLOCK: {
            Ast_block *d = &node->data.block;
            INTERN("label", d->label);
            CHILDREN("stmts", d->stmts);
            break;
        }
        case AST_RANGE_EXPR: {
            Ast_range_expr *d = &node->data.range_expr;
            FLAG("inclusive", d->inclusive);
            CHILD("start", d->start);
            CHILD("end",   d->end);
            break;
        }
        case AST_BINARY_EXPR: {
            Ast_binary_expr *d = &node->data.binary_expr;
            OP_FIELD("op",    d->op);
            CHILD("left",  d->left);
            CHILD("right", d->right);
            break;
        }
        case AST_UNARY_EXPR: {
            Ast_unary_expr *d = &node->data.unary_expr;
            OP_FIELD("op",   d->op);
            CHILD("expr", d->expr);
            break;
        }
        case AST_POSTFIX_EXPR: {
            Ast_postfix_expr *d = &node->data.postfix_expr;
            OP_FIELD("op",   d->op);
            CHILD("expr", d->expr);
            break;
        }
        case AST_CAST_EXPR: {
            Ast_cast_expr *d = &node->data.cast_expr;
            CHILD("expr", d->expr);
            CHILD("type", d->type);
            break;
        }
        case AST_CALL_EXPR: {
            Ast_call_expr *d = &node->data.call_expr;
            CHILD("callee", d->callee);
            CHILDREN("args", d->args);
            break;
        }
        case AST_INDEX_EXPR: {
            Ast_index_expr *d = &node->data.index_expr;
            CHILD("target", d->target);
            CHILD("index",  d->index);
            break;
        }
        case AST_MEMBER_EXPR: {
            Ast_member_expr *d = &node->data.member_expr;
            CHILD("target",  d->target);
            INTERN("member", d->member);
            CHILDREN("generics", d->generic_args);
            break;
        }
        case AST_IDENTIFIER: {
            Ast_identifier *d = &node->data.identifier;
            INTERN("name", d->name);
            CHILDREN("generics", d->generic_args);
            break;
        }
        case AST_PATH: {
            Ast_path *d = &node->data.path;
            INTERN_LIST("segments", d->segments);
            break;
        }
        case AST_STRUCT_LITERAL: {
            Ast_struct_literal *d = &node->data.struct_literal;
            CHILD("path",    d->path);
            CHILDREN("fields", d->fields);
            break;
        }
        case AST_ARRAY_TUPLE_LITERAL: {
            Ast_array_tuple_literal *d = &node->data.array_tuple_literal;
            CHILDREN("elements", d->elements);
            break;
        }
        case AST_LAMBDA_EXPR: {
            Ast_lambda_expr *d = &node->data.lambda_expr;
            CHILDREN("captures", d->captures);
            CHILDREN("params",   d->params);
            CHILD("expr",        d->expr);
            break;
        }
        case AST_IF_EXPR: {
            Ast_if_expr *d = &node->data.if_expr;
            CHILD("cond",  d->cond);
            CHILD("then",  d->then_block);
            CHILD("else",  d->else_node);
            break;
        }
        case AST_WHILE_EXPR: {
            Ast_while_expr *d = &node->data.while_expr;
            INTERN("label", d->label);
            CHILD("cond", d->cond);
            CHILD("body", d->body);
            break;
        }
        case AST_FOR_EXPR: {
            Ast_for_expr *d = &node->data.for_expr;
            INTERN("label",    d->label);
            CHILD("pattern",  d->pattern);
            CHILD("iterable", d->iterable);
            CHILD("body",     d->body);
            break;
        }
        case AST_LOOP_EXPR: {
            Ast_loop_expr *d = &node->data.loop_expr;
            INTERN("label", d->label);
            CHILD("body",   d->body);
            break;
        }
        case AST_MATCH_EXPR: {
            Ast_match_expr *d = &node->data.match_expr;
            CHILD("expr",    d->expr);
            CHILDREN("arms", d->arms);
            break;
        }
        case AST_SPAWN_EXPR: {
            CHILD("expr", node->data.spawn_expr.expr);
            break;
        }
        case AST_TYPE_NODE: {
            Ast_type_node *d = &node->data.type_node;
            const char *kind_str = "?";
            switch ((AstTypeKind)d->kind) {
                case AST_TYPE_PATH:        kind_str = "path";     break;
                case AST_TYPE_PTR:         kind_str = "ptr";      break;
                case AST_TYPE_ARRAY_SLICE: kind_str = "slice";    break;
                case AST_TYPE_FUNC:        kind_str = "fn";       break;
                case AST_TYPE_TUPLE:       kind_str = "tuple";    break;
                case AST_TYPE_OPTIONAL:    kind_str = "?";        break;
                case AST_TYPE_ERROR_UNION: kind_str = "!";        break;
                case AST_TYPE_NEVER:       kind_str = "never";    break;
            }
            print_indent(depth + 1, seq_last(&sq), pil);
            printf("\x1b[90mkind: \x1b[33m%s\x1b[0m\n", kind_str);
            
            FLAG("mut",     d->is_mut);
            CHILD("target", d->target);
            break;
        }
        case AST_PARAM: {
            Ast_param *d = &node->data.param;
            INTERN("name",       d->name);
            CHILD("type",        d->type);
            CHILD("default",     d->default_val);
            break;
        }
        case AST_FIELD: {
            Ast_field *d = &node->data.field;
            INTERN("name",   d->name);
            CHILD("type",    d->type);
            CHILD("default", d->default_val);
            break;
        }
        case AST_MATCH_ARM: {
            Ast_match_arm *d = &node->data.match_arm;
            CHILDREN("patterns", d->patterns);
            CHILD("guard", d->guard);
            CHILD("expr",  d->expr);
            break;
        }
        case AST_LITERAL: {
            Ast_literal *d = &node->data.literal;
            /* kind */
            const char *kind_str = "?";
            switch (d->kind) {
                case TK_INT_LIT:      kind_str = "int";    break;
                case TK_FLOAT_LIT:    kind_str = "float";  break;
                case TK_STR_LIT:      kind_str = "str";    break;
                case TK_C_STR_LIT:    kind_str = "c_str";  break;
                case TK_BYTE_STR_LIT: kind_str = "b_str";  break;
                case TK_RUNE_LIT:     kind_str = "rune";   break;
                case TK_BYTE_LIT:     kind_str = "byte";   break;
                case TK_TRUE:
                case TK_FALSE:        kind_str = "bool";   break;
                case TK_NULL:         kind_str = "null";   break;
                default: break;
            }
            print_indent(depth + 1, seq_last(&sq), pil);
            printf("\x1b[90mkind: \x1b[33m%s\x1b[0m\n", kind_str);
            
            /* value */
            print_indent(depth + 1, seq_last(&sq), pil);
            printf("\x1b[90mvalue: \x1b[33m");
            switch (d->kind) {
                case TK_INT_LIT:   printf("%llu", (unsigned long long)d->value.int_val); break;
                case TK_FLOAT_LIT: printf("%g",   d->value.float_val); break;
                case TK_STR_LIT:
                case TK_C_STR_LIT:
                case TK_BYTE_STR_LIT:
                    printf("\"%.*s\"", (int)d->value.str_val.len, d->value.str_val.ptr); break;
                case TK_RUNE_LIT:
                case TK_BYTE_LIT:  printf("'%c'", (char)d->value.char_val); break;
                case TK_TRUE:      printf("true");  break;
                case TK_FALSE:     printf("false"); break;
                case TK_NULL:      printf("null");  break;
                default:           printf("?");     break;
            }
            printf("\x1b[0m\n");
            break;
        }

        default:
            print_indent(depth + 1, true, pil);
            printf("\x1b[31m(unhandled: %d)\x1b[0m\n", node->node_type);
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Public entry point                                                   */
/* ------------------------------------------------------------------ */

void ast_print(AstNode *node, int indent) {
    bool pil[MAX_DEPTH] = {0};
    print_node(node, indent, pil);
}