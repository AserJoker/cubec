#include "cubec/statement_return.h"
#include "core/token.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_return_init(cubec_statement_return_t self,
                                         allocator_t allocator,
                                         cubec_statement_return_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_RETURN,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->expression = init->expression;
}

static void _cubec_statement_return_dispose(cubec_statement_return_t self,
                                            allocator_t allocator) {
  allocator_free(allocator, &self->expression);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_return_clone(cubec_statement_return_t self,
                                          allocator_t allocator,
                                          cubec_statement_return_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->expression =
      another->expression ? value_clone(allocator, another->expression) : NULL;
}

static void _cubec_statement_return_move(cubec_statement_return_t self,
                                         allocator_t allocator,
                                         cubec_statement_return_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->expression =
      another->expression ? value_move(allocator, another->expression) : NULL;
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
 *  Parser: read_statement_return
 * -------------------------------------------------------------------------- */

node_t read_statement_return(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t expression = NULL;
  cubec_statement_return_t node = NULL;

  /* 1. Expect 'return' keyword */
  if (!_is_keyword(tokens, current, "return")) {
    return NULL;
  }
  token_t return_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(return_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse optional expression (if not immediately followed by ';') */
  if (!_is_symbol(tokens, current, ";")) {
    expression = read_expression(ctx, tokens, &current, filename);
    if (node_is_error(expression)) {
      goto onerror;
    }
    if (!expression) {
      goto onerror;
    }
    skip_whitespace(tokens, &current);
  }

  /* 3. Expect ';' */
  token_t semi = vec_get(tokens, current);
  if (!semi || !token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    goto onerror;
  }
  current++;

  /* 4. Build location */
  location_t *end_loc =
      expression ? &expression->location : token_get_location(semi);
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
  node = allocator_create(allocator, &g_cubec_statement_return_type, &init);
  *position = current;
  return &node->super;

onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                       "invalid return statement");
  ctx->error_count++;
  allocator_free(allocator, &expression);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

node_t create_statement_return(context_t ctx, location_t loc, node_t expr) {
  allocator_t alloc = ctx->allocator;
  cubec_statement_return_init_t init = {.expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_statement_return_type, &init);
}

void write_statement_return(writer_t writer, node_t node) {
  cubec_statement_return_t stmt = (cubec_statement_return_t)node;
  writer_append(writer, "return");
  if (stmt->expression) {
    writer_append(writer, " ");
    write_expression(writer, stmt->expression);
  }
  writer_append(writer, ";");
  writer_newline(writer, 0);
}
