#include "cubec/switch_match.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_switch_match_init(cubec_switch_match_t self,
                                     allocator_t allocator,
                                     cubec_switch_match_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_SWITCH_MATCH,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->is_else = init->is_else;
  self->values = init->values;
  self->body = init->body;
}

static void _cubec_switch_match_dispose(cubec_switch_match_t self,
                                        allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->values);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_switch_match_clone(cubec_switch_match_t self,
                                      allocator_t allocator,
                                      cubec_switch_match_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->is_else = another->is_else;
  self->values =
      another->values ? value_clone(allocator, another->values) : NULL;
  self->body = value_clone(allocator, another->body);
  return;
}

static void _cubec_switch_match_move(cubec_switch_match_t self,
                                     allocator_t allocator,
                                     cubec_switch_match_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->is_else = another->is_else;
  self->values =
      another->values ? value_move(allocator, another->values) : NULL;
  self->body = value_move(allocator, another->body);
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
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

static bool _is_symbol(vec_t tokens, size_t position, const char *symbol) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  return token_is(token, CUBEC_TOKEN_SYMBOL, symbol);
}

/* --------------------------------------------------------------------------
 *  Parser: read_switch_match — case(v1, v2) -> { } | else -> { }
 * -------------------------------------------------------------------------- */

node_t read_switch_match(context_t ctx, vec_t tokens, size_t *position,
                         const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  vec_t values = NULL;
  node_t body = NULL;
  cubec_switch_match_t node = NULL;
  bool is_else = false;
  location_t start_location = {0};

  /* Check for 'else' or 'case' */
  if (_is_keyword(tokens, current, "else")) {
    is_else = true;
    token_t else_token = vec_get(tokens, current);
    start_location = *token_get_location(else_token);
    start_location.filename = filename;
    current++;
    skip_whitespace(tokens, &current);
  } else if (_is_keyword(tokens, current, "case")) {
    token_t case_token = vec_get(tokens, current);
    start_location = *token_get_location(case_token);
    start_location.filename = filename;
    current++;
    skip_whitespace(tokens, &current);

    /* Expect '(' */
    if (!_is_symbol(tokens, current, "(")) {
      goto onerror;
    }
    current++;
    skip_whitespace(tokens, &current);

    /* Parse match values (comma-separated) */
    values = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});

    while (true) {
      skip_whitespace(tokens, &current);
      node_t value = read_expression_base(ctx, tokens, &current, filename);
      if (!value) {
        goto cleanup;
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
      goto cleanup;
    }
    current++;
    skip_whitespace(tokens, &current);
  } else {
    /* Not a switch match arm */
    return NULL;
  }

  /* Expect '->' */
  if (!_is_symbol(tokens, current, "->")) {
    goto cleanup;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* Parse body (block) */
  body = read_statement_block(ctx, tokens, &current, filename);
  if (!body) {
    goto cleanup;
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
  node = allocator_create(allocator, &g_cubec_switch_match_type, &init);
  if (!node)
    goto cleanup;
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
 *  Factory: create_switch_match
 * -------------------------------------------------------------------------- */

node_t create_switch_match(context_t ctx, location_t loc, bool is_else,
                           vec_t values, node_t body) {
  allocator_t alloc = ctx->allocator;
  cubec_switch_match_init_t init = {
      .is_else = is_else, .values = values, .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_switch_match_type, &init);
}
