#include "cubec/statement_switch.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node_error.h"
#include "cubec/switch_match.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_switch_init(cubec_statement_switch_t self,
                                         allocator_t allocator,
                                         cubec_statement_switch_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_SWITCH,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->condition = init->condition;
  self->matches = init->matches;
}

static void _cubec_statement_switch_dispose(cubec_statement_switch_t self,
                                            allocator_t allocator) {
  allocator_free(allocator, &self->matches);
  allocator_free(allocator, &self->condition);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_switch_clone(cubec_statement_switch_t self,
                                          allocator_t allocator,
                                          cubec_statement_switch_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->condition = value_clone(allocator, another->condition);
  self->matches =
      another->matches ? value_clone(allocator, another->matches) : NULL;
  return;
}

static void _cubec_statement_switch_move(cubec_statement_switch_t self,
                                         allocator_t allocator,
                                         cubec_statement_switch_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->condition = value_move(allocator, another->condition);
  self->matches =
      another->matches ? value_move(allocator, another->matches) : NULL;
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
 *  Parser: read_statement_switch — switch(value) { case(...) -> { } else -> { }
 * }
 * -------------------------------------------------------------------------- */

node_t read_statement_switch(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t condition = NULL;
  vec_t matches = NULL;
  cubec_statement_switch_t node = NULL;

  /* 1. Expect 'switch' keyword */
  if (!_is_keyword(tokens, current, "switch")) {
    return NULL;
  }
  token_t switch_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(switch_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse condition expression */
  condition = read_expression(ctx, tokens, &current, filename);
  if (node_is_error(condition))
    return condition;
  if (!condition)
    goto onerror;
  skip_whitespace(tokens, &current);

  /* 4. Expect ')' */
  if (!_is_symbol(tokens, current, ")")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Expect '{' */
  if (!_is_symbol(tokens, current, "{")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 6. Parse match arms */
  matches = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});

  while (true) {
    skip_whitespace(tokens, &current);
    /* Check for '}' or end */
    if (_is_symbol(tokens, current, "}")) {
      break;
    }
    node_t match = read_switch_match(ctx, tokens, &current, filename);
    if (node_is_error(match)) {
      allocator_free(allocator, &matches);
      allocator_free(allocator, &condition);
      return match;
    }
    if (!match) {
      break;
    }
    vec_push(matches, match);
    skip_whitespace(tokens, &current);
  }

  /* 7. Expect '}' */
  if (!_is_symbol(tokens, current, "}")) {
    goto onerror;
  }
  token_t close_brace = vec_get(tokens, current);
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
  node = allocator_create(allocator, &g_cubec_statement_switch_type, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &matches);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

node_t create_statement_switch(context_t ctx, location_t loc, node_t cond,
                               vec_t matches) {
  allocator_t alloc = ctx->allocator;
  cubec_statement_switch_init_t init = {.condition = cond, .matches = matches};
  return (node_t)allocator_create(alloc, &g_cubec_statement_switch_type, &init);
}

void emit_statement_switch(emit_context_t ctx, node_t node) {
  cubec_statement_switch_t stmt = (cubec_statement_switch_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "switch");
  emit_space(ctx);
  emit_symbol(ctx, "(");
  emit_expression(ctx, stmt->condition);
  emit_symbol(ctx, ")");
  emit_space(ctx);
  emit_symbol(ctx, "{");
  if (vec_get_size(stmt->matches)) {
    emit_indent(ctx, +1);
    emit_newline(ctx);
    size_t count = vec_get_size(stmt->matches);
    for (size_t i = 0; i < count; i++) {
      recover_comments_to(ctx, ((node_t)vec_get(stmt->matches, i))->location.begin.offset);
      emit_switch_match(ctx, vec_get(stmt->matches, i));
      if (i + 1 < count) {
        emit_newline(ctx);
      }
    }
    emit_indent(ctx, -1);
    emit_newline(ctx);
  } else {
    emit_newline(ctx);
  }
  emit_symbol(ctx, "}");
}
