#include "cubec/expression_type_function.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_type_function_init(
    cubec_expression_type_function_t self, allocator_t allocator,
    cubec_expression_type_function_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TYPE_FUNCTION,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  TRY_VOID_LOCAL(onerror,
                 g_cubec_expression_type.init(&self->super, allocator, &super_init));
  self->parameters = init->parameters;
  self->return_type = init->return_type;
  self->is_c_variadic = init->is_c_variadic;
onerror:
  return;
}

static void _cubec_expression_type_function_dispose(
    cubec_expression_type_function_t self, allocator_t allocator) {
  allocator_free(allocator, &self->return_type);
  allocator_free(allocator, &self->parameters);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_type_function_clone(
    cubec_expression_type_function_t self, allocator_t allocator,
    cubec_expression_type_function_t another) {
  TRY_VOID_LOCAL(
      cleanup,
      g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->parameters = TRY_LOCAL(cleanup, value_clone(allocator, another->parameters));
  self->return_type = another->return_type
                          ? TRY_LOCAL(cleanup, value_clone(allocator, another->return_type))
                          : NULL;
  self->is_c_variadic = another->is_c_variadic;
  return;

cleanup:
  allocator_free(allocator, &self->parameters);
  allocator_free(allocator, &self->return_type);
}

static void _cubec_expression_type_function_move(
    cubec_expression_type_function_t self, allocator_t allocator,
    cubec_expression_type_function_t another) {
  TRY_VOID_LOCAL(
      cleanup,
      g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->parameters = TRY_LOCAL(cleanup, value_move(allocator, another->parameters));
  self->return_type = another->return_type
                          ? TRY_LOCAL(cleanup, value_move(allocator, another->return_type))
                          : NULL;
  self->is_c_variadic = another->is_c_variadic;
  return;

cleanup:
  allocator_free(allocator, &self->parameters);
  allocator_free(allocator, &self->return_type);
}

type_t g_cubec_expression_type_function_type = {
    .name = "cubec.cubec.expression_type_function",
    .size = sizeof(struct _cubec_expression_type_function_t),
    .init = (type_init_fn_t)_cubec_expression_type_function_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_type_function_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_type_function_clone,
    .move = (type_move_fn_t)_cubec_expression_type_function_move,
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
 *  Parser: read_expression_type_function
 * -------------------------------------------------------------------------- */

node_t read_expression_type_function(allocator_t allocator, vec_t tokens,
                                     size_t *position, const char *filename) {
  size_t current = *position;
  vec_t parameters = NULL;
  node_t return_type = NULL;
  bool is_c_variadic = false;
  cubec_expression_type_function_t node = NULL;

  /* 1. Expect 'func' keyword */
  if (!_is_keyword(tokens, current, "func")) {
    return NULL;
  }
  token_t func_token = vec_get(tokens, current);
  if (!func_token) return NULL;
  location_t start_location = *token_get_location(func_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  token_t open_paren = vec_get(tokens, current);
  if (!open_paren || !token_is(open_paren, CUBEC_TOKEN_SYMBOL, "(")) {
    /* Not a function type — could be a function expression in value context.
     * Return NULL to let other parsers handle it. */
    return NULL;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse parameter list (type-only, no names).
   * Function type parameters are just types: func(i32, f64) -> bool.
   * Function expression parameters have names: func(x: i32, y: f64): bool { ... }.
   * If we encounter a named parameter pattern (identifier followed by ':'),
   * this is a function expression, not a function type — return NULL. */
  parameters = TRY_LOCAL(onerror, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));

  if (_is_symbol(tokens, current, ")")) {
    /* no parameters */
  } else if (_is_symbol(tokens, current, "...")) {
    /* C-style variadic with no named params */
    is_c_variadic = true;
    current++;
    skip_whitespace(tokens, &current);
  } else {
    /* Parse type parameters.
     * Check for named parameter pattern: if the first token is an identifier
     * followed by ':', this is a function expression, not a function type. */
    {
      size_t lookahead = current;
      token_t first = vec_get(tokens, lookahead);
      if (first && token_get_kind(first) == CUBEC_TOKEN_IDENTIFIER) {
        lookahead++;
        skip_whitespace(tokens, &lookahead);
        token_t colon = vec_get(tokens, lookahead);
        if (colon && token_is(colon, CUBEC_TOKEN_SYMBOL, ":")) {
          /* Named parameter pattern — this is a function expression */
          allocator_free(allocator, &parameters);
          return NULL;
        }
      }
    }

    while (true) {
      node_t param = TRY_LOCAL(onerror,
                               read_expression_type(allocator, tokens, &current, filename));
      if (!param) {
        THROW_LOCAL(onerror, "expected type in function type parameter list");
      }
      vec_push(parameters, param);
      skip_whitespace(tokens, &current);

      token_t comma_or_close = vec_get(tokens, current);
      if (!comma_or_close) {
        THROW_LOCAL(onerror, "unexpected end of input in function type parameter list");
      }

      if (token_is(comma_or_close, CUBEC_TOKEN_SYMBOL, ",")) {
        current++;
        skip_whitespace(tokens, &current);
        /* Check for '...' after comma */
        if (_is_symbol(tokens, current, "...")) {
          is_c_variadic = true;
          current++;
          skip_whitespace(tokens, &current);
          break;
        }
      } else if (token_is(comma_or_close, CUBEC_TOKEN_SYMBOL, ")")) {
        break;
      } else {
        location_t *loc = token_get_location(comma_or_close);
        THROW_LOCAL(onerror,
                    "%s:%" PRIuPTR ":%" PRIuPTR " expected ',' or ')' in function type parameter list",
                    filename, loc->begin.line + 1, loc->begin.column);
      }
    }
  }

  /* 4. Expect ')' */
  token_t close_paren = vec_get(tokens, current);
  if (!close_paren || !token_is(close_paren, CUBEC_TOKEN_SYMBOL, ")")) {
    location_t *loc = close_paren ? token_get_location(close_paren) : NULL;
    if (loc) {
      THROW_LOCAL(onerror,
                  "%s:%" PRIuPTR ":%" PRIuPTR " expected ')' after function type parameters",
                  filename, loc->begin.line + 1, loc->begin.column);
    } else {
      THROW_LOCAL(onerror, "expected ')' after function type parameters");
    }
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Expect '->' (function type uses -> for return type).
   * If ':' follows instead, this is a function expression — return NULL. */
  if (_is_symbol(tokens, current, ":")) {
    /* ':' indicates function expression (func(params): type { body }) */
    allocator_free(allocator, &parameters);
    return NULL;
  }
  if (!_is_symbol(tokens, current, "->")) {
    location_t *loc = NULL;
    token_t tok = vec_get(tokens, current);
    if (tok) loc = token_get_location(tok);
    if (loc) {
      THROW_LOCAL(onerror,
                  "%s:%" PRIuPTR ":%" PRIuPTR " expected '->' in function type",
                  filename, loc->begin.line + 1, loc->begin.column);
    } else {
      THROW_LOCAL(onerror, "expected '->' in function type");
    }
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 6. Parse return type (greedy — consumes ternary/constraint).
   * func(i32) -> A ? B : C → func(i32) -> ternary(A, B, C).
   * Use grouping for the alternative: (func(i32) -> A) ? B : C. */
  return_type = TRY_LOCAL(onerror,
                          read_expression_type(allocator, tokens, &current, filename));
  if (!return_type) {
    THROW_LOCAL(onerror, "expected return type after '->' in function type");
  }

  /* 7. Build location */
  location_t loc = {
      .begin = start_location.begin,
      .end = return_type->location.end,
      .filename = filename,
  };

  /* 8. Create node */
  node = TRY_LOCAL(onerror,
                   allocator_create(allocator, &g_cubec_expression_type_function_type,
                                    &(cubec_expression_type_function_init_t){
                                        .location = loc,
                                        .parameters = parameters,
                                        .return_type = return_type,
                                        .is_c_variadic = is_c_variadic,
                                    }));

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &return_type);
  allocator_free(allocator, &parameters);
  allocator_free(allocator, &node);
  return NULL;
}
