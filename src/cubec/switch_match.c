#include "cubec/switch_match.h"
#include "cubec/ast_factory_internal.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/ast_factory.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_switch_match_init(
    cubec_switch_match_t self, allocator_t allocator,
    cubec_switch_match_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_SWITCH_MATCH,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->is_else = init->is_else;
  self->values = init->values;
  self->body = init->body;
onerror:
  return;
}

static void _cubec_switch_match_dispose(
    cubec_switch_match_t self, allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->values);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_switch_match_clone(
    cubec_switch_match_t self, allocator_t allocator,
    cubec_switch_match_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->is_else = another->is_else;
  self->values = another->values ? TRY_LOCAL(onerror, value_clone(allocator, another->values)) : NULL;
  self->body = TRY_LOCAL(onerror, value_clone(allocator, another->body));
  return;
onerror:
  return;
}

static void _cubec_switch_match_move(
    cubec_switch_match_t self, allocator_t allocator,
    cubec_switch_match_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->is_else = another->is_else;
  self->values = another->values ? TRY_LOCAL(onerror, value_move(allocator, another->values)) : NULL;
  self->body = TRY_LOCAL(onerror, value_move(allocator, another->body));
  return;
onerror:
  return;
}

type_t g_cubec_switch_match_type = {
    .name = "cubec.cubec.switch_match",
    .size = sizeof(struct _cubec_switch_match_t),
    .init = (type_init_fn_t)_cubec_switch_match_init,
    .dispose = (type_dispose_fn_t)_cubec_switch_match_dispose,
    .clone = (type_clone_fn_t)_cubec_switch_match_clone,
    .move = (type_move_fn_t)_cubec_switch_match_move,
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
 *  Parser: read_switch_match — case(v1, v2) -> { } | else -> { }
 * -------------------------------------------------------------------------- */

node_t read_switch_match(allocator_t allocator, vec_t tokens,
                          size_t *position, const char *filename) {
  size_t current = *position;
  vec_t values = NULL;
  node_t body = NULL;
  cubec_switch_match_t node = NULL;
  bool is_else = false;
  location_t start_location = {0};

  /* Check for 'else' or 'case' */
  if (_is_keyword(tokens, current, "else")) {
    is_else = true;
    token_t else_token = TRY_LOCAL(onerror, vec_get(tokens, current));
    start_location = *token_get_location(else_token);
    start_location.filename = filename;
    current++;
    skip_whitespace(tokens, &current);
  } else if (_is_keyword(tokens, current, "case")) {
    token_t case_token = TRY_LOCAL(onerror, vec_get(tokens, current));
    start_location = *token_get_location(case_token);
    start_location.filename = filename;
    current++;
    skip_whitespace(tokens, &current);

    /* Expect '(' */
    if (!_is_symbol(tokens, current, "(")) {
      THROW_LOCAL(onerror, "expected '(' after 'case'");
    }
    current++;
    skip_whitespace(tokens, &current);

    /* Parse match values (comma-separated) */
    values = TRY_LOCAL(cleanup, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));

    while (true) {
      skip_whitespace(tokens, &current);
      node_t value = TRY_LOCAL(cleanup, read_expression(allocator, tokens, &current, filename));
      if (!value) {
        THROW_LOCAL(cleanup, "expected expression in case");
      }
      vec_push(values, value);
      skip_whitespace(tokens, &current);

      if (_is_symbol(tokens, current, ",")) {
        current++;
        skip_whitespace(tokens, &current);
      } else {
        break;
      }
    }

    /* Expect ')' */
    if (!_is_symbol(tokens, current, ")")) {
      THROW_LOCAL(cleanup, "expected ')' after case values");
    }
    current++;
    skip_whitespace(tokens, &current);
  } else {
    /* Not a switch match arm */
    return NULL;
  }

  /* Expect '->' */
  if (!_is_symbol(tokens, current, "->")) {
    THROW_LOCAL(cleanup, "expected '->' after %s", is_else ? "else" : "case");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* Parse body (block) */
  body = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
  if (!body) {
    THROW_LOCAL(cleanup, "expected block after %s ->", is_else ? "else" : "case");
  }

  /* Build location */
  location_t loc = start_location;
  loc.end = body->location.end;

  cubec_switch_match_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_else = is_else,
      .values = values,
      .body = body,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_switch_match_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &body);
  allocator_free(allocator, &values);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &body);
  allocator_free(allocator, &values);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_switch_match
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_switch_match(allocator_t alloc, location_t loc,
                                     bool is_else, vec_t values,
                                     node_t body) {
  cubec_switch_match_init_t init = {.location = loc, .parent = NULL,
                                    .is_else = is_else, .values = values,
                                    .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_switch_match_type, &init);
}
