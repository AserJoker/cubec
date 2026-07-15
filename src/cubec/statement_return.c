#include "cubec/statement_return.h"
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
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_return_init(
    cubec_statement_return_t self, allocator_t allocator,
    cubec_statement_return_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_RETURN,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->expression = init->expression;
onerror:
  return;
}

static void _cubec_statement_return_dispose(
    cubec_statement_return_t self, allocator_t allocator) {
  allocator_free(allocator, &self->expression);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_return_clone(
    cubec_statement_return_t self, allocator_t allocator,
    cubec_statement_return_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->expression = another->expression
                         ? TRY_LOCAL(onerror, value_clone(allocator, another->expression))
                         : NULL;
onerror:
  return;
}

static void _cubec_statement_return_move(
    cubec_statement_return_t self, allocator_t allocator,
    cubec_statement_return_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->expression = another->expression
                         ? TRY_LOCAL(onerror, value_move(allocator, another->expression))
                         : NULL;
onerror:
  return;
}

type_t g_cubec_statement_return_type = {
    .name = "cubec.cubec.statement_return",
    .size = sizeof(struct _cubec_statement_return_t),
    .init = (type_init_fn_t)_cubec_statement_return_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_return_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_return_clone,
    .move = (type_move_fn_t)_cubec_statement_return_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check keyword
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
 *  Parser: read_statement_return
 * -------------------------------------------------------------------------- */

node_t read_statement_return(allocator_t allocator, vec_t tokens,
                              size_t *position, const char *filename) {
  size_t current = *position;
  node_t expression = NULL;
  cubec_statement_return_t node = NULL;

  /* 1. Expect 'return' keyword */
  if (!_is_keyword(tokens, current, "return")) {
    return NULL;
  }
  token_t return_token = TRY_LOCAL(cleanup, vec_get(tokens, current));
  location_t start_location = *token_get_location(return_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse optional expression (if not immediately followed by ';') */
  if (!_is_symbol(tokens, current, ";")) {
    expression = TRY_LOCAL(cleanup, read_expression(allocator, tokens, &current, filename));
    if (!expression) {
      THROW_LOCAL(cleanup, "expected expression after 'return'");
    }
    skip_whitespace(tokens, &current);
  }

  /* 3. Expect ';' */
  token_t semi = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!semi || !token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    location_t *loc = token_get_location(semi);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected ';' after return statement",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;

  /* 4. Build location */
  location_t *end_loc = expression ? &expression->location : token_get_location(semi);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  /* 5. Create node */
  cubec_statement_return_init_t init = {
      .location = loc,
      .parent = NULL,
      .expression = expression,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_return_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &expression);
  allocator_free(allocator, &node);
  return NULL;
}

node_t cubec_ast_create_return_stmt(allocator_t alloc, location_t loc,
                                    node_t expr) {
  cubec_statement_return_init_t init = {.location = loc, .parent = NULL,
                                        .expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_statement_return_type,
                                  &init);
}
