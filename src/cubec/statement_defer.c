#include "cubec/statement_defer.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/statement_expression.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_defer_init(
    cubec_statement_defer_t self, allocator_t allocator,
    cubec_statement_defer_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_DEFER,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->body = init->body;
onerror:
  return;
}

static void _cubec_statement_defer_dispose(
    cubec_statement_defer_t self, allocator_t allocator) {
  allocator_free(allocator, &self->body);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_defer_clone(
    cubec_statement_defer_t self, allocator_t allocator,
    cubec_statement_defer_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->body = TRY_LOCAL(onerror, value_clone(allocator, another->body));
  return;
onerror:
  return;
}

static void _cubec_statement_defer_move(
    cubec_statement_defer_t self, allocator_t allocator,
    cubec_statement_defer_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->body = TRY_LOCAL(onerror, value_move(allocator, another->body));
  return;
onerror:
  return;
}

type_t g_cubec_statement_defer_type = {
    .name = "cubec.cubec.statement_defer",
    .size = sizeof(struct _cubec_statement_defer_t),
    .init = (type_init_fn_t)_cubec_statement_defer_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_defer_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_defer_clone,
    .move = (type_move_fn_t)_cubec_statement_defer_move,
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

/* --------------------------------------------------------------------------
 *  Parser: read_statement_defer — defer expr; | defer { }
 * -------------------------------------------------------------------------- */

node_t read_statement_defer(allocator_t allocator, vec_t tokens,
                             size_t *position, const char *filename) {
  size_t current = *position;
  node_t body = NULL;
  cubec_statement_defer_t node = NULL;

  /* 1. Expect 'defer' keyword */
  if (!_is_keyword(tokens, current, "defer")) {
    return NULL;
  }
  token_t defer_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  location_t start_location = *token_get_location(defer_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Try block form first: defer { ... } */
  body = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
  if (!body) {
    /* 3. Expression form: defer expr; — parse as expression statement */
    body = TRY_LOCAL(cleanup, read_statement_expression(allocator, tokens, &current, filename));
  }
  if (!body) {
    THROW_LOCAL(cleanup, "expected expression or block after 'defer'");
  }

  /* 4. Build location */
  location_t loc = start_location;
  loc.end = body->location.end;

  cubec_statement_defer_init_t init = {
      .location = loc,
      .parent = NULL,
      .body = body,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_defer_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &body);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &body);
  allocator_free(allocator, &node);
  return NULL;
}
