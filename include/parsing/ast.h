#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "token.h"
#include "dynamic_array.h"
#include "utils.h"
#include "dense_arena_interner.h"
#include "arena.h"

/* ----------------------- Forward Declarations ----------------------- */ \
typedef struct AstNode AstNode;
typedef struct Symbol Symbol;

/* ----------------------- Operators ----------------------- */ \
typedef enum {
    OP_NONE,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_EQ, OP_NEQ, OP_LT, OP_LTE, OP_GT, OP_GTE,
    OP_AND, OP_OR, OP_BIT_AND, OP_BIT_OR, OP_BIT_XOR, OP_SHL, OP_SHR,
    OP_COALESCE,
    OP_NEG, OP_NOT, OP_REF, OP_REF_MUT, OP_DEREF, OP_COMPTIME,
    OP_PROPAGATE, OP_FORCE_UNWRAP,
    OP_ASSIGN, OP_ADD_ASSIGN, OP_SUB_ASSIGN, OP_MUL_ASSIGN, OP_DIV_ASSIGN, OP_MOD_ASSIGN,
    OP_BIT_AND_ASSIGN, OP_BIT_OR_ASSIGN, OP_BIT_XOR_ASSIGN, OP_SHL_ASSIGN, OP_SHR_ASSIGN
} OpKind;


typedef struct {
    union {
        uint64_t int_val;
        double float_val;
        Slice str_val;
        uint32_t char_val;
        bool bool_val;
    };
} LiteralValue;

typedef enum {
    AST_TYPE_PATH, AST_TYPE_PTR, AST_TYPE_ARRAY_SLICE,
    AST_TYPE_FUNC, AST_TYPE_TUPLE, AST_TYPE_OPTIONAL,
    AST_TYPE_ERROR_UNION, AST_TYPE_NEVER
} AstTypeKind;

/* ----------------------- AST X-Macros ----------------------- */ \

