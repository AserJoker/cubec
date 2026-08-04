#include "cubec/expression_binary.h"
#include "core/allocator.h"
#include "core/emit_context.h"
#include "core/string.h"
#include "core/token.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void
_cubec_expression_binary_init(cubec_expression_binary_t self,
                              allocator_t allocator,
                              cubec_expression_binary_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_BINARY,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->left = init->left;
  self->right = init->right;
  self->opt = init->opt;
}

static void _cubec_expression_binary_dispose(cubec_expression_binary_t self,
                                             allocator_t allocator) {
  allocator_free(allocator, &self->left);
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->opt);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_binary_clone(cubec_expression_binary_t self,
                                           allocator_t allocator,
                                           cubec_expression_binary_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->left = value_clone(allocator, another->left);
  self->right = value_clone(allocator, another->right);
  self->opt = (string_t)value_clone(allocator, another->opt);
  return;

cleanup:
  allocator_free(allocator, &self->opt);
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->left);
}

static void _cubec_expression_binary_move(cubec_expression_binary_t self,
                                          allocator_t allocator,
                                          cubec_expression_binary_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->left = value_move(allocator, another->left);
  self->right = value_move(allocator, another->right);
  self->opt = (string_t)value_move(allocator, another->opt);
  return;

cleanup:
  allocator_free(allocator, &self->opt);
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->left);
}

type_t g_cubec_expression_binary_type = {
    .name = "cubec.cubec.expression_binary",
    .size = sizeof(struct _cubec_expression_binary_t),
    .init = (type_init_fn_t)_cubec_expression_binary_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_binary_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_binary_clone,
    .move = (type_move_fn_t)_cubec_expression_binary_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_prefix
 * -------------------------------------------------------------------------- */

/** Set of supported prefix operator strings. */
static const char *prefix_operators[] = {
    "!",
    "+",
    "-",
    "~",
};

static bool is_prefix_operator_token(token_t tok) {
  if (token_get_kind(tok) != CUBEC_TOKEN_SYMBOL) {
    return false;
  }
  for (size_t i = 0; i < sizeof(prefix_operators) / sizeof(prefix_operators[0]);
       i++) {
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, prefix_operators[i])) {
      return true;
    }
  }
  return false;
}

/* Forward declaration: parse unary (prefix + value), used by prefix parser */
static node_t read_unary(context_t ctx, vec_t tokens, size_t *position,
                         const char *filename);

node_t read_expression_prefix(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_expression_binary_t node = NULL;
  string_t opt = NULL;
  node_t right = NULL;

  token_t op_token = vec_get(tokens, current);
  if (!op_token) {
    return NULL;
  }

  /* Check for prefix operator */
  if (!is_prefix_operator_token(op_token)) {
    return NULL;
  }
  location_t start_location = *token_get_location(op_token);
  start_location.filename = filename;
  current++;

  /* Build operator string */
  const char *op_text = token_get_string(op_token);
  size_t op_len = token_get_string_length(op_token);
  opt = allocator_create(allocator, &g_string_type, NULL);
  string_nconcat(opt, op_text, op_len);

  /* Parse operand via read_unary (prefix unary binds tighter than binary) */
  right = read_unary(ctx, tokens, &current, filename);
  if (node_is_error(right)) {
    allocator_free(allocator, &opt);
    return right;
  }
  if (!right) {
    goto onerror;
  }

  node = allocator_create(allocator, &g_cubec_expression_binary_type,
                          &(cubec_expression_binary_init_t){
                              .left = NULL,
                              .right = right,
                              .opt = opt,
                          });
  location_t *loc = token_get_location(op_token);
  node->super.super.location = *loc;
  node->super.super.location.filename = filename;

  *position = current;
  return (node_t)node;

onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                       "invalid prefix expression");
  allocator_free(allocator, &opt);
  allocator_free(allocator, &right);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Binary expression: precedence climbing
 * -------------------------------------------------------------------------- */

/**
 * @brief Binary operator with its precedence level and string representation.
 *        Lower precedence value = binds looser.
 */
typedef struct {
  const char *text; /**< Operator string (e.g. "*", "+", "<=") */
  int precedence;   /**< Precedence level (1=loosest for ||, 10=tightest for * /
                       %) */
} binary_op_entry_t;

/** Sorted by descending precedence (tightest first) for readability.
 *  Note: "extends" is a keyword binary operator (same precedence as ==, !=)
 *  handled separately in get_binary_precedence(). */
static const binary_op_entry_t binary_operators[] = {
    {"*", 10}, {"/", 10}, {"%", 10}, {"+", 9},  {"-", 9},  {"<<", 8},
    {">>", 8}, {"<", 7},  {">", 7},  {"<=", 7}, {">=", 7}, {"==", 6},
    {"!=", 6}, {"&", 5},  {"^", 4},  {"|", 3},  {"&&", 2}, {"||", 1},
};

/**
 * @brief Get precedence of a token if it's a binary operator, or 0.
 *        "extends" is a keyword binary operator at the same level as ==, !=.
 */
