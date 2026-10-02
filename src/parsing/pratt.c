#include "pratt.h"
#include "ast.h"

/*
 * Expression parser (Pratt).
 *
 * Each infix operator has a precedence level. Its binding powers are
 *     left  = 2 * level
 *     right = left + 1        (left-associative)
 * parse_expression keeps extending the left node while the next operator's
 * left power is >= min_bp.
 *
 * Assignment is not part of expressions. Use parse_assign_or_expr() where a
 * statement is expected.
 */

typedef enum {
    PREC_NONE = 0,
    PREC_RANGE,
    PREC_COALESCE,
    PREC_OR,
    PREC_AND,
    PREC_CMP,
    PREC_BIT_OR,
    PREC_BIT_XOR,
    PREC_BIT_AND,
    PREC_SHIFT,
    PREC_ADD,
    PREC_MUL,
    PREC_CAST,
    PREC_UNARY,
    PREC_POSTFIX,
} Precedence;

#define BP_LEFT(prec)       ((int)(prec) * 2)
#define BP_ANY              0
#define BP_UNARY_OPERAND    BP_LEFT(PREC_UNARY)   /* lets postfix bind, stops at 'as' */

typedef enum {
    INFIX_NONE,
    INFIX_BINARY,
    INFIX_RANGE,
    INFIX_CAST,
    INFIX_CALL,
    INFIX_INDEX,
    INFIX_MEMBER,
    INFIX_POSTFIX,
} InfixKind;

typedef struct {
    Precedence prec;
    InfixKind  kind;
    OpKind     op;
} InfixRule;

#define BINARY(tok, level, opkind)  [tok] = { level, INFIX_BINARY, opkind }

static const InfixRule INFIX_RULES[TK_COUNT] = {
    BINARY(TK_COALESCE,    PREC_COALESCE, OP_COALESCE),
    BINARY(TK_OR_OR,       PREC_OR,       OP_OR),
    BINARY(TK_AND_AND,     PREC_AND,      OP_AND),

    BINARY(TK_EQ_EQ,       PREC_CMP,      OP_EQ),
    BINARY(TK_BANG_EQ,     PREC_CMP,      OP_NEQ),
    BINARY(TK_LESS,        PREC_CMP,      OP_LT),
    BINARY(TK_LESS_EQ,     PREC_CMP,      OP_LTE),
    BINARY(TK_GREATER,     PREC_CMP,      OP_GT),
    BINARY(TK_GREATER_EQ,  PREC_CMP,      OP_GTE),

    BINARY(TK_PIPE,        PREC_BIT_OR,   OP_BIT_OR),
    BINARY(TK_CARET,       PREC_BIT_XOR,  OP_BIT_XOR),
    BINARY(TK_AMPERSAND,   PREC_BIT_AND,  OP_BIT_AND),
    BINARY(TK_SHL,         PREC_SHIFT,    OP_SHL),
    BINARY(TK_SHR,         PREC_SHIFT,    OP_SHR),

    BINARY(TK_PLUS,        PREC_ADD,      OP_ADD),
    BINARY(TK_MINUS,       PREC_ADD,      OP_SUB),
    BINARY(TK_STAR,        PREC_MUL,      OP_MUL),
    BINARY(TK_SLASH,       PREC_MUL,      OP_DIV),
    BINARY(TK_PERCENT,     PREC_MUL,      OP_MOD),

    [TK_DOT_DOT]    = { PREC_RANGE,   INFIX_RANGE,   OP_NONE },
    [TK_DOT_DOT_EQ] = { PREC_RANGE,   INFIX_RANGE,   OP_NONE },
    [TK_AS]         = { PREC_CAST,    INFIX_CAST,    OP_NONE },

    [TK_LPAREN]     = { PREC_POSTFIX, INFIX_CALL,    OP_NONE },
    [TK_LBRACKET]   = { PREC_POSTFIX, INFIX_INDEX,   OP_NONE },
    [TK_DOT]        = { PREC_POSTFIX, INFIX_MEMBER,  OP_NONE },
    [TK_QUESTION]   = { PREC_POSTFIX, INFIX_POSTFIX, OP_PROPAGATE },
    [TK_BANG]       = { PREC_POSTFIX, INFIX_POSTFIX, OP_FORCE_UNWRAP },
};

