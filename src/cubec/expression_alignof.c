#include "cubec/expression_alignof.h"
#include "core/allocator.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/ast_factory.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include "engine/context.h"

static void _cubec_expression_alignof_init(cubec_expression_alignof_t self,
                                            allocator_t allocator,
                                            cubec_expression_alignof_init_t *init) {
  if (!init) return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_ALIGNOF,
      .location = init->location,
  };
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->expression = init->expression;
}

static void _cubec_expression_alignof_dispose(cubec_expression_alignof_t self,
                                               allocator_t allocator) {
  allocator_free(allocator, &self->expression);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_alignof_clone(cubec_expression_alignof_t self,
                                             allocator_t allocator,
                                             cubec_expression_alignof_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->expression = value_clone(allocator, another->expression);
  return;
}

static void _cubec_expression_alignof_move(cubec_expression_alignof_t self,
                                            allocator_t allocator,
                                            cubec_expression_alignof_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->expression = value_move(allocator, another->expression);
  return;
}

type_t g_cubec_expression_alignof_type = {
    .name = "cubec.cubec.expression_alignof",
    .size = sizeof(struct _cubec_expression_alignof_t),
    .init = (type_init_fn_t)_cubec_expression_alignof_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_alignof_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_alignof_clone,
    .move = (type_move_fn_t)_cubec_expression_alignof_move,
};

node_t read_expression_alignof(context_t ctx, vec_t tokens,
                                size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;

  /* Expect 'alignof' keyword */
  token_t alignof_token = vec_get(tokens, current);
  if (!token_is(alignof_token, CUBEC_TOKEN_KEYWORD, "alignof")) {
    return NULL;
  }
  size_t alignof_start = current;
  current++;

  /* Expect '(' */
  skip_whitespace(tokens, &current);
  token_t lparen = vec_get(tokens, current);
  if (!token_is(lparen, CUBEC_TOKEN_SYMBOL, "(")) {
    location_t *loc = token_get_location(lparen);
    goto onerror;
  }
  current++;

  /* Parse the inner expression */
  node_t expr = read_expression(ctx, tokens, &current, filename);
  if (!expr) {
    goto onerror;
  }

  /* Expect ')' */
  skip_whitespace(tokens, &current);
  token_t rparen = vec_get(tokens, current);
  if (!token_is(rparen, CUBEC_TOKEN_SYMBOL, ")")) {
    location_t *loc = token_get_location(rparen);
    goto cleanup;
  }
  current++;

  /* Build location spanning from 'alignof' to ')' */
  location_t start_loc = *token_get_location(alignof_token);
  location_t *end_loc = token_get_location(rparen);
  location_t loc = {
      .begin = start_loc.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_expression_alignof_init_t init = {
      .location = loc,
      .parent = NULL,
      .expression = expr,
  };
  cubec_expression_alignof_t node =
      allocator_create(allocator, &g_cubec_expression_alignof_type, &init);
  *position = current;
  return (node_t)&node->super;

cleanup:
  allocator_free(allocator, &expr);
onerror:
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_alignof
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_alignof(context_t ctx, location_t loc,
                                node_t expr) {
  allocator_t alloc = ctx->allocator;
                                          cubec_expression_alignof_init_t init = {
                                          .expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_expression_alignof_type,
                                  &init);
}