#define AST_NODES(X) \
    /* <Program> ::= { <Item> } */ \
    X(AST_PROGRAM, program, \
        AST_FIELD_DYNARRAY(program_items) /* AstNode* */ \
    ) \
    X(AST_IMPORT, import_decl, \
        AST_FIELD_NODE(path) \
        AST_FIELD_INTERN(alias) \
        AST_FIELD_BOOL(is_glob) \
        AST_FIELD_DYNARRAY(items) /* AstNode* (AST_IMPORT_ITEM) */ \
    ) \
    /* <ImportItem> ::= <Ident> [ AS <Ident> ] */ \
    X(AST_IMPORT_ITEM, import_item, \
        AST_FIELD_INTERN(name) \
        AST_FIELD_INTERN(alias) \
    ) \
    /* <ImportC> ::= IMPORT_C <String> SEMICOLON */ \
    X(AST_IMPORT_C, import_c_decl, \
        AST_FIELD_SLICE(name) \
    ) \
    /* <ExternBlock> ::= EXTERN <String> L_BRACE { <FnSig> SEMICOLON } R_BRACE */ \
    X(AST_EXTERN_BLOCK, extern_block, \
        AST_FIELD_DYNARRAY(items) /* AstNode* */ \
    ) \
    /* <TypeAlias> ::= TYPE <Ident> [ <Generics> ] ASSIGN <Type> SEMICOLON */ \
    X(AST_TYPE_ALIAS, type_alias, \
        AST_FIELD_INTERN(name) \
        AST_FIELD_DYNARRAY(generic_params) \
        AST_FIELD_NODE(type) \
    ) \
    /* <ConstDecl> ::= CONST <Ident> [ COLON <Type> ] COLON_EQ <Expr> SEMICOLON */ \
    X(AST_CONST_DECL, const_decl, \
        AST_FIELD_INTERN(name) \
        AST_FIELD_NODE(type) \
        AST_FIELD_NODE(expr) \
    ) \
    /* <Fn> ::= [ COMPTIME ] <FnSig> ( <Block> | FAT_ARROW <Expr> SEMICOLON ) */ \
    /* <FnSig> ::= FN <Ident> [ <Generics> ] LPAREN [ <Params> ] RPAREN [ ARROW <Type> ] [ <Where> ] */ \
    X(AST_FN_DECL, fn_decl, \
        AST_FIELD_INTERN(name) \
        AST_FIELD_BOOL(is_comptime) \
        AST_FIELD_DYNARRAY(generic_params) \
        AST_FIELD_DYNARRAY(params) \
        AST_FIELD_NODE(return_type) \
        AST_FIELD_DYNARRAY(where_clauses) \
        AST_FIELD_NODE(body) \
    ) \
    /* <Struct> ::= [ PACKED ] STRUCT <Ident> [ <Generics> ] [ <Where> ] L_BRACE [ <Fields> ] R_BRACE */ \
    X(AST_STRUCT_DECL, struct_decl, \
        AST_FIELD_INTERN(name) \
        AST_FIELD_BOOL(is_packed) \
        AST_FIELD_DYNARRAY(generic_params) \
        AST_FIELD_DYNARRAY(where_clauses) \
        AST_FIELD_DYNARRAY(fields) \
    ) \
    /* <Enum> ::= ENUM <Ident> [ <Generics> ] L_BRACE <Variant> { COMMA <Variant> } [ COMMA ] R_BRACE */ \
    X(AST_ENUM_DECL, enum_decl, \
        AST_FIELD_INTERN(name) \
        AST_FIELD_DYNARRAY(generic_params) \
        AST_FIELD_DYNARRAY(variants) \
    ) \
    /* <Concept> ::= CONCEPT <Ident> [ <Generics> ] [ COLON <Bounds> ] L_BRACE { <ConceptItem> } R_BRACE */ \
    X(AST_CONCEPT_DECL, concept_decl, \
        AST_FIELD_INTERN(name) \
        AST_FIELD_DYNARRAY(generic_params) \
        AST_FIELD_DYNARRAY(bounds) \
        AST_FIELD_DYNARRAY(items) \
    ) \
    /* <Impl> ::= IMPL <Type> [ COLON <Bounds> ] [ <Where> ] L_BRACE { <ImplItem> } R_BRACE */ \
    X(AST_IMPL_DECL, impl_decl, \
        AST_FIELD_DYNARRAY(generic_params) \
        AST_FIELD_NODE(type) \
        AST_FIELD_DYNARRAY(bounds) \
        AST_FIELD_DYNARRAY(where_clauses) \
        AST_FIELD_DYNARRAY(items) \
    ) \
    /* <Test> ::= TEST <String> <Block> */ \
    X(AST_TEST_DECL, test_decl, \
        AST_FIELD_SLICE(name) \
        AST_FIELD_NODE(block) \
    ) \
    /* <Let> ::= [ MUT ] ( <Ident> ( COLON_EQ | COLON <Type> ASSIGN ) | <Pattern> COLON_EQ ) <Expr> SEMICOLON */ \
    X(AST_LET_STMT, let_stmt, \
        AST_FIELD_BOOL(is_mut) \
        AST_FIELD_NODE(pattern) \
        AST_FIELD_NODE(type) \
        AST_FIELD_NODE(expr) \
    ) \
    /* <Assign> ::= <Range> [ <AssignOp> <Assign> ] */ \
    X(AST_ASSIGN_STMT, assign_stmt, \
        AST_FIELD_NODE(lvalue) \
        AST_FIELD_NODE(rvalue) \
        AST_FIELD_OP(op) \
    ) \
    /* <ExprStmt> ::= <Expr> SEMICOLON | <BlockLike> | COMPTIME <BlockLike> */ \
    X(AST_EXPR_STMT, expr_stmt, \
        AST_FIELD_NODE(expr) \
        AST_FIELD_BOOL(is_comptime) \
    ) \
    /* <Defer> ::= ( DEFER ) ( <Expr> SEMICOLON | <Block> ) */ \
    X(AST_DEFER_STMT, defer_stmt, \
        AST_FIELD_NODE(expr) \
    ) \
    /* <Defer> ::= ( ERRDEFER ) ( <Expr> SEMICOLON | <Block> ) */ \
    X(AST_ERRDEFER_STMT, errdefer_stmt, \
        AST_FIELD_NODE(expr) \
    ) \
    /* <Return> ::= RETURN [ <Expr> ] SEMICOLON */ \
    X(AST_RETURN_STMT, return_stmt, \
        AST_FIELD_NODE(expr) \
    ) \
    /* <Break> ::= BREAK [ <Label> ] [ <Expr> ] SEMICOLON */ \
    X(AST_BREAK_STMT, break_stmt, \
        AST_FIELD_INTERN(label) \
        AST_FIELD_NODE(expr) \
    ) \
    /* <Continue> ::= CONTINUE [ <Label> ] SEMICOLON */ \
    X(AST_CONTINUE_STMT, continue_stmt, \
        AST_FIELD_INTERN(label) \
    ) \
    /* <Assert> ::= ASSERT <Expr> SEMICOLON */ \
    X(AST_ASSERT_STMT, assert_stmt, \
        AST_FIELD_NODE(expr) \
    ) \
    /* <Unsafe> ::= UNSAFE <Block> */ \
    X(AST_UNSAFE_BLOCK, unsafe_block, \
        AST_FIELD_NODE(block) \
    ) \
    /* <Block> ::= L_BRACE { <Stmt> } [ <Expr> ] R_BRACE */ \
    X(AST_BLOCK, block, \
        AST_FIELD_INTERN(label) \
        AST_FIELD_DYNARRAY(stmts) \
    ) \
    /* <Range> ::= <Coalesce> [ ( DOT_DOT | DOT_DOT_EQ ) <Coalesce> ] */ \
    X(AST_RANGE_EXPR, range_expr, \
        AST_FIELD_NODE(start) \
        AST_FIELD_NODE(end) \
        AST_FIELD_BOOL(inclusive) \
    ) \
    /* Binary expressions (Coalesce, Or, And, Cmp, BitOr, BitXor, BitAnd, Shift, Add, Mul) */ \
    X(AST_BINARY_EXPR, binary_expr, \
        AST_FIELD_NODE(left) \
        AST_FIELD_NODE(right) \
        AST_FIELD_OP(op) \
    ) \
    /* <Unary> ::= ( MINUS | BANG | AMP [ MUT ] | STAR | COMPTIME ) <Unary> | <Postfix> */ \
    X(AST_UNARY_EXPR, unary_expr, \
        AST_FIELD_NODE(expr) \
        AST_FIELD_OP(op) \
    ) \
    /* <Postfix> ::= <Primary> { L_SQB <Expr> R_SQB | DOT ( <Ident> | <Int> ) | DOT_L_SQB <Expr> R_SQB | QUESTION | BANG } */ \
    X(AST_POSTFIX_EXPR, postfix_expr, \
        AST_FIELD_NODE(expr) \
        AST_FIELD_OP(op) \
    ) \
    /* <Cast> ::= <Unary> { AS <Type> } */ \
    X(AST_CAST_EXPR, cast_expr, \
        AST_FIELD_NODE(expr) \
        AST_FIELD_NODE(type) \
    ) \
    /* <Postfix> ::= <Primary> { LPAREN [ <Args> ] RPAREN } */ \
    X(AST_CALL_EXPR, call_expr, \
        AST_FIELD_NODE(callee) \
        AST_FIELD_DYNARRAY(args) \
    ) \
    /* <Postfix> ::= <Primary> { L_SQB <Expr> R_SQB } */ \
    X(AST_INDEX_EXPR, index_expr, \
        AST_FIELD_NODE(target) \
        AST_FIELD_NODE(index) \
    ) \
    /* <Postfix> ::= <Primary> { DOT ( <Ident> | <Int> ) [ <GenericArgs> ] } */ \
    X(AST_MEMBER_EXPR, member_expr, \
        AST_FIELD_NODE(target) \
        AST_FIELD_INTERN(member) \
        AST_FIELD_DYNARRAY(generic_args) \
    ) \
    /* <Ident> ::= ( <Letter> | UNDERSCORE ) { <Letter> | <Digit> | UNDERSCORE } */ \
    X(AST_IDENTIFIER, identifier, \
        AST_FIELD_INTERN(name) \
        AST_FIELD_DYNARRAY(generic_args) \
    ) \
    /* <Path> ::= <Ident> { DOT <Ident> } */ \
    X(AST_PATH, path, \
        AST_FIELD_INTERN_DYNARRAY(segments) \
    ) \
    /* <StructLit> ::= <Path> [ <GenericArgs> ] L_BRACE [ <Ident> COLON <Expr> { COMMA <Ident> COLON <Expr> } [ COMMA ] ] R_BRACE */ \
    X(AST_STRUCT_LITERAL, struct_literal, \
        AST_FIELD_NODE(path) \
        AST_FIELD_DYNARRAY(fields) \
    ) \
    /* <Primary> ::= L_SQB [ <Expr> { COMMA <Expr> } ] R_SQB | LPAREN [ <Expr> { COMMA <Expr> } ] RPAREN */ \
    X(AST_ARRAY_TUPLE_LITERAL, array_tuple_literal, \
        AST_FIELD_DYNARRAY(elements) \
    ) \
    /* <Lambda> ::= ( PIPE [ <LParam> { COMMA <LParam> } ] PIPE | OR_OR ) <Expr> */ \
    X(AST_LAMBDA_EXPR, lambda_expr, \
        AST_FIELD_DYNARRAY(captures) \
        AST_FIELD_DYNARRAY(params) \
        AST_FIELD_NODE(expr) \
    ) \
    /* <If> ::= IF <Cond> <Block> [ ELSE ( <If> | <Block> ) ] */ \
    X(AST_IF_EXPR, if_expr, \
        AST_FIELD_NODE(cond) \
        AST_FIELD_NODE(then_block) \
        AST_FIELD_NODE(else_node) \
    ) \
    /* <While> ::= [ <LabelDef> ] WHILE <Cond> <Block> */ \
    X(AST_WHILE_EXPR, while_expr, \
        AST_FIELD_INTERN(label) \
        AST_FIELD_NODE(cond) \
        AST_FIELD_NODE(body) \
    ) \
    /* <For> ::= [ <LabelDef> ] FOR <Pattern> IN <Expr> <Block> */ \
    X(AST_FOR_EXPR, for_expr, \
        AST_FIELD_INTERN(label) \
        AST_FIELD_NODE(pattern) \
        AST_FIELD_NODE(iterable) \
        AST_FIELD_NODE(body) \
    ) \
    /* <Loop> ::= [ <LabelDef> ] LOOP <Block> */ \
    X(AST_LOOP_EXPR, loop_expr, \
        AST_FIELD_INTERN(label) \
        AST_FIELD_NODE(body) \
    ) \
    /* <Match> ::= MATCH <Expr> L_BRACE <Arm> { COMMA <Arm> } [ COMMA ] R_BRACE */ \
    X(AST_MATCH_EXPR, match_expr, \
        AST_FIELD_NODE(expr) \
        AST_FIELD_DYNARRAY(arms) \
    ) \
    /* <BlockLike> ::= SPAWN LPAREN <Expr> RPAREN */ \
    X(AST_SPAWN_EXPR, spawn_expr, \
        AST_FIELD_NODE(expr) \
    ) \
    /* <Type> ::= <TypeAtom> { <TypeSuffix> } */ \
    X(AST_TYPE_NODE, type_node, \
        AST_FIELD_KIND(kind) \
        AST_FIELD_BOOL(is_mut) \
        AST_FIELD_NODE(target) \
    ) \
    /* <Param> ::= <SelfParam> | <Ident> COLON <Type> [ ASSIGN <Expr> ] */ \
    X(AST_PARAM, param, \
        AST_FIELD_INTERN(name) \
        AST_FIELD_NODE(type) \
        AST_FIELD_NODE(default_val) \
    ) \
    /* <Field> ::= <Ident> COLON <Type> [ ASSIGN <Expr> ] */ \
    X(AST_FIELD, field, \
        AST_FIELD_INTERN(name) \
        AST_FIELD_NODE(type) \
        AST_FIELD_NODE(default_val) \
    ) \
    /* <Arm> ::= <Pattern> { PIPE <Pattern> } [ IF <Expr> ] FAT_ARROW <Expr> */ \
    X(AST_MATCH_ARM, match_arm, \
        AST_FIELD_DYNARRAY(patterns) \
        AST_FIELD_NODE(guard) \
        AST_FIELD_NODE(expr) \
    ) \
    /* <Literal> ::= <Int> | <Float> | <String> | <CString> | <Rune> | <Byte> | TRUE | FALSE | NULL */ \
    X(AST_LITERAL, literal, \
        AST_FIELD_TOKEN(kind) \
        AST_FIELD_LITERAL_VALUE(value) \
    )

