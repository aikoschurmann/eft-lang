#pragma once

#include "ast.h"

const char *ast_node_type_name(AstNodeType t);
const char *ast_op_name(OpKind op);
void ast_print(AstNode *node, int indent);