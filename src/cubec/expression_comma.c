#include "cubec/expression_comma.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "core/token_writer.h"
#include "cubec/expression.h"
#include "cubec/expression_assignment.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_comma_init(cubec_expression_comma_t self,
                                         allocator_t allocator,
                                         cubec_expression_comma_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_COMMA,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->left = init->left;
  self->right = init->right;
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
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->left = value_clone(allocator, another->left);
  self->right = value_clone(allocator, another->right);
  return;

cleanup:
  allocator_free(allocator, &self->right);
  allocator_free(allocator, &self->left);
}

static void _cubec_expression_comma_move(cubec_expression_comma_t self,
                                         allocator_t allocator,
                                         cubec_expression_comma_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->left = value_move(allocator, another->left);
  self->right = value_move(allocator, another->right);
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

node_t read_expression_comma(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t left = NULL;
  node_t right = NULL;
  cubec_expression_comma_t node = NULL;

  /* Try assignment first (value = expression). If no assignment operator
   * follows the value, read_expression_assignment returns NULL and we
   * fall through to read_expression_base which handles ternary/binary. */
  left = read_expression_assignment(ctx, tokens, &current, filename);
  if (node_is_error(left))
    return left;
  if (!left) {
    left = read_expression_base(ctx, tokens, &current, filename);
  }
  if (node_is_error(left))
    return left;
  if (!left) {
    return NULL;
  }

  location_t start_location = left->location;

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
   * This allows comma expressions like a, b, c to parse as comma(a, comma(b,
   * c)) */
  right = read_expression_comma(ctx, tokens, &current, filename);
  if (node_is_error(right)) {
    allocator_free(allocator, &left);
    return right;
  }
  if (!right) {
    allocator_free(allocator, &left);
    goto onerror;
  }

  /* Create comma node */
  node = allocator_create(allocator, &g_cubec_expression_comma_type,
                          &(cubec_expression_comma_init_t){
                              .left = left,
                              .right = right,
                          });

  *position = current;
  return (node_t)node;

onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                       "invalid comma expression");
  allocator_free(allocator, &right);
  allocator_free(allocator, &left);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_comma
 * -------------------------------------------------------------------------- */

node_t create_expression_comma(context_t ctx, location_t loc, node_t left,
                               node_t right) {
  allocator_t alloc = ctx->allocator;
  cubec_expression_comma_init_t init = {.left = left, .right = right};
  return (node_t)allocator_create(alloc, &g_cubec_expression_comma_type, &init);
}

void emit_expression_comma(emit_context_t ctx, node_t node) {
  cubec_expression_comma_t comma = (cubec_expression_comma_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_expression(ctx, comma->left);
  emit_symbol(ctx, ",");
  emit_space(ctx);
  emit_expression(ctx, comma->right);
}