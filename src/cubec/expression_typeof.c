#include "cubec/expression_typeof.h"
#include "core/allocator.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/ast_factory.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include "engine/context.h"
#include "engine/diagnostic.h"

static void _cubec_expression_typeof_init(cubec_expression_typeof_t self,
                                           allocator_t allocator,
                                           cubec_expression_typeof_init_t *init) {
  if (!init) return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TYPEOF,
      .location = init->location,
  };
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->expression = init->expression;
}

static void _cubec_expression_typeof_dispose(cubec_expression_typeof_t self,
                                              allocator_t allocator) {
  allocator_free(allocator, &self->expression);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_typeof_clone(cubec_expression_typeof_t self,
                                            allocator_t allocator,
                                            cubec_expression_typeof_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->expression = value_clone(allocator, another->expression);
}

static void _cubec_expression_typeof_move(cubec_expression_typeof_t self,
                                           allocator_t allocator,
                                           cubec_expression_typeof_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->expression = value_move(allocator, another->expression);
}

type_t g_cubec_expression_typeof_type = {
    .name = "cubec.cubec.expression_typeof",
    .size = sizeof(struct _cubec_expression_typeof_t),
    .init = (type_init_fn_t)_cubec_expression_typeof_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_typeof_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_typeof_clone,
    .move = (type_move_fn_t)_cubec_expression_typeof_move,
};

node_t read_expression_typeof(context_t ctx, vec_t tokens,
                               size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t expr = NULL;

  /* Expect 'typeof' keyword */
  token_t typeof_token = vec_get(tokens, current);
  if (!token_is(typeof_token, CUBEC_TOKEN_KEYWORD, "typeof")) {
    return NULL;
  }
  location_t start_location = *token_get_location(typeof_token);
  start_location.filename = filename;
  size_t typeof_start = current;
  current++;

  /* Expect '(' */
  skip_whitespace(tokens, &current);
  token_t lparen = vec_get(tokens, current);
  if (!token_is(lparen, CUBEC_TOKEN_SYMBOL, "(")) {
    goto onerror;
  }
  current++;

  /* Parse the inner expression */
  expr = read_expression(ctx, tokens, &current, filename);
  if (node_is_error(expr)) return expr;
  if (!expr) {
    goto onerror;
  }

  /* Expect ')' */
  skip_whitespace(tokens, &current);
  token_t rparen = vec_get(tokens, current);
  if (!token_is(rparen, CUBEC_TOKEN_SYMBOL, ")")) {
    goto cleanup;
  }
  current++;

  /* Build location spanning from 'typeof' to ')' */
  location_t start_loc = *token_get_location(typeof_token);
  location_t *end_loc = token_get_location(rparen);
  location_t loc = {
      .begin = start_loc.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_expression_typeof_init_t init = {
      .location = loc,
      .parent = NULL,
      .expression = expr,
  };
  cubec_expression_typeof_t node =
      allocator_create(allocator, &g_cubec_expression_typeof_type, &init);
  *position = current;
  return (node_t)&node->super;

cleanup:
onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                       start_location, "invalid typeof expression");
  ctx->error_count++;
  allocator_free(allocator, &expr);
  return cubec_ast_create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_typeof
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_typeof(context_t ctx, location_t loc,
                               node_t expr) {
  allocator_t alloc = ctx->allocator;
  cubec_expression_typeof_init_t init = {.expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_expression_typeof_type,
                                  &init);
}
