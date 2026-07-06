#include "cubec/expression_spread.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_spread_init(cubec_expression_spread_t self,
                                          allocator_t allocator,
                                          cubec_expression_spread_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_SPREAD,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.init(&self->super, allocator, &super_init));
  self->value = init->value;
onerror:
  return;
}

static void _cubec_expression_spread_dispose(cubec_expression_spread_t self,
                                             allocator_t allocator) {
  allocator_free(allocator, &self->value);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_spread_clone(cubec_expression_spread_t self,
                                           allocator_t allocator,
                                           cubec_expression_spread_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->value = TRY_LOCAL(cleanup, value_clone(allocator, another->value));
  return;

cleanup:
  allocator_free(allocator, &self->value);
}

static void _cubec_expression_spread_move(cubec_expression_spread_t self,
                                          allocator_t allocator,
                                          cubec_expression_spread_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->value = TRY_LOCAL(cleanup, value_move(allocator, another->value));
  return;

cleanup:
  allocator_free(allocator, &self->value);
}

type_t g_cubec_expression_spread_type = {
    .name = "cubec.cubec.expression_spread",
    .size = sizeof(struct _cubec_expression_spread_t),
    .init = (type_init_fn_t)_cubec_expression_spread_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_spread_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_spread_clone,
    .move = (type_move_fn_t)_cubec_expression_spread_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_spread
 * -------------------------------------------------------------------------- */

node_t read_expression_spread(allocator_t allocator, vec_t tokens,
                              size_t *position, const char *filename) {
  size_t current = *position;
  cubec_expression_spread_t node = NULL;
  node_t value = NULL;

  /* Expect '...' token */
  token_t spread_token = vec_get(tokens, current);
  if (!token_is(spread_token, CUBEC_TOKEN_SYMBOL, "...")) {
    return NULL;
  }
  current++;

  /* Expect a value expression after the spread operator */
  skip_whitespace(tokens, &current);
  value =
      TRY_LOCAL(onerror, read_expression(allocator, tokens, &current, filename));
  if (!value) {
    goto onerror;
  }

  node = TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_expression_spread_type,
                          &(cubec_expression_spread_init_t){
                              .value = value,
                          }));

  /* Location spans from first '.' to end of value */
  token_t first_dot = vec_get(tokens, *position);
  location_t *loc = token_get_location(first_dot);
  node->super.super.location = *loc;
  node->super.super.location.filename = filename;

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &value);
  allocator_free(allocator, &node);
  return NULL;
}