BindingPower get_infix_binding_power(TokenKind kind) {
    int left = (kind < TK_COUNT) ? BP_LEFT(INFIX_RULES[kind].prec) : 0;
    return (BindingPower){ left, left + 1 };
}

static OpKind assign_op(TokenKind kind) {
    switch (kind) {
        case TK_EQ:           return OP_ASSIGN;
        case TK_PLUS_EQ:      return OP_ADD_ASSIGN;
        case TK_MINUS_EQ:     return OP_SUB_ASSIGN;
        case TK_STAR_EQ:      return OP_MUL_ASSIGN;
        case TK_SLASH_EQ:     return OP_DIV_ASSIGN;
        case TK_PERCENT_EQ:   return OP_MOD_ASSIGN;
        case TK_AMPERSAND_EQ: return OP_BIT_AND_ASSIGN;
        case TK_PIPE_EQ:      return OP_BIT_OR_ASSIGN;
        case TK_CARET_EQ:     return OP_BIT_XOR_ASSIGN;
        case TK_SHL_EQ:       return OP_SHL_ASSIGN;
        case TK_SHR_EQ:       return OP_SHR_ASSIGN;
        default:              return OP_NONE;
    }
}

#define CASE_LITERAL \
    case TK_INT_LIT: case TK_FLOAT_LIT: case TK_STR_LIT: case TK_BYTE_STR_LIT: \
    case TK_C_STR_LIT: case TK_RUNE_LIT: case TK_BYTE_LIT: \
    case TK_TRUE: case TK_FALSE: case TK_NULL

#define CASE_BLOCK_LIKE \
    case TK_IF: case TK_MATCH: case TK_WHILE: case TK_FOR: case TK_LOOP: case TK_SPAWN

/* True if the token can begin an expression. Used for optional operands. */
static bool can_start_expr(const Token *t) {
    if (!t) return false;
    switch (t->type) {
        CASE_LITERAL:
        CASE_BLOCK_LIKE:
        case TK_IDENT:
        case TK_LPAREN:
        case TK_MINUS: case TK_BANG: case TK_STAR: case TK_AMPERSAND: case TK_COMPTIME:
            return true;
        default:
            return false;
    }
}

static void reject_range_chain(Parser *p) {
    Token *t = current_token(p);
    if (t && (t->type == TK_DOT_DOT || t->type == TK_DOT_DOT_EQ))
        parser_throw(p, "Ranges cannot be chained", t);
}

/* ---------------------------------------------------------
 * Prefix forms
 * --------------------------------------------------------- */

/* '(' already consumed: unit, grouping, or tuple. */
static AstNode *parse_paren(Parser *p, Token *open) {
    DynArray *elems = new_dynarray(p, sizeof(AstNode *), 4);

    if (!parser_match(p, TK_RPAREN)) {
        AstNode *first = parse_expression(p, BP_ANY);

        if (!parser_match(p, TK_COMMA)) {
            expect(p, TK_RPAREN, "Expected ')' after expression");
            return first;   /* plain grouping: no node of its own */
        }

        dynarray_push_ptr(elems, first);
        while (!parser_match(p, TK_RPAREN)) {
            dynarray_push_ptr(elems, parse_expression(p, BP_ANY));
            if (!parser_match(p, TK_COMMA)) {
                expect(p, TK_RPAREN, "Expected ',' or ')' in tuple");
                break;
            }
        }
    }

    AstNode *node = new_node_spanned(p, AST_ARRAY_TUPLE_LITERAL, open);
    node->data.array_tuple_literal.elements = elems;
    return node;
}

static AstNode *parse_unary(Parser *p, Token *tok) {
    OpKind op;
    switch (tok->type) {
        case TK_MINUS:     op = OP_NEG;     break;
        case TK_BANG:      op = OP_NOT;     break;
        case TK_STAR:      op = OP_DEREF;   break;
        case TK_COMPTIME:  op = OP_COMPTIME; break;
        case TK_AMPERSAND: op = OP_REF; break;
        default:
            parser_throw(p, "Internal error: not a unary operator", tok);
            return NULL;
    }

    AstNode *operand = parse_expression(p, BP_UNARY_OPERAND);
    AstNode *node = new_node_spanned(p, AST_UNARY_EXPR, tok);
    node->data.unary_expr.op = op;
    node->data.unary_expr.expr = operand;
    return node;
}

