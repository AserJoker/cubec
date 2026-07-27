#include "cubec/statement_defer.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/ast_factory.h"
#include "cubec/node_error.h"
#include "core/allocator.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/function_capture.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/token.h"
#include <inttypes.h>
#include "engine/context.h"
#include "engine/diagnostic.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_defer_init(
    cubec_statement_defer_t self, allocator_t allocator,
    cubec_statement_defer_init_t *init) {
  if (!init) return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_DEFER,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->captures = init->captures;
  self->body = init->body;
}

static void _cubec_statement_defer_dispose(
    cubec_statement_defer_t self, allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->captures);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_defer_clone(
    cubec_statement_defer_t self, allocator_t allocator,
    cubec_statement_defer_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->captures = another->captures
                       ? value_clone(allocator, another->captures)
                       : NULL;
  self->body = value_clone(allocator, another->body);
  return;
}

static void _cubec_statement_defer_move(
    cubec_statement_defer_t self, allocator_t allocator,
    cubec_statement_defer_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->captures = another->captures
                       ? value_move(allocator, another->captures)
                       : NULL;
  self->body = value_move(allocator, another->body);
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

static bool _is_symbol(vec_t tokens, size_t position, const char *symbol) {
  token_t token = vec_get(tokens, position);
  if (!token) return false;
  return token_is(token, CUBEC_TOKEN_SYMBOL, symbol);
}

/* --------------------------------------------------------------------------
 *  Parser: read_statement_defer — defer [|captures|] { }
 * -------------------------------------------------------------------------- */

node_t read_statement_defer(context_t ctx, vec_t tokens,
                             size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  vec_t captures = NULL;
  node_t body = NULL;
  cubec_statement_defer_t node = NULL;

  /* 1. Expect 'defer' keyword */
  if (!_is_keyword(tokens, current, "defer")) {
    return NULL;
  }
  token_t defer_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(defer_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse optional capture list: || or |x, y| */
  if (_is_symbol(tokens, current, "||")) {
    /* Empty capture list — skip, captures remains NULL */
    current++;
    skip_whitespace(tokens, &current);
  } else if (_is_symbol(tokens, current, "|")) {
    /* Non-empty capture list */
    current++;
    skip_whitespace(tokens, &current);

    captures = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});

    while (true) {
      node_t cap = read_function_capture(ctx, tokens, &current, filename);
      if (!cap) {
        goto onerror;
      }
      vec_push(captures, cap);

      skip_whitespace(tokens, &current);

      if (_is_symbol(tokens, current, ",")) {
        current++;
        skip_whitespace(tokens, &current);
      } else if (_is_symbol(tokens, current, "|")) {
        current++;
        skip_whitespace(tokens, &current);
        break;
      } else {
        token_t tok = vec_get(tokens, current);
        location_t *loc = token_get_location(tok);
        goto onerror;
      }
    }
  }

  /* 3. Parse block body: defer { ... } */
  body = read_statement_block(ctx, tokens, &current, filename);
  if (!body) {
    goto onerror;
  }

  /* 4. Build location */
  location_t loc = start_location;
  loc.end = body->location.end;

  cubec_statement_defer_init_t init = {
      .location = loc,
      .parent = NULL,
      .captures = captures,
      .body = body,
  };
  node = allocator_create(allocator, &g_cubec_statement_defer_type, &init);
  *position = current;
  return &node->super;

onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                       start_location, "invalid defer statement");
  ctx->error_count++;
  allocator_free(allocator, &body);
  allocator_free(allocator, &captures);
  allocator_free(allocator, &node);
  return cubec_ast_create_error(ctx, start_location);
}

node_t cubec_ast_create_defer_stmt(context_t ctx, location_t loc,
                                   vec_t captures, node_t body) {
  allocator_t alloc = ctx->allocator;
                                       cubec_statement_defer_init_t init = {
                                       .captures = captures, .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_defer_type,
                                  &init);
}
