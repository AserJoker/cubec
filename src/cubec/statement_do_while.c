#include "cubec/statement_do_while.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/ast_factory.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_do_while_init(
    cubec_statement_do_while_t self, allocator_t allocator,
    cubec_statement_do_while_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_DO_WHILE,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->body = init->body;
  self->condition = init->condition;
onerror:
  return;
}

static void _cubec_statement_do_while_dispose(
    cubec_statement_do_while_t self, allocator_t allocator) {
  allocator_free(allocator, &self->condition);
  allocator_free(allocator, &self->body);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_do_while_clone(
    cubec_statement_do_while_t self, allocator_t allocator,
    cubec_statement_do_while_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->body = TRY_LOCAL(onerror, value_clone(allocator, another->body));
  self->condition = TRY_LOCAL(onerror, value_clone(allocator, another->condition));
  return;
onerror:
  return;
}

static void _cubec_statement_do_while_move(
    cubec_statement_do_while_t self, allocator_t allocator,
    cubec_statement_do_while_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->body = TRY_LOCAL(onerror, value_move(allocator, another->body));
  self->condition = TRY_LOCAL(onerror, value_move(allocator, another->condition));
  return;
onerror:
  return;
}

type_t g_cubec_statement_do_while_type = {
    .name = "cubec.cubec.statement_do_while",
    .size = sizeof(struct _cubec_statement_do_while_t),
    .init = (type_init_fn_t)_cubec_statement_do_while_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_do_while_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_do_while_clone,
    .move = (type_move_fn_t)_cubec_statement_do_while_move,
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
 *  Parser: read_statement_do_while — do { } while(condition);
 * -------------------------------------------------------------------------- */

node_t read_statement_do_while(allocator_t allocator, vec_t tokens,
                                size_t *position, const char *filename) {
  size_t current = *position;
  node_t body = NULL;
  node_t condition = NULL;
  cubec_statement_do_while_t node = NULL;

  /* 1. Expect 'do' keyword */
  if (!_is_keyword(tokens, current, "do")) {
    return NULL;
  }
  token_t do_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  location_t start_location = *token_get_location(do_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse body (block) */
  body = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
  if (!body) {
    THROW_LOCAL(cleanup, "expected block after 'do'");
  }
  skip_whitespace(tokens, &current);

  /* 3. Expect 'while' keyword */
  if (!_is_keyword(tokens, current, "while")) {
    THROW_LOCAL(cleanup, "expected 'while' after do block");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 4. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    THROW_LOCAL(cleanup, "expected '(' after 'while'");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse condition */
  condition = TRY_LOCAL(cleanup, read_expression(allocator, tokens, &current, filename));
  if (!condition) {
    THROW_LOCAL(cleanup, "expected condition after '('");
  }
  skip_whitespace(tokens, &current);

  /* 6. Expect ')' */
  if (!_is_symbol(tokens, current, ")")) {
    token_t tok = TRY_LOCAL(cleanup, vec_get(tokens, current));
    location_t *loc = token_get_location(tok);
    THROW_LOCAL(cleanup, "%s:%" PRIuPTR ":%" PRIuPTR " expected ')' after condition",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 7. Expect ';' */
  if (!_is_symbol(tokens, current, ";")) {
    token_t tok = TRY_LOCAL(cleanup, vec_get(tokens, current));
    location_t *loc = token_get_location(tok);
    THROW_LOCAL(cleanup, "%s:%" PRIuPTR ":%" PRIuPTR " expected ';' after do-while",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  token_t semi = TRY_LOCAL(cleanup, vec_get(tokens, current));
  current++;

  /* 8. Build location */
  location_t loc = start_location;
  loc.end = token_get_location(semi)->end;

  cubec_statement_do_while_init_t init = {
      .location = loc,
      .parent = NULL,
      .body = body,
      .condition = condition,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_do_while_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &condition);
  allocator_free(allocator, &body);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &condition);
  allocator_free(allocator, &body);
  allocator_free(allocator, &node);
  return NULL;
}

node_t cubec_ast_create_do_while_stmt(allocator_t alloc, location_t loc,
                                      node_t body, node_t cond) {
  cubec_statement_do_while_init_t init = {.location = loc, .parent = NULL,
                                          .body = body, .condition = cond};
  return (node_t)allocator_create(alloc, &g_cubec_statement_do_while_type,
                                  &init);
}