static int get_binary_precedence(token_t tok) {
  /* Check for keyword binary operators */
  if (token_get_kind(tok) == CUBEC_TOKEN_KEYWORD) {
    if (token_is(tok, CUBEC_TOKEN_KEYWORD, "extends") ||
        token_is(tok, CUBEC_TOKEN_KEYWORD, "is")) {
      return 6; /* Same precedence as ==, != */
    }
    return 0;
  }
  if (token_get_kind(tok) != CUBEC_TOKEN_SYMBOL) {
    return 0;
  }
  for (size_t i = 0; i < sizeof(binary_operators) / sizeof(binary_operators[0]);
       i++) {
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, binary_operators[i].text)) {
      return binary_operators[i].precedence;
    }
  }
  return 0;
}

/**
 * @brief Make a binary-expression AST node.
 */
static cubec_expression_binary_t make_binary_node(context_t ctx, node_t left,
                                                  node_t right, string_t opt,
                                                  token_t op_token,
                                                  const char *filename) {
  allocator_t allocator = ctx->allocator;
  cubec_expression_binary_t node =
      allocator_create(allocator, &g_cubec_expression_binary_type,
                       &(cubec_expression_binary_init_t){
                           .left = left,
                           .right = right,
                           .opt = opt,
                       });
  location_t *loc = token_get_location(op_token);
  node->super.super.location = *loc;
  node->super.super.location.filename = filename;
  return node;
}

/**
 * @brief Parse unary operand: tries prefix unary first, then value (atom +
 *        postfix). Does NOT handle binary infix operators.
 */
static node_t read_unary(context_t ctx, vec_t tokens, size_t *position,
                         const char *filename) {
  node_t node = read_expression_prefix(ctx, tokens, position, filename);
  if (node) {
    return node;
  }
  /* Fall back to value (atom + postfix .member) */
  return read_value(ctx, tokens, position, filename);
}

/**
 * @brief Precedence-climbing RHS for binary infix operators.
 *        Continues parsing right-hand operands while the next token is a
 *        binary operator whose precedence >= @p min_precedence.
 */
static node_t read_binary_rhs(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename, node_t left,
                              int min_precedence) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;

  while (true) {
    skip_whitespace(tokens, &current);
    /* Update position so caller gets correct offset when we break.
     * After skipping whitespace, current points to the next meaningful token.
     */
    *position = current;

    token_t op_token = vec_get(tokens, current);
    if (!op_token) {
      break;
    }

    int prec = get_binary_precedence(op_token);
    if (prec < min_precedence) {
      break;
    }
    current++; /* consume operator */

    /* Build operator string */
    const char *op_text = token_get_string(op_token);
    size_t op_len = token_get_string_length(op_token);
    string_t opt = allocator_create(allocator, &g_string_type, NULL);
    string_nconcat(opt, op_text, op_len);

    /* Parse right operand via read_unary (no binary yet) */
    skip_whitespace(tokens, &current);
    node_t right = read_unary(ctx, tokens, &current, filename);
    if (node_is_error(right)) {
      allocator_free(allocator, &opt);
      return right;
    }
    if (!right) {
      allocator_free(allocator, &opt);
      break;
    }

    /* Look ahead: higher-precedence operators bind to right first
     * (left-associative: use prec + 1 so equal-precedence on right
     *  won't steal the operand; right-associative would use prec). */
    skip_whitespace(tokens, &current);
    right = read_binary_rhs(ctx, tokens, &current, filename, right, prec + 1);

    left = (node_t)make_binary_node(ctx, left, right, opt, op_token, filename);
    *position = current;
  }

  return left;
}

node_t read_expression_binary(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename) {
  node_t left = read_unary(ctx, tokens, position, filename);
  if (node_is_error(left))
    return left;
  if (!left) {
    return NULL;
  }

  /* Continue with binary infix operators */
  return read_binary_rhs(ctx, tokens, position, filename, left,
                         1 /* lowest precedence */);
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_binary
 * -------------------------------------------------------------------------- */

node_t create_expression_binary(context_t ctx, location_t loc, const char *op,
                                node_t left, node_t right) {
  allocator_t alloc = ctx->allocator;
  string_t op_str =
      allocator_create(alloc, &g_string_type, &(string_init_t){.str = op});
  cubec_expression_binary_init_t init = {.location = loc,
                                         .parent = NULL,
                                         .left = left,
                                         .right = right,
                                         .opt = op_str};
  return (node_t)allocator_create(alloc, &g_cubec_expression_binary_type,
                                  &init);
}

void write_expression_binary(writer_t writer, node_t node) {
  cubec_expression_binary_t expr = (cubec_expression_binary_t)node;
  if (expr->left) {
    write_expression(writer, expr->left);
    writer_append(writer, " ");
  }
  writer_append(writer, string_get(expr->opt));
  writer_append(writer, " ");
  write_expression(writer, expr->right);
}

void emit_expression_binary(emit_context_t ctx, node_t node) {
  cubec_expression_binary_t expr = (cubec_expression_binary_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  if (expr->left) {
    emit_expression(ctx, expr->left);
    emit_space(ctx);
  }
  /* Operator string: emit each character via emit_symbol for token tracking */
  {
    const char *s = string_get(expr->opt);
    /* Most binary operators are 1-2 chars; use emit_symbol for each */
    emit_symbol(ctx, s);
  }
  emit_space(ctx);
  emit_expression(ctx, expr->right);
}