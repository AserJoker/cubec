#include "cubec/expression_group.h"
#include "core/allocator.h"
#include "core/token.h"
#include "cubec/ast_factory.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include "engine/context.h"
#include "engine/diagnostic.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_group_init(cubec_expression_group_t self,
                                         allocator_t allocator,
                                         cubec_expression_group_init_t *init) {
  if (!init) return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_GROUP,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->inner = init->inner;
}

static void _cubec_expression_group_dispose(cubec_expression_group_t self,
                                            allocator_t allocator) {
  allocator_free(allocator, &self->inner);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_group_clone(cubec_expression_group_t self,
                                          allocator_t allocator,
                                          cubec_expression_group_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->inner = value_clone(allocator, another->inner);
}

static void _cubec_expression_group_move(cubec_expression_group_t self,
                                         allocator_t allocator,
                                         cubec_expression_group_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->inner = value_move(allocator, another->inner);
}

type_t g_cubec_expression_group_type = {
    .name = "cubec.cubec.expression_group",
    .size = sizeof(struct _cubec_expression_group_t),
    .init = (type_init_fn_t)_cubec_expression_group_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_group_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_group_clone,
    .move = (type_move_fn_t)_cubec_expression_group_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_group
 * -------------------------------------------------------------------------- */

node_t read_expression_group(context_t ctx, vec_t tokens,
                             size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_expression_group_t node = NULL;
  node_t inner = NULL;

  /* Expect '(' */
  token_t open_token = vec_get(tokens, current);
  if (!open_token || !token_is(open_token, CUBEC_TOKEN_SYMBOL, "(")) {
    return NULL;
  }
  location_t start_location = *token_get_location(open_token);
  start_location.filename = filename;
  current++;

  /* Parse inner expression */
  skip_whitespace(tokens, &current);
  inner =
      read_expression(ctx, tokens, &current, filename);
  if (node_is_error(inner)) return inner;
  if (!inner) {
    goto onerror;
  }

  /* Expect ')' */
  skip_whitespace(tokens, &current);
  token_t close_token = vec_get(tokens, current);
  if (!close_token || !token_is(close_token, CUBEC_TOKEN_SYMBOL, ")")) {
    goto onerror;
  }
  current++;

  node = allocator_create(allocator, &g_cubec_expression_group_type,
                          &(cubec_expression_group_init_t){
                              .inner = inner,
                          });
  location_t *loc = token_get_location(open_token);
  node->super.super.location = *loc;
  node->super.super.location.filename = filename;

  *position = current;
  return (node_t)node;

onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                       start_location, "invalid grouped expression");
  ctx->error_count++;
  allocator_free(allocator, &inner);
  allocator_free(allocator, &node);
  return cubec_ast_create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_group
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_group(context_t ctx, location_t loc,
                              node_t inner) {
  allocator_t alloc = ctx->allocator;
                                        cubec_expression_group_init_t init = {
                                        .inner = inner};
  return (node_t)allocator_create(alloc, &g_cubec_expression_group_type,
                                  &init);
}