/* Pass 1: Generate Enum */ \
#define AST_GENERATE_ENUM(enum_val, name, fields) enum_val,
typedef enum {
    AST_NODES(AST_GENERATE_ENUM)
} AstNodeType;

/* Pass 2: Generate Structs */ \
#define AST_FIELD_NODE(name) AstNode *name;
#define AST_FIELD_INTERN(name) InternResult *name;
#define AST_FIELD_DYNARRAY(name) DynArray *name;
#define AST_FIELD_INTERN_DYNARRAY(name) DynArray *name;
#define AST_FIELD_BOOL(name) bool name;
#define AST_FIELD_OP(name) OpKind name;
#define AST_FIELD_SLICE(name) Slice name;
#define AST_FIELD_KIND(name) AstTypeKind name;
#define AST_FIELD_TOKEN(name) TokenKind name;
#define AST_FIELD_LITERAL_VALUE(name) LiteralValue name;

#define AST_GENERATE_STRUCT(enum_val, name, fields) \
    typedef struct { fields } Ast_##name;

AST_NODES(AST_GENERATE_STRUCT)

/* Pass 3: Generate Union */ \
#define AST_GENERATE_UNION(enum_val, name, fields) Ast_##name name;
struct AstNode {
    AstNodeType node_type;
    bool is_pub;            // set by parse_item / parse_impl_item
    Span span;
    union {
        AST_NODES(AST_GENERATE_UNION)
    } data;
};

AstNode *ast_create_node(AstNodeType type, Arena *arena);