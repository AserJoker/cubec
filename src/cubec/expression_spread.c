#include "core/emit_context.h"
#include "core/token_writer.h"
#include "cubec/expression_spread.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void
_cubec_expression_spread_init(cubec_expression_spread_t self,
                              allocator_t allocator,
                              cubec_expression_spread_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_SPREAD,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->value = init->value;
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
  self->value = alloc_clone(allocator, another->value);
}

static void _cubec_expression_spread_move(cubec_expression_spread_t self,
                                          allocator_t allocator,
                                          cubec_expression_spread_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->value = alloc_move(allocator, another->value);
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

node_t read_expression_spread(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_expression_spread_t node = NULL;
  node_t value = NULL;

  /* Expect '...' token */
  token_t spread_token = vec_get(tokens, current);
  if (!token_is(spread_token, CUBEC_TOKEN_SYMBOL, "...")) {
    return NULL;
  }
  location_t start_location = *token_get_location(spread_token);
  start_location.filename = filename;
  current++;

  /* Expect a value expression after the spread operator */
  skip_whitespace(tokens, &current);
  value = read_expression_base(ctx, tokens, &current, filename);
  if (node_is_error(value))
    return value;
  if (!value) {
    goto onerror;
  }

  node = allocator_create(allocator, &g_cubec_expression_spread_type,
                          &(cubec_expression_spread_init_t){
                              .value = value,
                          });

  /* Location spans from first '.' to end of value */
  token_t first_dot = vec_get(tokens, *position);
  location_t *loc = token_get_location(first_dot);
  node->super.super.location = *loc;
  node->super.super.location.filename = filename;

  *position = current;
  return (node_t)node;

onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                       "invalid spread expression");
  allocator_free(allocator, &value);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_spread
 * -------------------------------------------------------------------------- */

node_t create_expression_spread(context_t ctx, location_t loc, node_t value) {
  allocator_t alloc = ctx->allocator;
  cubec_expression_spread_init_t init = {.value = value};
  return (node_t)allocator_create(alloc, &g_cubec_expression_spread_type,
                                  &init);
}

/* --------------------------------------------------------------------------
 *  Writer: write_expression_spread
 * -------------------------------------------------------------------------- */

void emit_expression_spread(emit_context_t ctx, node_t node) {
  cubec_expression_spread_t spread = (cubec_expression_spread_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_symbol(ctx, "...");
  emit_expression(ctx, spread->value);
}
