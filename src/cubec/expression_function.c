#include "cubec/expression_function.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "cubec/function_argument.h"
#include "cubec/function_capture.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_function_init(
    cubec_expression_function_t self, allocator_t allocator,
    cubec_expression_function_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_FUNCTION,
      .location = init->location,
      .parent = NULL,
  };
  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.init(&self->super, allocator, &super_init));
  self->captures = init->captures;
  self->generic_params = init->generic_params;
  self->arguments = init->arguments;
  self->return_type = init->return_type;
  self->body = init->body;
onerror:
  return;
}

static void _cubec_expression_function_dispose(
    cubec_expression_function_t self, allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->return_type);
  allocator_free(allocator, &self->arguments);
  allocator_free(allocator, &self->generic_params);
  allocator_free(allocator, &self->captures);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_function_clone(
    cubec_expression_function_t self, allocator_t allocator,
    cubec_expression_function_t another) {
  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.clone(&self->super, allocator, &another->super));
  self->captures = another->captures
                       ? TRY_LOCAL(onerror, value_clone(allocator, another->captures))
                       : NULL;
  self->generic_params = another->generic_params
                             ? TRY_LOCAL(onerror, value_clone(allocator, another->generic_params))
                             : NULL;
  self->arguments = TRY_LOCAL(onerror, value_clone(allocator, another->arguments));
  self->return_type = another->return_type
                          ? TRY_LOCAL(onerror, value_clone(allocator, another->return_type))
                          : NULL;
  self->body = another->body
                   ? TRY_LOCAL(onerror, value_clone(allocator, another->body))
                   : NULL;
  return;
onerror:
  return;
}

static void _cubec_expression_function_move(
    cubec_expression_function_t self, allocator_t allocator,
    cubec_expression_function_t another) {
  TRY_VOID_LOCAL(onerror, g_cubec_expression_type.move(&self->super, allocator, &another->super));
  self->captures = another->captures
                       ? TRY_LOCAL(onerror, value_move(allocator, another->captures))
                       : NULL;
  self->generic_params = another->generic_params
                             ? TRY_LOCAL(onerror, value_move(allocator, another->generic_params))
                             : NULL;
  self->arguments = TRY_LOCAL(onerror, value_move(allocator, another->arguments));
  self->return_type = another->return_type
                          ? TRY_LOCAL(onerror, value_move(allocator, another->return_type))
                          : NULL;
  self->body = another->body
                   ? TRY_LOCAL(onerror, value_move(allocator, another->body))
                   : NULL;
  return;
onerror:
  return;
}

