#include "framework.h"
#include "ast.h"
#include "parser.h"

static void test_ast_printer(void) {
    Arena *arena = arena_create(1024 * 1024);
    
    AstNode *root = ast_create_node(AST_PROGRAM, arena);
    DynArray *arr = arena_alloc(arena, sizeof(DynArray));
    dynarray_init_in_arena(arr, arena, sizeof(AstNode*), 2);
    root->data.program.program_items = arr;
    
    AstNode *var_decl = ast_create_node(AST_LET_STMT, arena);
    var_decl->data.let_stmt.is_mut = true;
    
    AstNode *ident = ast_create_node(AST_IDENTIFIER, arena);
    // Fake the intern result for demo
    InternResult dummy_intern = { .key = "my_variable" };
    ident->data.identifier.name = &dummy_intern;
    ident->span = (Span){1, 5, 1, 10};
    var_decl->data.let_stmt.pattern = ident;
    
    AstNode *value = ast_create_node(AST_LITERAL, arena);
    value->data.literal.kind = TK_INT_LIT;
    value->data.literal.value.int_val = 42;
    var_decl->data.let_stmt.expr = value;
    
    dynarray_push_ptr(root->data.program.program_items, var_decl);
    
    // Print it
    printf("\n--- AST X-Macro Dump Test ---\n");
    ast_print(root, 0);
    printf("-----------------------------\n");
    
    TEST_ASSERT(true);
}


static void test_ast_complex(void) {
    Arena *arena = arena_create(1024 * 1024);
    
    // Call Expression: execute(|x| x + 1)
    AstNode *call = ast_create_node(AST_CALL_EXPR, arena);
    call->span = (Span){1, 1, 1, 20};
    
    // Callee
    AstNode *callee = ast_create_node(AST_IDENTIFIER, arena);
    InternResult dummy_execute = { .key = "execute" };
    callee->data.identifier.name = &dummy_execute;
    call->data.call_expr.callee = callee;
    
    // Args
    DynArray *args = arena_alloc(arena, sizeof(DynArray));
    dynarray_init_in_arena(args, arena, sizeof(AstNode*), 1);
    call->data.call_expr.args = args;
    
    // Lambda Expr
    AstNode *lambda = ast_create_node(AST_LAMBDA_EXPR, arena);
    lambda->span = (Span){1, 9, 1, 19};
    
    // Lambda Params
    DynArray *l_params = arena_alloc(arena, sizeof(DynArray));
    dynarray_init_in_arena(l_params, arena, sizeof(AstNode*), 1);
    lambda->data.lambda_expr.params = l_params;
    
    AstNode *param_x = ast_create_node(AST_PARAM, arena);
    InternResult dummy_x = { .key = "x" };
    param_x->data.param.name = &dummy_x;
    dynarray_push_ptr(l_params, param_x);
    
    // Lambda Body (Binary Expr: x + 1)
    AstNode *bin = ast_create_node(AST_BINARY_EXPR, arena);
    bin->data.binary_expr.op = OP_ADD;
    
    AstNode *bin_left = ast_create_node(AST_IDENTIFIER, arena);
    bin_left->data.identifier.name = &dummy_x;
    bin->data.binary_expr.left = bin_left;
    
    AstNode *bin_right = ast_create_node(AST_LITERAL, arena);
    bin_right->data.literal.kind = TK_INT_LIT;
    bin_right->data.literal.value.int_val = 1;
    bin->data.binary_expr.right = bin_right;
    
    lambda->data.lambda_expr.expr = bin;
    
    dynarray_push_ptr(args, lambda);
    
    printf("\n--- Complex AST Lambda Test ---\n");
    ast_print(call, 0);
    printf("-------------------------------\n");
}

void test_ast(void) {
    RUN_TEST(test_ast_complex);
    }
