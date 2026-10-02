#pragma once

#include "token.h"
#include "parser.h"

typedef struct {
    int left;
    int right;
} BindingPower;

BindingPower get_infix_binding_power(TokenKind kind);
AstNode *parse_expression(Parser *p, int min_bp);
AstNode *parse_assign_or_expr(Parser *p);