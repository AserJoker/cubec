#include "cubec/expression_wildcard.h"
#include "cubec/expression.h"
#include "core/allocator.h"
#include "core/token.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "engine/context.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_wildcard_init(cubec_expression_wildcard_t self,
                                            allocator_t allocator, void *arg) {
  (void)arg;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_WILDCARD,
      .parent = NULL,
  };
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->is_tuple = false;
}

static void _cubec_expression_wildcard_dispose(cubec_expression_wildcard_t self,
                                                allocator_t allocator) {
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_wildcard_clone(cubec_expression_wildcard_t self,
                                              allocator_t allocator,
                                              cubec_expression_wildcard_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->is_tuple = another->is_tuple;
}

static void _cubec_expression_wildcard_move(cubec_expression_wildcard_t self,
                                             allocator_t allocator,
                                             cubec_expression_wildcard_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->is_tuple = another->is_tuple;
}

type_t g_cubec_expression_wildcard_type = {
    .name = "cubec.cubec.expression_wildcard",
    .size = sizeof(struct _cubec_expression_wildcard_t),
    .init = (type_init_fn_t)_cubec_expression_wildcard_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_wildcard_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_wildcard_clone,
    .move = (type_move_fn_t)_cubec_expression_wildcard_move,
};

/* --------------------------------------------------------------------------
 *  Parser
 * -------------------------------------------------------------------------- */

node_t read_expression_wildcard(context_t ctx, vec_t tokens,
                                size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;

  skip_whitespace(tokens, &current);
  token_t tok = vec_get(tokens, current);
  if (!token_is(tok, CUBEC_TOKEN_SYMBOL, "?"))
    return NULL;

  current++; /* consume `?` */

  cubec_expression_wildcard_t node =
      (cubec_expression_wildcard_t)allocator_create(
          allocator, &g_cubec_expression_wildcard_type, NULL);
  if (!node) return NULL;

  location_t loc = *token_get_location(tok);
  loc.filename = filename;
  node->super.super.location = loc;
  node->is_tuple = false;

  *position = current;
  return (node_t)node;
}
