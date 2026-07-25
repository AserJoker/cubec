#include "cubec/statement_while.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/ast_factory.h"
#include "core/allocator.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include <inttypes.h>
#include "engine/context.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_while_init(
    cubec_statement_while_t self, allocator_t allocator,
    cubec_statement_while_init_t *init) {
  if (!init) return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_WHILE,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->condition = init->condition;
  self->body = init->body;
}

static void _cubec_statement_while_dispose(
    cubec_statement_while_t self, allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->condition);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_while_clone(
    cubec_statement_while_t self, allocator_t allocator,
    cubec_statement_while_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->condition = value_clone(allocator, another->condition);
  self->body = value_clone(allocator, another->body);
  return;
}

static void _cubec_statement_while_move(
    cubec_statement_while_t self, allocator_t allocator,
    cubec_statement_while_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->condition = value_move(allocator, another->condition);
  self->body = value_move(allocator, another->body);
  return;
}

type_t g_cubec_statement_while_type = {
    .name = "cubec.cubec.statement_while",
    .size = sizeof(struct _cubec_statement_while_t),
    .init = (type_init_fn_t)_cubec_statement_while_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_while_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_while_clone,
    .move = (type_move_fn_t)_cubec_statement_while_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check keyword / symbol
 * -------------------------------------------------------------------------- */

static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token) return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD) return false;
  return location_is(token_get_location(token), keyword);
}

static bool _is_symbol(vec_t tokens, size_t position, const char *symbol) {
  token_t token = vec_get(tokens, position);
  if (!token) return false;
  return token_is(token, CUBEC_TOKEN_SYMBOL, symbol);
}

/* --------------------------------------------------------------------------
 *  Parser: read_statement_while — while(condition) { }
 * -------------------------------------------------------------------------- */

node_t read_statement_while(context_t ctx, vec_t tokens,
                             size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t condition = NULL;
  node_t body = NULL;
  cubec_statement_while_t node = NULL;

  /* 1. Expect 'while' keyword */
  if (!_is_keyword(tokens, current, "while")) {
    return NULL;
  }
  token_t while_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(while_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse condition */
  condition = read_expression(ctx, tokens, &current, filename);
  if (!condition) {
    goto cleanup;
  }
  skip_whitespace(tokens, &current);

  /* 4. Expect ')' */
  if (!_is_symbol(tokens, current, ")")) {
    token_t tok = vec_get(tokens, current);
    location_t *loc = token_get_location(tok);
    goto cleanup;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse body (any statement) */
  body = read_statement(ctx, tokens, &current, filename);
  if (!body) {
    goto cleanup;
  }

  /* 6. Build location */
  location_t loc = start_location;
  loc.end = body->location.end;

  cubec_statement_while_init_t init = {
      .location = loc,
      .parent = NULL,
      .condition = condition,
      .body = body,
  };
  node = allocator_create(allocator, &g_cubec_statement_while_type, &init);
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &body);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &body);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &node);
  return NULL;
}

node_t cubec_ast_create_while_stmt(context_t ctx, location_t loc,
                                   node_t cond, node_t body) {
  allocator_t alloc = ctx->allocator;
                                       cubec_statement_while_init_t init = {
                                       .condition = cond, .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_while_type,
                                  &init);
}
