#include "cubec/expression_sizeof.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"

static void _cubec_expression_sizeof_init(cubec_expression_sizeof_t self,
                                           allocator_t allocator,
                                           cubec_expression_sizeof_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_SIZEOF,
      .location = init->location,
  };
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.init(&self->super, allocator, &super_init));
  self->expression = init->expression;
onerror:
  return;
}

static void _cubec_expression_sizeof_dispose(cubec_expression_sizeof_t self,
                                              allocator_t allocator) {
  allocator_free(allocator, &self->expression);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_sizeof_clone(cubec_expression_sizeof_t self,
                                            allocator_t allocator,
                                            cubec_expression_sizeof_t another) {
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->expression = TRY_LOCAL(onerror, value_clone(allocator, another->expression));
  return;
onerror:
  return;
}

static void _cubec_expression_sizeof_move(cubec_expression_sizeof_t self,
                                           allocator_t allocator,
                                           cubec_expression_sizeof_t another) {
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->expression = TRY_LOCAL(onerror, value_move(allocator, another->expression));
  return;
onerror:
  return;
}

type_t g_cubec_expression_sizeof_type = {
    .name = "cubec.cubec.expression_sizeof",
    .size = sizeof(struct _cubec_expression_sizeof_t),
    .init = (type_init_fn_t)_cubec_expression_sizeof_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_sizeof_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_sizeof_clone,
    .move = (type_move_fn_t)_cubec_expression_sizeof_move,
};

node_t read_expression_sizeof(allocator_t allocator, vec_t tokens,
                               size_t *position, const char *filename) {
  size_t current = *position;

  /* Expect 'sizeof' keyword */
  token_t sizeof_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(sizeof_token, CUBEC_TOKEN_KEYWORD, "sizeof")) {
    return NULL;
  }
  size_t sizeof_start = current;
  current++;

  /* Expect '(' */
  TRY_VOID_LOCAL(onerror, skip_whitespace(tokens, &current));
  token_t lparen = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(lparen, CUBEC_TOKEN_SYMBOL, "(")) {
    location_t *loc = token_get_location(lparen);
    THROW_LOCAL(onerror,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected '(' after 'sizeof'",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;

  /* Parse the inner expression */
  node_t expr = TRY_LOCAL(onerror, read_expression(allocator, tokens, &current, filename));
  if (!expr) {
    THROW_LOCAL(onerror, "%s: expected expression inside sizeof()", filename);
  }

  /* Expect ')' */
  TRY_VOID_LOCAL(cleanup, skip_whitespace(tokens, &current));
  token_t rparen = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!token_is(rparen, CUBEC_TOKEN_SYMBOL, ")")) {
    location_t *loc = token_get_location(rparen);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected ')' to close sizeof()",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;

  /* Build location spanning from 'sizeof' to ')' */
  location_t start_loc = *token_get_location(sizeof_token);
  location_t *end_loc = token_get_location(rparen);
  location_t loc = {
      .begin = start_loc.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_expression_sizeof_init_t init = {
      .location = loc,
      .parent = NULL,
      .expression = expr,
  };
  cubec_expression_sizeof_t node =
      TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_expression_sizeof_type, &init));
  *position = current;
  return (node_t)&node->super;

cleanup:
  allocator_free(allocator, &expr);
onerror:
  return NULL;
}
