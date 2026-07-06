#include "cubec/expression_postfix_unary.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/string.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <string.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move (reuses expression_binary)
 * -------------------------------------------------------------------------- */

static void _cubec_expression_postfix_unary_init(
    cubec_expression_postfix_unary_t self, allocator_t allocator,
    cubec_expression_postfix_unary_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }

  cubec_expression_init_t super_init = {
      .kind = init->kind,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;

  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.init(&self->super, allocator, &super_init));
  self->left = NULL;
  self->right = init->host;
  self->opt = init->opt;

onerror:
  return;
}

static void
_cubec_expression_postfix_unary_dispose(cubec_expression_postfix_unary_t self,
                                        allocator_t allocator) {
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->opt);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_postfix_unary_clone(
    cubec_expression_postfix_unary_t self, allocator_t allocator,
    cubec_expression_postfix_unary_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->left = NULL;
  self->right = TRY_LOCAL(cleanup, value_clone(allocator, another->right));
  self->opt = (string_t)TRY_LOCAL(cleanup, value_clone(allocator, another->opt));
  return;

cleanup:
  allocator_free(allocator, &self->opt);
  allocator_free(allocator, &self->right);
}

static void
_cubec_expression_postfix_unary_move(cubec_expression_postfix_unary_t self,
                                     allocator_t allocator,
                                     cubec_expression_postfix_unary_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->left = NULL;
  self->right = TRY_LOCAL(cleanup, value_move(allocator, another->right));
  self->opt = (string_t)TRY_LOCAL(cleanup, value_move(allocator, another->opt));
  return;

cleanup:
  allocator_free(allocator, &self->opt);
  allocator_free(allocator, &self->right);
}

type_t g_cubec_expression_postfix_unary_type = {
    .name = "cubec.cubec.expression_postfix_unary",
    .size = sizeof(struct _cubec_expression_binary_t),
    .init = (type_init_fn_t)_cubec_expression_postfix_unary_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_postfix_unary_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_postfix_unary_clone,
    .move = (type_move_fn_t)_cubec_expression_postfix_unary_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_postfix_unary
 * -------------------------------------------------------------------------- */

/**
 * Try to parse postfix unary operators:
 *   - .*  (postfix dereference, e.g. ptr.*)
 *   - .&  (postfix address-of, e.g. &obj)
 *   - .?  (postfix try/unwrap, e.g. result.?)
 *
 * These are composed of separate '.' and '&'/'*'/'?' tokens.
 * Returns NULL if next token is not '.' followed by '&', '*', or '?'.
 */
node_t read_expression_postfix_unary(allocator_t allocator, vec_t tokens,
                                     size_t *position, const char *filename,
                                     node_t host) {
  size_t current = *position;
  cubec_expression_postfix_unary_t node = NULL;
  string_t opt = NULL;

  /* Expect '.' token first */
  token_t dot_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(dot_token, CUBEC_TOKEN_SYMBOL, ".")) {
    return NULL;
  }
  current++;

  /* Expect '&' or '*' after '.' */
  skip_whitespace(tokens, &current);
  token_t second_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!second_token || token_get_kind(second_token) != CUBEC_TOKEN_SYMBOL) {
    return NULL;
  }

  const char *second_op = token_get_string(second_token);
  size_t second_len = token_get_string_length(second_token);

  /* Determine operator: .& or .* */
  const char *op_text = NULL;
  size_t op_len = 0;
  cubec_node_kind_t kind = CUBEC_NODE_EXPRESSION_TRY;
  if (second_len == 1 &&
      (*second_op == '&' || *second_op == '*' || *second_op == '?')) {
    op_text = second_op;
    op_len = second_len;
  } else {
    return NULL;
  }
  if (*second_op == '&') {
    kind = CUBEC_NODE_EXPRESSION_ADDR;
  } else if (*second_op == '*') {
    kind = CUBEC_NODE_EXPRESSION_DEREF;
  } else if (*second_op == '?') {
    kind = CUBEC_NODE_EXPRESSION_TRY;
  }
  current++;

  /* Build operator string ".&" or ".*" */
  opt = TRY_LOCAL(onerror, allocator_create(allocator, &g_string_type, NULL));
  string_nconcat(opt, ".", 1);
  string_nconcat(opt, op_text, op_len);

  node = TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_expression_postfix_unary_type,
                          &(cubec_expression_postfix_unary_init_t){
                              .host = host,
                              .opt = opt,
                              .kind = kind,
                          }));
  location_t *loc = token_get_location(dot_token);
  node->super.super.location = *loc;
  node->super.super.location.filename = filename;

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &opt);
  allocator_free(allocator, &node);
  return NULL;
}