/* Prefix range: `..end`. The '..' token is already consumed. */
static AstNode *parse_prefix_range(Parser *p, Token *tok) {
    AstNode *end = NULL;
    if (can_start_expr(current_token(p)))
        end = parse_expression(p, BP_LEFT(PREC_COALESCE));

    AstNode *node = new_node_spanned(p, AST_RANGE_EXPR, tok);
    node->data.range_expr.end = end;
    reject_range_chain(p);
    return node;
}

static AstNode *parse_prefix(Parser *p, Token *tok) {
    switch (tok->type) {
        CASE_LITERAL: {
            AstNode *node = new_node(p, AST_LITERAL);
            node->data.literal.kind = tok->type;
            /* TODO: decode the value from tok->slice (numbers, escapes). */
            set_span(node, tok, tok);
            return node;
        }

        case TK_IDENT: {
            AstNode *node = new_node(p, AST_IDENTIFIER);
            node->data.identifier.name = tok->record;
            set_span(node, tok, tok);
            return node;
        }

        case TK_LPAREN:
            return parse_paren(p, tok);

        case TK_DOT_DOT:
            return parse_prefix_range(p, tok);

        case TK_MINUS: case TK_BANG: case TK_STAR: case TK_AMPERSAND: case TK_COMPTIME:
            return parse_unary(p, tok);

        CASE_BLOCK_LIKE:
            parser_throw(p, "Block-like expressions are not implemented yet", tok);
            return NULL;

        default:
            parser_throw(p, "Unexpected token at start of expression", tok);
            return NULL;
    }
}

/* ---------------------------------------------------------
 * Infix and postfix forms
 * --------------------------------------------------------- */

static AstNode *parse_binary(Parser *p, AstNode *left, OpKind op, int right_bp) {
    AstNode *right = parse_expression(p, right_bp);
    AstNode *node = new_node(p, AST_BINARY_EXPR);
    node->data.binary_expr.op = op;
    node->data.binary_expr.left = left;
    node->data.binary_expr.right = right;
    set_node_span(node, left, previous_token(p));
    return node;
}

static AstNode *parse_range_tail(Parser *p, AstNode *left, Token *op_tok, int right_bp) {
    bool inclusive = (op_tok->type == TK_DOT_DOT_EQ);
    AstNode *end = NULL;

    if (can_start_expr(current_token(p)))
        end = parse_expression(p, right_bp);
    else if (inclusive)
        parser_throw(p, "'..=' requires an end bound", op_tok);

    AstNode *node = new_node(p, AST_RANGE_EXPR);
    node->data.range_expr.start = left;
    node->data.range_expr.end = end;
    node->data.range_expr.inclusive = inclusive;
    set_node_span(node, left, previous_token(p));
    reject_range_chain(p);
    return node;
}

/* Call argument: `expr` or `name: expr`. */
static AstNode *parse_arg(Parser *p) {
    Token *first = current_token(p);
    Token *next  = peek(p, 1);
    Token *name  = NULL;

    if (first && first->type == TK_IDENT && next && next->type == TK_COLON) {
        name = parser_advance(p);
        parser_advance(p);
    }

    AstNode *expr = parse_expression(p, BP_ANY);
    AstNode *arg = new_node(p, AST_ARG);
    arg->data.arg.name = name ? name->record : NULL;
    arg->data.arg.expr = expr;

    if (name) set_span(arg, name, previous_token(p));
    else      arg->span = expr->span;
    return arg;
}

/* '(' already consumed. */
static AstNode *parse_call(Parser *p, AstNode *callee) {
    DynArray *args = new_dynarray(p, sizeof(AstNode *), 4);

    while (!parser_match(p, TK_RPAREN)) {
        dynarray_push_ptr(args, parse_arg(p));
        if (!parser_match(p, TK_COMMA)) {
            expect(p, TK_RPAREN, "Expected ',' or ')' after call argument");
            break;
        }
    }

    AstNode *node = new_node(p, AST_CALL_EXPR);
    node->data.call_expr.callee = callee;
    node->data.call_expr.args = args;
    set_node_span(node, callee, previous_token(p));
    return node;
}

