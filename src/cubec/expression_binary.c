#include "cubec/expression_binary.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/string.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_binary_init(cubec_expression_binary_t self,
                                          allocator_t allocator,
                                          cubec_expression_binary_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
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
onerror:
  return;
}

static void _cubec_expression_binary_dispose(cubec_expression_binary_t self,
                                             allocator_t allocator) {
  allocator_free(allocator, &self->left);
  self->left = NULL;
  allocator_free(allocator, &self->right);
  self->right = NULL;
  allocator_free(allocator, &self->opt);
  self->opt = NULL;
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_binary_clone(cubec_expression_binary_t self,
                                           allocator_t allocator,
                                           cubec_expression_binary_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->left = value_clone(allocator, another->left);
  self->right = value_clone(allocator, another->right);
  self->opt = (string_t)value_clone(allocator, another->opt);
}

static void _cubec_expression_binary_move(cubec_expression_binary_t self,
                                          allocator_t allocator,
                                          cubec_expression_binary_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->left = value_move(allocator, another->left);
  self->right = value_move(allocator, another->right);
  self->opt = (string_t)value_move(allocator, another->opt);
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
    "!", "+", "-", "~",
};

static bool is_prefix_operator_token(token_t tok) {
  if (token_get_kind(tok) != CUBEC_TOKEN_SYMBOL) {
    return false;
  }
  for (size_t i = 0;
       i < sizeof(prefix_operators) / sizeof(prefix_operators[0]); i++) {
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, prefix_operators[i])) {
      return true;
    }
  }
  return false;
}

/* Forward declaration: parse unary (prefix + value), used by prefix parser */
static node_t read_unary(allocator_t allocator, vec_t tokens,
                         size_t *position, const char *filename);

node_t read_expression_prefix(allocator_t allocator, vec_t tokens,
                              size_t *position, const char *filename) {
  size_t current = *position;
  cubec_expression_binary_t node = NULL;
  string_t opt = NULL;
  node_t right = NULL;

  token_t op_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!op_token) {
    return NULL;
  }

  /* Check for prefix operator */
  if (!is_prefix_operator_token(op_token)) {
    return NULL;
  }
  current++;

  /* Build operator string */
  const char *op_text = token_get_string(op_token);
  size_t op_len = token_get_string_length(op_token);
  opt = TRY_LOCAL(onerror, allocator_create(allocator, &g_string_type, NULL));
  string_nconcat(opt, op_text, op_len);

  /* Parse operand via read_unary (prefix unary binds tighter than binary) */
  right = TRY_LOCAL(onerror,
                    read_unary(allocator, tokens, &current, filename));
  if (!right) {
    goto onerror;
  }

  node = TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_expression_binary_type,
                          &(cubec_expression_binary_init_t){
                              .left = NULL,
                              .right = right,
                              .opt = opt,
                          }));
  location_t *loc = token_get_location(op_token);
  node->super.super.location = *loc;
  node->super.super.location.filename = filename;

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &opt);
  allocator_free(allocator, &right);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Binary expression: precedence climbing
 * -------------------------------------------------------------------------- */

/**
 * @brief Binary operator with its precedence level and string representation.
 *        Lower precedence value = binds looser.
 */
typedef struct {
  const char *text;    /**< Operator string (e.g. "*", "+", "<=") */
  int precedence;      /**< Precedence level (1=loosest for ||, 10=tightest for * / %) */
} binary_op_entry_t;

/** Sorted by descending precedence (tightest first) for readability. */
static const binary_op_entry_t binary_operators[] = {
    {"*", 10},  {"/", 10},  {"%", 10},
    {"+", 9},   {"-", 9},
    {"<<", 8},  {">>", 8},
    {"<", 7},   {">", 7},   {"<=", 7},  {">=", 7},
    {"==", 6},  {"!=", 6},
    {"&", 5},
    {"^", 4},
    {"|", 3},
    {"&&", 2},
    {"||", 1},
};

/**
 * @brief Get precedence of a token if it's a binary operator, or 0.
 */
static int get_binary_precedence(token_t tok) {
  if (token_get_kind(tok) != CUBEC_TOKEN_SYMBOL) {
    return 0;
  }
  for (size_t i = 0;
       i < sizeof(binary_operators) / sizeof(binary_operators[0]); i++) {
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, binary_operators[i].text)) {
      return binary_operators[i].precedence;
    }
  }
  return 0;
}

/**
 * @brief Make a binary-expression AST node.
 */
static cubec_expression_binary_t make_binary_node(allocator_t allocator,
                                                   node_t left, node_t right,
                                                   string_t opt,
                                                   token_t op_token,
                                                   const char *filename) {
  cubec_expression_binary_t node = allocator_create(
      allocator, &g_cubec_expression_binary_type,
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
static node_t read_unary(allocator_t allocator, vec_t tokens,
                         size_t *position, const char *filename) {
  /* Try prefix unary: !, +, -, &, *, ~ */
  node_t node = read_expression_prefix(allocator, tokens, position, filename);
  if (node) {
    return node;
  }
  /* Fall back to value (atom + postfix .member) */
  return read_value(allocator, tokens, position, filename);
}

/**
 * @brief Precedence-climbing RHS for binary infix operators.
 *        Continues parsing right-hand operands while the next token is a
 *        binary operator whose precedence >= @p min_precedence.
 */
static node_t read_binary_rhs(allocator_t allocator, vec_t tokens,
                               size_t *position, const char *filename,
                               node_t left, int min_precedence) {
  size_t current = *position;

  while (true) {
    skip_whitespace(tokens, &current);
    /* Update position so caller gets correct offset when we break.
     * After skipping whitespace, current points to the next meaningful token. */
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
    node_t right = read_unary(allocator, tokens, &current, filename);
    if (!right) {
      allocator_free(allocator, &opt);
      break;
    }

    /* Look ahead: higher-precedence operators bind to right first
     * (left-associative: use prec + 1 so equal-precedence on right
     *  won't steal the operand; right-associative would use prec). */
    skip_whitespace(tokens, &current);
    right = read_binary_rhs(allocator, tokens, &current, filename, right,
                            prec + 1);

    left = (node_t)make_binary_node(allocator, left, right, opt, op_token,
                                    filename);
    *position = current;
  }

  return left;
}

node_t read_expression_binary(allocator_t allocator, vec_t tokens,
                               size_t *position, const char *filename) {
  /* Parse leftmost unary operand */
  node_t left = read_unary(allocator, tokens, position, filename);
  if (!left) {
    return NULL;
  }

  /* Continue with binary infix operators */
  return read_binary_rhs(allocator, tokens, position, filename, left,
                         1 /* lowest precedence */);
}
