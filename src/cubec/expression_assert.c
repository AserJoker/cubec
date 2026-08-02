#include "cubec/expression_assert.h"
#include "core/token.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <string.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void
_cubec_expression_assert_init(cubec_expression_assert_t self,
                               allocator_t allocator,
                               cubec_expression_assert_init_t *init) {
  if (!init)
    return;

  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_ASSERT,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;

  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->host = init->host;
}

static void _cubec_expression_assert_dispose(cubec_expression_assert_t self,
                                             allocator_t allocator) {
  allocator_free(allocator, &self->host);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_assert_clone(cubec_expression_assert_t self,
                                           allocator_t allocator,
                                           cubec_expression_assert_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->host = value_clone(allocator, another->host);
}

static void _cubec_expression_assert_move(cubec_expression_assert_t self,
                                          allocator_t allocator,
                                          cubec_expression_assert_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->host = value_move(allocator, another->host);
}

type_t g_cubec_expression_assert_type = {
    .name = "cubec.cubec.expression_assert",
    .size = sizeof(struct _cubec_expression_assert_t),
    .init = (type_init_fn_t)_cubec_expression_assert_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_assert_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_assert_clone,
    .move = (type_move_fn_t)_cubec_expression_assert_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_assert
 * -------------------------------------------------------------------------- */

/**
 * Try to parse postfix assert/panic: <host>.!
 * Expects '.' token followed by '!' token.
 * Returns NULL if tokens don't match.
 */
node_t read_expression_assert(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename, node_t host) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_expression_assert_t node = NULL;

  /* Expect '.' token first */
  token_t dot_token = vec_get(tokens, current);
  if (!token_is(dot_token, CUBEC_TOKEN_SYMBOL, ".")) {
    return NULL;
  }
  current++;

  /* Expect '!' after '.' */
  skip_whitespace(tokens, &current);
  token_t second_token = vec_get(tokens, current);
  if (!token_is(second_token, CUBEC_TOKEN_SYMBOL, "!")) {
    return NULL;
  }
  location_t start_location = *token_get_location(dot_token);
  start_location.filename = filename;
  current++;

  node = allocator_create(allocator, &g_cubec_expression_assert_type,
                          &(cubec_expression_assert_init_t){
                              .host = host,
                          });
  location_t *loc = token_get_location(dot_token);
  node->super.super.location = *loc;
  node->super.super.location.filename = filename;

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_assert
 * -------------------------------------------------------------------------- */

node_t create_expression_assert(context_t ctx, location_t loc, node_t host) {
  allocator_t alloc = ctx->allocator;
  cubec_expression_assert_init_t init = {
      .location = loc,
      .parent = NULL,
      .host = host,
  };
  return (node_t)allocator_create(alloc, &g_cubec_expression_assert_type,
                                  &init);
}
