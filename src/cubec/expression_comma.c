#include "cubec/expression_comma.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_ternary.h"
#include "cubec/node.h"
#include "cubec/token.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_comma_init(cubec_expression_comma_t self,
                                         allocator_t allocator,
                                         cubec_expression_comma_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_COMMA,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.init(&self->super, allocator, &super_init));
  self->left = init->left;
  self->right = init->right;
onerror:
  return;
}

static void _cubec_expression_comma_dispose(cubec_expression_comma_t self,
                                            allocator_t allocator) {
  allocator_free(allocator, &self->left);
  allocator_free(allocator, &self->right);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_comma_clone(cubec_expression_comma_t self,
                                          allocator_t allocator,
                                          cubec_expression_comma_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->left = TRY_LOCAL(cleanup, value_clone(allocator, another->left));
  self->right = TRY_LOCAL(cleanup, value_clone(allocator, another->right));
  return;

cleanup:
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->left);
}

static void _cubec_expression_comma_move(cubec_expression_comma_t self,
                                         allocator_t allocator,
                                         cubec_expression_comma_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->left = TRY_LOCAL(cleanup, value_move(allocator, another->left));
  self->right = TRY_LOCAL(cleanup, value_move(allocator, another->right));
  return;

cleanup:
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->left);
}

type_t g_cubec_expression_comma_type = {
    .name = "cubec.cubec.expression_comma",
    .size = sizeof(struct _cubec_expression_comma_t),
    .init = (type_init_fn_t)_cubec_expression_comma_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_comma_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_comma_clone,
    .move = (type_move_fn_t)_cubec_expression_comma_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_comma
 * -------------------------------------------------------------------------- */

node_t read_expression_comma(allocator_t allocator, vec_t tokens,
                             size_t *position, const char *filename) {
  size_t current = *position;
  node_t left = NULL;
  node_t right = NULL;
  cubec_expression_comma_t node = NULL;

  /* Try to parse a comma expression: left, right
   * The left side can be an expression, assignment, or another comma.
   * The right side is a non-comma expression. */

  /* Parse left operand: first try assignment expression, then fall back to ternary.
   * This allows comma expressions like "a = b, c" to work correctly. */
  left = read_expression_assignment(allocator, tokens, &current, filename);
  if (!left) {
    left = read_expression_ternary(allocator, tokens, &current, filename);
  }
  if (!left) {
    return NULL;
  }

  /* Check if next token is a comma */
  skip_whitespace(tokens, &current);
  token_t tok = vec_get(tokens, current);
  if (!tok || !token_is(tok, CUBEC_TOKEN_SYMBOL, ",")) {
    /* No comma, not a comma expression - return left operand as-is */
    *position = current;
    return left;
  }
  current++; /* consume comma */
  skip_whitespace(tokens, &current);

  /* Parse right operand: recursively call self for right-associativity
   * This allows comma expressions like a, b, c to parse as comma(a, comma(b, c)) */
  right = read_expression_comma(allocator, tokens, &current, filename);
  if (!right) {
    /* No more commas, parse as ternary expression (which includes assignment, binary, etc.) */
    right = read_expression_ternary(allocator, tokens, &current, filename);
    if (!right) {
      allocator_free(allocator, &left);
      THROW_LOCAL(onerror, "expected expression after comma");
    }
  }

  /* Create comma node */
  node = TRY_LOCAL(onerror,
                   allocator_create(allocator, &g_cubec_expression_comma_type,
                                    &(cubec_expression_comma_init_t){
                                        .left = left,
                                        .right = right,
                                    }));

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &right);
  allocator_free(allocator, &left);
  allocator_free(allocator, &node);
  return NULL;
}