/* '[' already consumed. */
static AstNode *parse_index(Parser *p, AstNode *target) {
    AstNode *index = parse_expression(p, BP_ANY);
    expect(p, TK_RBRACKET, "Expected ']' after index expression");

    AstNode *node = new_node(p, AST_INDEX_EXPR);
    node->data.index_expr.target = target;
    node->data.index_expr.index = index;
    set_node_span(node, target, previous_token(p));
    return node;
}

/* '.' already consumed: `.name` or `.[comptime_expr]`. */
static AstNode *parse_member(Parser *p, AstNode *target) {
    if (parser_match(p, TK_LBRACKET)) {
        /* NOTE: shares AST_INDEX_EXPR with `a[i]`, so sema cannot tell them
         * apart. Give it a dedicated node kind. */
        return parse_index(p, target);
    }

    Token *name = expect(p, TK_IDENT, "Expected a field or method name after '.'");
    AstNode *node = new_node(p, AST_MEMBER_EXPR);
    node->data.member_expr.target = target;
    node->data.member_expr.member = name->record;
    set_node_span(node, target, name);
    return node;
}

static AstNode *parse_postfix_op(Parser *p, AstNode *operand, Token *tok, OpKind op) {
    AstNode *node = new_node(p, AST_POSTFIX_EXPR);
    node->data.postfix_expr.expr = operand;
    node->data.postfix_expr.op = op;
    set_node_span(node, operand, tok);
    return node;
}

/* The operator token `tok` has already been consumed. */
AstNode *parse_infix(Parser *p, AstNode *left, Token *tok, int right_bp) {
    const InfixRule *rule = &INFIX_RULES[tok->type];

    switch (rule->kind) {
        case INFIX_BINARY:  return parse_binary(p, left, rule->op, right_bp);
        case INFIX_RANGE:   return parse_range_tail(p, left, tok, right_bp);
        case INFIX_CALL:    return parse_call(p, left);
        case INFIX_INDEX:   return parse_index(p, left);
        case INFIX_MEMBER:  return parse_member(p, left);
        case INFIX_POSTFIX: return parse_postfix_op(p, left, tok, rule->op);
        case INFIX_CAST:
            /* TODO: left as cast(left, parse_type(p)) once parse_type exists. */
            parser_throw(p, "'as' is not implemented yet (needs type parsing)", tok);
            return NULL;
        case INFIX_NONE:
            break;
    }

    parser_throw(p, "Internal error: token is not an infix operator", tok);
    return NULL;
}

/* ---------------------------------------------------------
 * Entry points
 * --------------------------------------------------------- */

AstNode *parse_expression(Parser *p, int min_bp) {
    Token *tok = current_token(p);
    if (!tok || tok->type == TK_EOF)
        parser_throw(p, "Unexpected end of input in expression", previous_token(p));

    parser_advance(p);
    AstNode *left = parse_prefix(p, tok);

    for (;;) {
        Token *next = current_token(p);
        if (!next) break;

        BindingPower bp = get_infix_binding_power(next->type);
        if (bp.left == 0 || bp.left < min_bp) break;

        parser_advance(p);
        left = parse_infix(p, left, next, bp.right);
    }

    return left;
}

/* Statement position: an expression, optionally followed by an assignment
 * operator and a right-hand side. The trailing ';' is the caller's job. */
AstNode *parse_assign_or_expr(Parser *p) {
    AstNode *lhs = parse_expression(p, BP_ANY);

    Token *op_tok = current_token(p);
    OpKind op = op_tok ? assign_op(op_tok->type) : OP_NONE;
    if (op == OP_NONE) return lhs;

    parser_advance(p);
    AstNode *rhs = parse_expression(p, BP_ANY);

    AstNode *node = new_node(p, AST_ASSIGN_STMT);
    node->data.assign_stmt.op = op;
    node->data.assign_stmt.lvalue = lhs;
    node->data.assign_stmt.rvalue = rhs;
    set_node_span(node, lhs, previous_token(p));
    return node;
}