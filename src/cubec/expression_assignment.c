#include "cubec/expression_assignment.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/token.h"
#include "cubec/ast_factory.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>
#include "engine/context.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_assignment_init(
    cubec_expression_assignment_t self, allocator_t allocator,
    cubec_expression_assignment_init_t *init) {
  if (!init) return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_ASSIGNMENT,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);

  self->left = init->lvalue;
  self->right = init->rvalue;
  self->opt = init->opt;
}

static void _cubec_expression_assignment_dispose(
    cubec_expression_assignment_t self, allocator_t allocator) {
  allocator_free(allocator, &self->left);
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->opt);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_assignment_clone(
    cubec_expression_assignment_t self, allocator_t allocator,
    cubec_expression_assignment_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->left = value_clone(allocator, another->left);
  self->right = value_clone(allocator, another->right);
  self->opt = (string_t)value_clone(allocator, another->opt);
  return;

cleanup:
  /* Clean up already-cloned members on failure */
  allocator_free(allocator, &self->opt);
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->left);
}

static void _cubec_expression_assignment_move(
    cubec_expression_assignment_t self, allocator_t allocator,
    cubec_expression_assignment_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->left = value_move(allocator, another->left);
  self->right = value_move(allocator, another->right);
  self->opt = (string_t)value_move(allocator, another->opt);
  return;

cleanup:
  /* Clean up already-moved members on failure */
  allocator_free(allocator, &self->opt);
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->left);
}

type_t g_cubec_expression_assignment_type = {
    .name = "cubec.cubec.expression_assignment",
    .size = sizeof(struct _cubec_expression_binary_t),
    .init = (type_init_fn_t)_cubec_expression_assignment_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_assignment_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_assignment_clone,
    .move = (type_move_fn_t)_cubec_expression_assignment_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_assignment
 * -------------------------------------------------------------------------- */

/** Set of supported assignment operator strings (sorted by length descending) */
static const char *assignment_operators[] = {
    "<<=", ">>=", "&&=", "||=",  // 3-char operators first
    "+=",  "-=",  "*=",  "/=",  "%=",  "&=",  "|=",  "^=",  "=",
};

static bool is_assignment_operator_token(token_t tok) {
  if (token_get_kind(tok) != CUBEC_TOKEN_SYMBOL) {
    return false;
  }
  for (size_t i = 0;
       i < sizeof(assignment_operators) / sizeof(assignment_operators[0]);
       i++) {
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, assignment_operators[i])) {
      return true;
    }
  }
  return false;
}

node_t read_expression_assignment(context_t ctx, vec_t tokens,
                                  size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t lvalue = NULL;
  node_t rvalue = NULL;
  cubec_expression_assignment_t node = NULL;
  string_t opt = NULL;
  token_t op_token = NULL;

  /* First, read a value as the potential lvalue */
  lvalue = read_value(ctx, tokens, &current, filename);
  if (!lvalue) {
    return NULL;
  }

  /* Skip whitespace and check for assignment operator */
  skip_whitespace(tokens, &current);
  token_t tok = vec_get(tokens, current);

  if (!is_assignment_operator_token(tok)) {
    /* Not an assignment expression — free the lvalue and return NULL.
     * The caller (read_expression_comma) will fall through to try
     * read_expression_base which handles binary/ternary chains. */
    allocator_free(allocator, &lvalue);
    return NULL;
  }

  /* This is an assignment expression — committed from here */
  op_token = tok;
  const char *op_text = token_get_string(op_token);
  size_t op_len = token_get_string_length(op_token);
  opt = allocator_create(allocator, &g_string_type, NULL);
  string_nconcat(opt, op_text, op_len);
  current++; /* consume operator */

  /* Parse rvalue expression */
  skip_whitespace(tokens, &current);
  rvalue = read_expression_base(ctx, tokens, &current, filename);
  if (!rvalue) {
    /* No rvalue expression found — this is an error */
    goto onerror;
  }

  /* Create assignment node */
  node = allocator_create(allocator, &g_cubec_expression_assignment_type,
                          &(cubec_expression_assignment_init_t){
                              .lvalue = lvalue,
                              .rvalue = rvalue,
                              .opt = opt,
                          });

  /* Location spans from lvalue start to rvalue end */
  {
    location_t loc = lvalue->location;
    loc.end = rvalue->location.end;
    node->super.super.location = loc;
    node->super.super.location.filename = filename;
  }

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &opt);
  allocator_free(allocator, &rvalue);
  allocator_free(allocator, &lvalue);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_assignment
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_assignment(context_t ctx, location_t loc,
                                   const char *op, node_t lvalue,
                                   node_t rvalue) {
  allocator_t alloc = ctx->allocator;
  string_t op_str = _make_string(ctx, op);
  cubec_expression_assignment_init_t init = {.location = loc, .parent = NULL,
                                              .lvalue = lvalue,
                                              .rvalue = rvalue,
                                              .opt = op_str};
  return (node_t)allocator_create(alloc, &g_cubec_expression_assignment_type,
                                  &init);
}