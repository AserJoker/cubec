#include "core/emit_context.h"
#include "core/token_writer.h"
#include "cubec/expression_typeof.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

static void
_cubec_expression_typeof_init(cubec_expression_typeof_t self,
                              allocator_t allocator,
                              cubec_expression_typeof_init_t *init) {
  if (!init)
    return;
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
  self->expression = alloc_clone(allocator, another->expression);
}

static void _cubec_expression_typeof_move(cubec_expression_typeof_t self,
                                          allocator_t allocator,
                                          cubec_expression_typeof_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->expression = alloc_move(allocator, another->expression);
}

type_t g_cubec_expression_typeof_type = {
    .name = "cubec.cubec.expression_typeof",
    .size = sizeof(struct _cubec_expression_typeof_t),
    .init = (type_init_fn_t)_cubec_expression_typeof_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_typeof_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_typeof_clone,
    .move = (type_move_fn_t)_cubec_expression_typeof_move,
};

node_t read_expression_typeof(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename) {
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
  if (node_is_error(expr))
    return expr;
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
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                       "invalid typeof expression");
  allocator_free(allocator, &expr);
  return create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_typeof
 * -------------------------------------------------------------------------- */

node_t create_expression_typeof(context_t ctx, location_t loc, node_t expr) {
  allocator_t alloc = ctx->allocator;
  cubec_expression_typeof_init_t init = {.expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_expression_typeof_type,
                                  &init);
}

/* --------------------------------------------------------------------------
 *  Writer: write_expression_typeof
 * -------------------------------------------------------------------------- */

void emit_expression_typeof(emit_context_t ctx, node_t node) {
  cubec_expression_typeof_t expr = (cubec_expression_typeof_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "typeof");
  emit_symbol(ctx, "(");
  emit_expression(ctx, expr->expression);
  emit_symbol(ctx, ")");
}
