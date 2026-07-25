#include "cubec/expression_function.h"
#include "core/allocator.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/ast_factory.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/expression.h"
#include "cubec/function_argument.h"
#include "cubec/function_capture.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/token.h"
#include <inttypes.h>
#include "engine/context.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_function_init(
    cubec_expression_function_t self, allocator_t allocator,
    cubec_expression_function_init_t *init) {
  if (!init) return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_FUNCTION,
      .location = init->location,
      .parent = NULL,
  };
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->name = init->name;
  self->captures = init->captures;
  self->generic_params = init->generic_params;
  self->arguments = init->arguments;
  self->return_type = init->return_type;
  self->body = init->body;
  self->is_c_variadic = init->is_c_variadic;
}

static void _cubec_expression_function_dispose(
    cubec_expression_function_t self, allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->return_type);
  allocator_free(allocator, &self->arguments);
  allocator_free(allocator, &self->generic_params);
  allocator_free(allocator, &self->captures);
  allocator_free(allocator, &self->name);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_function_clone(
    cubec_expression_function_t self, allocator_t allocator,
    cubec_expression_function_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->name = another->name
                   ? value_clone(allocator, another->name)
                   : NULL;
  self->captures = another->captures
                       ? value_clone(allocator, another->captures)
                       : NULL;
  self->generic_params = another->generic_params
                             ? value_clone(allocator, another->generic_params)
                             : NULL;
  self->arguments = value_clone(allocator, another->arguments);
  self->return_type = another->return_type
                          ? value_clone(allocator, another->return_type)
                          : NULL;
  self->body = another->body
                   ? value_clone(allocator, another->body)
                   : NULL;
  self->is_c_variadic = another->is_c_variadic;
  return;
}

