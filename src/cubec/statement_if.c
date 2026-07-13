#include "cubec/statement_if.h"
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

static void _cubec_statement_if_init(
    cubec_statement_if_t self, allocator_t allocator,
    cubec_statement_if_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_IF,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->condition = init->condition;
  self->then_branch = init->then_branch;
  self->else_branch = init->else_branch;
onerror:
  return;
}

static void _cubec_statement_if_dispose(
    cubec_statement_if_t self, allocator_t allocator) {
  allocator_free(allocator, &self->else_branch);
  allocator_free(allocator, &self->then_branch);
  allocator_free(allocator, &self->condition);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_if_clone(
    cubec_statement_if_t self, allocator_t allocator,
    cubec_statement_if_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->condition = TRY_LOCAL(onerror, value_clone(allocator, another->condition));
  self->then_branch = TRY_LOCAL(onerror, value_clone(allocator, another->then_branch));
  self->else_branch = another->else_branch ? TRY_LOCAL(onerror, value_clone(allocator, another->else_branch)) : NULL;
  return;
onerror:
  return;
}

static void _cubec_statement_if_move(
    cubec_statement_if_t self, allocator_t allocator,
    cubec_statement_if_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->condition = TRY_LOCAL(onerror, value_move(allocator, another->condition));
  self->then_branch = TRY_LOCAL(onerror, value_move(allocator, another->then_branch));
  self->else_branch = another->else_branch ? TRY_LOCAL(onerror, value_move(allocator, another->else_branch)) : NULL;
  return;
onerror:
  return;
}

type_t g_cubec_statement_if_type = {
    .name = "cubec.cubec.statement_if",
    .size = sizeof(struct _cubec_statement_if_t),
    .init = (type_init_fn_t)_cubec_statement_if_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_if_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_if_clone,
    .move = (type_move_fn_t)_cubec_statement_if_move,
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
 *  Parser: read_statement_if — if(condition) { } [else { } | else if(...)]
 * -------------------------------------------------------------------------- */

node_t read_statement_if(allocator_t allocator, vec_t tokens,
                          size_t *position, const char *filename) {
  size_t current = *position;
  node_t condition = NULL;
  node_t then_branch = NULL;
  node_t else_branch = NULL;
  cubec_statement_if_t node = NULL;

  /* 1. Expect 'if' keyword */
  if (!_is_keyword(tokens, current, "if")) {
    return NULL;
  }
  token_t if_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  location_t start_location = *token_get_location(if_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    THROW_LOCAL(onerror, "expected '(' after 'if'");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse condition expression */
  condition = TRY_LOCAL(cleanup, read_expression(allocator, tokens, &current, filename));
  if (!condition) {
    THROW_LOCAL(cleanup, "expected condition expression after '('");
  }
  skip_whitespace(tokens, &current);

  /* 4. Expect ')' */
  if (!_is_symbol(tokens, current, ")")) {
    token_t tok = TRY_LOCAL(cleanup, vec_get(tokens, current));
    location_t *loc = token_get_location(tok);
    THROW_LOCAL(cleanup, "%s:%" PRIuPTR ":%" PRIuPTR " expected ')' after condition",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse then branch (block) */
  then_branch = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
  if (!then_branch) {
    THROW_LOCAL(cleanup, "expected block after if condition");
  }
  skip_whitespace(tokens, &current);

  /* 6. Optional else clause */
  if (_is_keyword(tokens, current, "else")) {
    current++;
    skip_whitespace(tokens, &current);

    if (_is_keyword(tokens, current, "if")) {
      /* else if — parse as nested if statement */
      else_branch = TRY_LOCAL(cleanup, read_statement_if(allocator, tokens, &current, filename));
      if (!else_branch) {
        THROW_LOCAL(cleanup, "expected if after else");
      }
    } else {
      /* else { } */
      else_branch = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
      if (!else_branch) {
        THROW_LOCAL(cleanup, "expected block after else");
      }
    }
  }

  /* 7. Build location */
  location_t loc = start_location;
  if (else_branch) {
    loc.end = else_branch->location.end;
  } else {
    loc.end = then_branch->location.end;
  }

  cubec_statement_if_init_t init = {
      .location = loc,
      .parent = NULL,
      .condition = condition,
      .then_branch = then_branch,
      .else_branch = else_branch,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_if_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &else_branch);
  allocator_free(allocator, &then_branch);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &else_branch);
  allocator_free(allocator, &then_branch);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &node);
  return NULL;
}
