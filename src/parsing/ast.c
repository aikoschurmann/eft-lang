#include "ast.h"
#include <stdio.h>
#include <string.h>

const char *ast_node_type_name(AstNodeType t);
const char *ast_op_name(OpKind op);

/* ------------------------------------------------------------------ */
/* Node allocation                                                      */
/* ------------------------------------------------------------------ */

AstNode *ast_create_node(AstNodeType type, Arena *arena) {
    AstNode *node = arena_calloc(arena, sizeof(AstNode));
    if (node) {
        node->node_type = type;
    }
    return node;
}