type_t g_cubec_expression_function_type = {
    .name = "cubec.cubec.expression_function",
    .size = sizeof(struct _cubec_expression_function_t),
    .init = (type_init_fn_t)_cubec_expression_function_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_function_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_function_clone,
    .move = (type_move_fn_t)_cubec_expression_function_move,
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
 *  Parser: read_expression_function
 * -------------------------------------------------------------------------- */

node_t read_expression_function(allocator_t allocator, vec_t tokens,
                                 size_t *position, const char *filename) {
  size_t current = *position;
  vec_t captures = NULL;
  vec_t generic_params = NULL;
  vec_t arguments = NULL;
  node_t return_type = NULL;
  node_t body = NULL;
  cubec_expression_function_t node = NULL;

  /* 1. Expect 'func' keyword */
  if (!_is_keyword(tokens, current, "func")) {
    return NULL;
  }
  token_t func_token = TRY_LOCAL(cleanup, vec_get(tokens, current));
  location_t start_location = *token_get_location(func_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse optional capture list: '||' (empty) or '|...|' (non-empty) or none */
  if (_is_symbol(tokens, current, "||")) {
    /* Empty capture list: || (tokenized as single || due to lexer longest-match) */
    current++;
    skip_whitespace(tokens, &current);
  } else if (_is_symbol(tokens, current, "|")) {
    current++;
    skip_whitespace(tokens, &current);

    /* Non-empty capture list */
    captures = TRY_LOCAL(cleanup, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));

    while (true) {
      node_t cap = TRY_LOCAL(cleanup, read_function_capture(allocator, tokens, &current, filename));
      if (!cap) {
        THROW_LOCAL(cleanup, "expected capture name in capture list");
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
        token_t tok = TRY_LOCAL(cleanup, vec_get(tokens, current));
        location_t *loc = token_get_location(tok);
        THROW_LOCAL(cleanup,
                    "%s:%" PRIuPTR ":%" PRIuPTR " expected ',' or '|' in capture list",
                    filename, loc->begin.line + 1, loc->begin.column);
      }
    }
  }
  /* else: no capture list at all — captures remains NULL */

  /* 4. Parse optional generic parameters */
  generic_params = TRY_LOCAL(cleanup, read_generic_params(allocator, tokens, &current, filename));
  if (generic_params) {
    skip_whitespace(tokens, &current);
  }

  /* 5. Expect '(' */
  token_t open_paren = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!open_paren || !token_is(open_paren, CUBEC_TOKEN_SYMBOL, "(")) {
    location_t *loc = token_get_location(open_paren);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected '(' in anonymous function",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 6. Parse parameter list (no C-style variadic for anonymous funcs) */
  arguments = TRY_LOCAL(cleanup, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));

  if (!_is_symbol(tokens, current, ")")) {
    while (true) {
      node_t arg = TRY_LOCAL(cleanup, read_function_argument(allocator, tokens, &current, filename));
      if (!arg) {
        THROW_LOCAL(cleanup, "expected function parameter");
      }
      vec_push(arguments, arg);

      skip_whitespace(tokens, &current);

      token_t comma_or_close = TRY_LOCAL(cleanup, vec_get(tokens, current));
      if (token_is(comma_or_close, CUBEC_TOKEN_SYMBOL, ",")) {
        current++;
        skip_whitespace(tokens, &current);
      } else if (token_is(comma_or_close, CUBEC_TOKEN_SYMBOL, ")")) {
        break;
      } else {
        location_t *loc = token_get_location(comma_or_close);
        THROW_LOCAL(cleanup,
                    "%s:%" PRIuPTR ":%" PRIuPTR " expected ',' or ')' in parameter list",
                    filename, loc->begin.line + 1, loc->begin.column);
      }
    }
  }

  /* 7. Expect ')' */
  token_t close_paren = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!close_paren || !token_is(close_paren, CUBEC_TOKEN_SYMBOL, ")")) {
    location_t *loc = token_get_location(close_paren);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected ')' after parameter list",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 8. Parse optional return type: -> type */
  if (_is_symbol(tokens, current, "-")) {
    current++;
    skip_whitespace(tokens, &current);
    token_t gt = TRY_LOCAL(cleanup, vec_get(tokens, current));
    if (!gt || !token_is(gt, CUBEC_TOKEN_SYMBOL, ">")) {
      location_t *loc = token_get_location(gt);
      THROW_LOCAL(cleanup,
                  "%s:%" PRIuPTR ":%" PRIuPTR " expected '>' after '-' in return type annotation",
                  filename, loc->begin.line + 1, loc->begin.column);
    }
    current++;
    skip_whitespace(tokens, &current);

    return_type = TRY_LOCAL(cleanup, read_expression_type(allocator, tokens, &current, filename));
    if (!return_type) {
      THROW_LOCAL(cleanup, "expected return type after '->'");
    }
    skip_whitespace(tokens, &current);
  }

  /* 9. Expect '{' — anonymous functions must have a body */
  token_t brace = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!brace || !token_is(brace, CUBEC_TOKEN_SYMBOL, "{")) {
    location_t *loc = token_get_location(brace);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected function body '{' in anonymous function",
                filename, loc->begin.line + 1, loc->begin.column);
  }

  body = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
  if (!body) {
    THROW_LOCAL(cleanup, "expected function body");
  }

  /* 10. Build location */
  location_t *end_loc = &body->location;
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  /* 11. Create node */
  cubec_expression_function_init_t init = {
      .location = loc,
      .parent = NULL,
      .captures = captures,
      .generic_params = generic_params,
      .arguments = arguments,
      .return_type = return_type,
      .body = body,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_expression_function_type, &init));
  *position = current;
  return (node_t)&node->super;

cleanup:
  allocator_free(allocator, &body);
  allocator_free(allocator, &return_type);
  allocator_free(allocator, &arguments);
  allocator_free(allocator, &generic_params);
  allocator_free(allocator, &captures);
  allocator_free(allocator, &node);
  return NULL;
}
