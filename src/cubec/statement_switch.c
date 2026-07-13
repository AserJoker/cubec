#include "cubec/statement_switch.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/switch_match.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_switch_init(
    cubec_statement_switch_t self, allocator_t allocator,
    cubec_statement_switch_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_SWITCH,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->condition = init->condition;
  self->matches = init->matches;
onerror:
  return;
}

static void _cubec_statement_switch_dispose(
    cubec_statement_switch_t self, allocator_t allocator) {
  allocator_free(allocator, &self->matches);
  allocator_free(allocator, &self->condition);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_switch_clone(
    cubec_statement_switch_t self, allocator_t allocator,
    cubec_statement_switch_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->condition = TRY_LOCAL(onerror, value_clone(allocator, another->condition));
  self->matches = another->matches ? TRY_LOCAL(onerror, value_clone(allocator, another->matches)) : NULL;
  return;
onerror:
  return;
}

static void _cubec_statement_switch_move(
    cubec_statement_switch_t self, allocator_t allocator,
    cubec_statement_switch_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->condition = TRY_LOCAL(onerror, value_move(allocator, another->condition));
  self->matches = another->matches ? TRY_LOCAL(onerror, value_move(allocator, another->matches)) : NULL;
  return;
onerror:
  return;
}

type_t g_cubec_statement_switch_type = {
    .name = "cubec.cubec.statement_switch",
    .size = sizeof(struct _cubec_statement_switch_t),
    .init = (type_init_fn_t)_cubec_statement_switch_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_switch_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_switch_clone,
    .move = (type_move_fn_t)_cubec_statement_switch_move,
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
 *  Parser: read_statement_switch — switch(value) { case(...) -> { } else -> { } }
 * -------------------------------------------------------------------------- */

node_t read_statement_switch(allocator_t allocator, vec_t tokens,
                              size_t *position, const char *filename) {
  size_t current = *position;
  node_t condition = NULL;
  vec_t matches = NULL;
  cubec_statement_switch_t node = NULL;

  /* 1. Expect 'switch' keyword */
  if (!_is_keyword(tokens, current, "switch")) {
    return NULL;
  }
  token_t switch_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  location_t start_location = *token_get_location(switch_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    THROW_LOCAL(onerror, "expected '(' after 'switch'");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse condition expression */
  condition = TRY_LOCAL(cleanup, read_expression(allocator, tokens, &current, filename));
  if (!condition) {
    THROW_LOCAL(cleanup, "expected expression after '('");
  }
  skip_whitespace(tokens, &current);

  /* 4. Expect ')' */
  if (!_is_symbol(tokens, current, ")")) {
    token_t tok = TRY_LOCAL(cleanup, vec_get(tokens, current));
    location_t *loc = token_get_location(tok);
    THROW_LOCAL(cleanup, "%s:%" PRIuPTR ":%" PRIuPTR " expected ')' after switch expression",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Expect '{' */
  if (!_is_symbol(tokens, current, "{")) {
    THROW_LOCAL(cleanup, "expected '{' after switch");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 6. Parse match arms */
  matches = TRY_LOCAL(cleanup, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));

  while (true) {
    skip_whitespace(tokens, &current);
    /* Check for '}' or end */
    if (_is_symbol(tokens, current, "}")) {
      break;
    }
    node_t match = TRY_LOCAL(cleanup, read_switch_match(allocator, tokens, &current, filename));
    if (!match) {
      break;
    }
    vec_push(matches, match);
    skip_whitespace(tokens, &current);
  }

  /* 7. Expect '}' */
  if (!_is_symbol(tokens, current, "}")) {
    THROW_LOCAL(cleanup, "expected '}' to close switch body");
  }
  token_t close_brace = TRY_LOCAL(cleanup, vec_get(tokens, current));
  current++;

  /* 8. Build location */
  location_t loc = start_location;
  loc.end = token_get_location(close_brace)->end;

  cubec_statement_switch_init_t init = {
      .location = loc,
      .parent = NULL,
      .condition = condition,
      .matches = matches,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_switch_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &matches);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &matches);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &node);
  return NULL;
}