static void _cubec_expression_function_move(
    cubec_expression_function_t self, allocator_t allocator,
    cubec_expression_function_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->name = another->name
                   ? value_move(allocator, another->name)
                   : NULL;
  self->captures = another->captures
                       ? value_move(allocator, another->captures)
                       : NULL;
  self->generic_params = another->generic_params
                             ? value_move(allocator, another->generic_params)
                             : NULL;
  self->arguments = value_move(allocator, another->arguments);
  self->return_type = another->return_type
                          ? value_move(allocator, another->return_type)
                          : NULL;
  self->body = another->body
                   ? value_move(allocator, another->body)
                   : NULL;
  self->is_c_variadic = another->is_c_variadic;
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

node_t read_expression_function(context_t ctx, vec_t tokens,
                                 size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t name = NULL;
  vec_t captures = NULL;
  vec_t generic_params = NULL;
  vec_t arguments = NULL;
  node_t return_type = NULL;
  node_t body = NULL;
  bool is_c_variadic = false;
  cubec_expression_function_t node = NULL;

  /* 1. Expect 'func' keyword */
  if (!_is_keyword(tokens, current, "func")) {
    return NULL;
  }
  token_t func_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(func_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse name or capture list after 'func' */
  if (_is_symbol(tokens, current, "||")) {
    /* Empty capture list: || (tokenized as single || due to lexer longest-match) */
    /* captures remains NULL — empty captures is semantically equivalent to no captures */
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

    /* After capture list, try to parse function name if present */
    token_t name_tok = vec_get(tokens, current);
    if (name_tok && token_get_kind(name_tok) == CUBEC_TOKEN_IDENTIFIER) {
      name = read_literal_identifier(ctx, tokens, &current, filename);
      skip_whitespace(tokens, &current);
    }
  } else {
    /* No capture list — captures remain NULL.
       Try to parse function name (identifier) if present. */
    token_t tok = vec_get(tokens, current);
    if (tok && token_get_kind(tok) == CUBEC_TOKEN_IDENTIFIER) {
      name = read_literal_identifier(ctx, tokens, &current, filename);
      skip_whitespace(tokens, &current);
    }
  }

  /* 3. Parse optional generic parameters */
  generic_params = read_generic_params(ctx, tokens, &current, filename);
  if (generic_params) {
    skip_whitespace(tokens, &current);
  }

  /* 4. Expect '(' */
  token_t open_paren = vec_get(tokens, current);
  if (!open_paren || !token_is(open_paren, CUBEC_TOKEN_SYMBOL, "(")) {
    location_t *loc = token_get_location(open_paren);
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse parameter list */
  arguments = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});

  if (_is_symbol(tokens, current, ")")) {
    /* no parameters */
  } else {
    /* Parse parameters (read_function_argument handles ... prefix for pack params) */
    while (true) {
      node_t arg = read_function_argument(ctx, tokens, &current, filename);
      if (!arg) {
        /* Check for C-style variadic with no named params: func(...)  */
        if (_is_symbol(tokens, current, "...")) {
          is_c_variadic = true;
          current++;
          skip_whitespace(tokens, &current);
          break;
        }
        goto onerror;
      }
      vec_push(arguments, arg);

      skip_whitespace(tokens, &current);

      token_t comma_or_close = vec_get(tokens, current);
      if (token_is(comma_or_close, CUBEC_TOKEN_SYMBOL, ",")) {
        current++;
        skip_whitespace(tokens, &current);
      } else if (token_is(comma_or_close, CUBEC_TOKEN_SYMBOL, ")")) {
        break;
      } else {
        location_t *loc = token_get_location(comma_or_close);
        goto onerror;
      }
    }
  }

  /* 6. Expect ')' */
  token_t close_paren = vec_get(tokens, current);
  if (!close_paren || !token_is(close_paren, CUBEC_TOKEN_SYMBOL, ")")) {
    location_t *loc = token_get_location(close_paren);
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 7. Parse optional return type: : type */
  if (_is_symbol(tokens, current, ":")) {
    current++;
    skip_whitespace(tokens, &current);

    return_type = read_expression_base(ctx, tokens, &current, filename);
    if (!return_type) {
      goto onerror;
    }
    skip_whitespace(tokens, &current);
  }

  /* 8. Parse function body or semicolon */
  token_t brace_or_semi = vec_get(tokens, current);
  if (token_is(brace_or_semi, CUBEC_TOKEN_SYMBOL, "{")) {
    body = read_statement_block(ctx, tokens, &current, filename);
    if (!body) {
      goto onerror;
    }
  } else if (token_is(brace_or_semi, CUBEC_TOKEN_SYMBOL, ";")) {
    if (!name) {
      location_t *loc = token_get_location(brace_or_semi);
      goto onerror;
    }
    current++;
    body = NULL;
  } else {
    location_t *loc = token_get_location(brace_or_semi);
    if (name) {
      goto onerror;
    } else {
      goto onerror;
    }
  }

  /* 9. Build location */
  location_t *end_loc = body ? &body->location : token_get_location(brace_or_semi);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  /* 10. Create node */
  cubec_expression_function_init_t init = {
      .location = loc,
      .parent = NULL,
      .name = name,
      .captures = captures,
      .generic_params = generic_params,
      .arguments = arguments,
      .return_type = return_type,
      .body = body,
      .is_c_variadic = is_c_variadic,
  };
  node = allocator_create(allocator, &g_cubec_expression_function_type, &init);
  *position = current;
  return (node_t)&node->super;

onerror:
  allocator_free(allocator, &body);
  allocator_free(allocator, &return_type);
  allocator_free(allocator, &arguments);
  allocator_free(allocator, &generic_params);
  allocator_free(allocator, &captures);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_function_expr
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_function_expr(context_t ctx, location_t loc,
                                      node_t name, vec_t captures,
                                      vec_t generic_params, vec_t args,
                                      node_t return_type, node_t body,
                                      bool is_c_variadic) {
  allocator_t alloc = ctx->allocator;
      cubec_expression_function_init_t init = {
      .location = loc, .parent = NULL, .name = name,
      .captures = captures, .generic_params = generic_params,
      .arguments = args, .return_type = return_type, .body = body,
      .is_c_variadic = is_c_variadic};
  return (node_t)allocator_create(alloc, &g_cubec_expression_function_type,
                                  &init);
}
