#include "cubec/decorator.h"
#include "core/token.h"
#include "cubec/expression_call.h"
#include "cubec/literal_identifier.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_decorator_init(cubec_decorator_t self, allocator_t allocator,
                                  cubec_decorator_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_DECORATOR,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->expression = init->expression;
}

static void _cubec_decorator_dispose(cubec_decorator_t self,
                                     allocator_t allocator) {
  allocator_free(allocator, &self->expression);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_decorator_clone(cubec_decorator_t self,
                                   allocator_t allocator,
                                   cubec_decorator_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->expression = value_clone(allocator, another->expression);
  return;
}

static void _cubec_decorator_move(cubec_decorator_t self, allocator_t allocator,
                                  cubec_decorator_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->expression = value_move(allocator, another->expression);
  return;
}

type_t g_cubec_decorator_type = {
    .name = "cubec.cubec.decorator",
    .size = sizeof(struct _cubec_decorator_t),
    .init = (type_init_fn_t)_cubec_decorator_init,
    .dispose = (type_dispose_fn_t)_cubec_decorator_dispose,
    .clone = (type_clone_fn_t)_cubec_decorator_clone,
    .move = (type_move_fn_t)_cubec_decorator_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check symbol
 * -------------------------------------------------------------------------- */

static bool _is_symbol(vec_t tokens, size_t position, const char *symbol) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  return token_is(token, CUBEC_TOKEN_SYMBOL, symbol);
}

/**
 * @brief Parse a keyword token as an identifier node (for decorator context).
 *        Keywords like 'inline', 'export' are valid decorator names.
 *        Also handles call syntax: keyword(args).
 */
static node_t _read_keyword_as_identifier(context_t ctx, vec_t tokens,
                                          size_t *position,
                                          const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  token_t tok = vec_get(tokens, current);
  if (!tok || token_get_kind(tok) != CUBEC_TOKEN_KEYWORD) {
    return NULL;
  }

  location_t *loc = token_get_location(tok);
  cubec_literal_identifier_init_t id_init = {
      .location = *loc,
      .parent = NULL,
      .value = NULL,
  };
  cubec_literal_identifier_t id_node =
      allocator_create(allocator, &g_cubec_literal_identifier_type, &id_init);
  if (!id_node)
    goto onerror;
  const char *token_str = token_get_string(tok);
  size_t token_len = token_get_string_length(tok);
  string_nconcat(id_node->value, token_str, token_len);
  node_t expression = (node_t)id_node;
  expression->location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* Try call syntax: keyword(args) — e.g., [[deprecated("reason")]] */
  node_t call =
      read_expression_call(ctx, tokens, &current, filename, expression);
  if (call) {
    expression = call;
  }

  *position = current;
  return expression;

onerror:
  allocator_free(allocator, &expression);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Parser: read_decorator — [[expression]]
 * -------------------------------------------------------------------------- */

node_t read_decorator(context_t ctx, vec_t tokens, size_t *position,
                      const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t expression = NULL;
  cubec_decorator_t node = NULL;

  /* 1. Expect '[[' */
  if (!_is_symbol(tokens, current, "[")) {
    return NULL;
  }
  token_t first_bracket = vec_get(tokens, current);
  current++;
  skip_whitespace(tokens, &current);

  /* Check for second '[' */
  token_t second_bracket = vec_get(tokens, current);
  if (!token_is(second_bracket, CUBEC_TOKEN_SYMBOL, "[")) {
    /* Single '[' — not a decorator */
    return NULL;
  }

  /* This is [[ — committed to parsing a decorator */
  location_t start_location = *token_get_location(first_bracket);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse expression — try keyword-as-identifier first, then normal
   * expression */
  expression = _read_keyword_as_identifier(ctx, tokens, &current, filename);
  if (!expression) {
    expression = read_expression(ctx, tokens, &current, filename);
    if (!expression) {
      goto cleanup;
    }
  }
  skip_whitespace(tokens, &current);

  /* 3. Expect ']]' */
  if (!_is_symbol(tokens, current, "]")) {
    goto cleanup;
  }
  current++;
  skip_whitespace(tokens, &current);

  if (!_is_symbol(tokens, current, "]")) {
    goto cleanup;
  }
  token_t close_bracket = vec_get(tokens, current);
  current++;

  /* 4. Build location */
  location_t loc = start_location;
  loc.end = token_get_location(close_bracket)->end;

  cubec_decorator_init_t init = {
      .location = loc,
      .parent = NULL,
      .expression = expression,
  };
  node = allocator_create(allocator, &g_cubec_decorator_type, &init);
  if (!node)
    goto onerror;
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &expression);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &expression);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: create_decorator
 * -------------------------------------------------------------------------- */

node_t create_decorator(context_t ctx, location_t loc, node_t expr) {
  allocator_t alloc = ctx->allocator;
  cubec_decorator_init_t init = {.expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_decorator_type, &init);
}

void write_decorator(writer_t writer, node_t node) {
  cubec_decorator_t dec = (cubec_decorator_t)node;
  writer_append(writer, "[[");
  write_expression(writer, dec->expression);
  writer_append(writer, "]]");
}
