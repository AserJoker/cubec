#include "cubec/statement_break.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/ast_factory.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_break_init(
    cubec_statement_break_t self, allocator_t allocator,
    cubec_statement_break_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_BREAK,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
onerror:
  return;
}

static void _cubec_statement_break_dispose(
    cubec_statement_break_t self, allocator_t allocator) {
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_break_clone(
    cubec_statement_break_t self, allocator_t allocator,
    cubec_statement_break_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  return;
onerror:
  return;
}

static void _cubec_statement_break_move(
    cubec_statement_break_t self, allocator_t allocator,
    cubec_statement_break_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  return;
onerror:
  return;
}

type_t g_cubec_statement_break_type = {
    .name = "cubec.cubec.statement_break",
    .size = sizeof(struct _cubec_statement_break_t),
    .init = (type_init_fn_t)_cubec_statement_break_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_break_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_break_clone,
    .move = (type_move_fn_t)_cubec_statement_break_move,
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
 *  Parser: read_statement_break — break;
 * -------------------------------------------------------------------------- */

node_t read_statement_break(allocator_t allocator, vec_t tokens,
                             size_t *position, const char *filename) {
  size_t current = *position;

  /* 1. Expect 'break' keyword */
  if (!_is_keyword(tokens, current, "break")) {
    return NULL;
  }
  token_t break_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  location_t start_location = *token_get_location(break_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect ';' */
  if (!_is_symbol(tokens, current, ";")) {
    token_t tok = TRY_LOCAL(onerror, vec_get(tokens, current));
    location_t *loc = token_get_location(tok);
    THROW_LOCAL(onerror, "%s:%" PRIuPTR ":%" PRIuPTR " expected ';' after 'break'",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  token_t semi = TRY_LOCAL(onerror, vec_get(tokens, current));
  current++;

  /* 3. Build location */
  location_t loc = start_location;
  loc.end = token_get_location(semi)->end;

  cubec_statement_break_init_t init = {
      .location = loc,
      .parent = NULL,
  };
  cubec_statement_break_t node = TRY_LOCAL(onerror,
      allocator_create(allocator, &g_cubec_statement_break_type, &init));
  *position = current;
  return &node->super;

onerror:
  return NULL;
}

node_t cubec_ast_create_break_stmt(allocator_t alloc, location_t loc) {
  cubec_statement_break_init_t init = {.location = loc, .parent = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_statement_break_type,
                                  &init);
}
