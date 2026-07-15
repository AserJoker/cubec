#include "cubec/statement_expression.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/ast_factory.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"

static void _cubec_statement_expression_init(
    cubec_statement_expression_t self, allocator_t allocator,
    cubec_statement_expression_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_EXPRESSION,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->expression = init->expression;
onerror:
  return;
}

static void _cubec_statement_expression_dispose(
    cubec_statement_expression_t self, allocator_t allocator) {
  allocator_free(allocator, &self->expression);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_expression_clone(
    cubec_statement_expression_t self, allocator_t allocator,
    cubec_statement_expression_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->expression = TRY_LOCAL(onerror, value_clone(allocator, another->expression));
  return;
onerror:
  return;
}

static void _cubec_statement_expression_move(
    cubec_statement_expression_t self, allocator_t allocator,
    cubec_statement_expression_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->expression = TRY_LOCAL(onerror, value_move(allocator, another->expression));
  return;
onerror:
  return;
}

type_t g_cubec_statement_expression_type = {
    .name = "cubec.cubec.statement_expression",
    .size = sizeof(struct _cubec_statement_expression_t),
    .init = (type_init_fn_t)_cubec_statement_expression_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_expression_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_expression_clone,
    .move = (type_move_fn_t)_cubec_statement_expression_move,
};

node_t read_statement_expression(allocator_t allocator, vec_t tokens,
                                 size_t *position, const char *filename) {
  size_t current = *position;

  /* Try to parse the expression */
  node_t expr = TRY_LOCAL(onerror, read_expression(allocator, tokens, &current, filename));
  if (!expr) {
    return NULL;
  }

  /* Expect semicolon after the expression */
  TRY_VOID_LOCAL(cleanup, skip_whitespace(tokens, &current));
  token_t semi = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    location_t *loc = token_get_location(semi);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected ';' after expression statement",
                filename, loc->begin.line + 1, loc->begin.column);
  }

  /* Build location spanning from expression start to semicolon */
  location_t start_loc = expr->location;
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_loc.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  /* Consume the semicolon */
  current++;

  cubec_statement_expression_init_t init = {
      .location = loc,
      .parent = NULL,
      .expression = expr,
  };
  cubec_statement_expression_t node =
      TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_expression_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &expr);
onerror:
  return NULL;
}

node_t cubec_ast_create_expr_stmt(allocator_t alloc, location_t loc,
                                  node_t expr) {
  cubec_statement_expression_init_t init = {.location = loc, .parent = NULL,
                                            .expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_statement_expression_type,
                                  &init);
}
