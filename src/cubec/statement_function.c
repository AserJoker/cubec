#include "cubec/statement_function.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "cubec/function_argument.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_function_init(
    cubec_statement_function_t self, allocator_t allocator,
    cubec_statement_function_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_FUNCTION,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->is_export = init->is_export;
  self->is_inline = init->is_inline;
  self->is_extern = init->is_extern;
  self->is_c_variadic = init->is_c_variadic;
  self->name = init->name;
  self->generic_params = init->generic_params;
  self->arguments = init->arguments;
  self->return_type = init->return_type;
  self->body = init->body;
onerror:
  return;
}

static void _cubec_statement_function_dispose(
    cubec_statement_function_t self, allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->return_type);
  allocator_free(allocator, &self->arguments);
  allocator_free(allocator, &self->generic_params);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_function_clone(
    cubec_statement_function_t self, allocator_t allocator,
    cubec_statement_function_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->is_export = another->is_export;
  self->is_inline = another->is_inline;
  self->is_extern = another->is_extern;
  self->is_c_variadic = another->is_c_variadic;
  self->name = TRY_LOCAL(onerror, value_clone(allocator, another->name));
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

static void _cubec_statement_function_move(
    cubec_statement_function_t self, allocator_t allocator,
    cubec_statement_function_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->is_export = another->is_export;
  self->is_inline = another->is_inline;
  self->is_extern = another->is_extern;
  self->is_c_variadic = another->is_c_variadic;
  self->name = TRY_LOCAL(onerror, value_move(allocator, another->name));
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

type_t g_cubec_statement_function_type = {
    .name = "cubec.cubec.statement_function",
    .size = sizeof(struct _cubec_statement_function_t),
    .init = (type_init_fn_t)_cubec_statement_function_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_function_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_function_clone,
    .move = (type_move_fn_t)_cubec_statement_function_move,
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
 *  Parser: read_statement_function
 * -------------------------------------------------------------------------- */

node_t read_statement_function(allocator_t allocator, vec_t tokens,
                                size_t *position, const char *filename) {
  size_t current = *position;
  bool is_export = false;
  bool is_inline = false;
  bool is_extern = false;
  bool is_c_variadic = false;
  node_t name = NULL;
  vec_t generic_params = NULL;
  vec_t arguments = NULL;
  node_t return_type = NULL;
  node_t body = NULL;
  location_t start_location = {0};
  cubec_statement_function_t node = NULL;

  /* 1. Parse optional modifiers: export / inline / extern */
  while (true) {
    if (_is_keyword(tokens, current, "export")) {
      if (is_export) THROW_LOCAL(onerror, "duplicate 'export' modifier");
      is_export = true;
      if (start_location.begin.offset == 0) {
        token_t tok = TRY_LOCAL(onerror, vec_get(tokens, current));
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else if (_is_keyword(tokens, current, "inline")) {
      if (is_inline) THROW_LOCAL(onerror, "duplicate 'inline' modifier");
      is_inline = true;
      if (start_location.begin.offset == 0) {
        token_t tok = TRY_LOCAL(onerror, vec_get(tokens, current));
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else if (_is_keyword(tokens, current, "extern")) {
      if (is_extern) THROW_LOCAL(onerror, "duplicate 'extern' modifier");
      is_extern = true;
      if (start_location.begin.offset == 0) {
        token_t tok = TRY_LOCAL(onerror, vec_get(tokens, current));
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else {
      break;
    }
  }

  /* 2. Mutually exclusive check */
  if (is_export && is_extern)
    THROW_LOCAL(onerror, "'export' and 'extern' are mutually exclusive");
  if (is_inline && is_extern)
    THROW_LOCAL(onerror, "'inline' and 'extern' are mutually exclusive");

  /* 3. Expect 'func' keyword */
  if (!_is_keyword(tokens, current, "func")) {
    return NULL;
  }
  token_t func_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (start_location.begin.offset == 0) {
    start_location = *token_get_location(func_token);
    start_location.filename = filename;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 4. Parse function name (required) */
  name = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
  if (!name) {
    THROW_LOCAL(cleanup, "expected function name after 'func'");
  }
  skip_whitespace(tokens, &current);

  /* 5. Parse optional generic parameters */
  generic_params = TRY_LOCAL(cleanup, read_generic_params(allocator, tokens, &current, filename));
  if (generic_params) {
    skip_whitespace(tokens, &current);
  }

  /* 6. Expect '(' */
  token_t open_paren = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!open_paren || !token_is(open_paren, CUBEC_TOKEN_SYMBOL, "(")) {
    location_t *loc = token_get_location(open_paren);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected '(' after function name",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 7. Parse parameter list */
  arguments = TRY_LOCAL(cleanup, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));

  /* Check for empty parameter list */
  if (_is_symbol(tokens, current, ")")) {
    /* no parameters */
  } else if (_is_symbol(tokens, current, "...")) {
    /* C-style variadic with no named params */
    is_c_variadic = true;
    current++;
    skip_whitespace(tokens, &current);
  } else {
    /* Parse parameters */
    while (true) {
      node_t arg = TRY_LOCAL(cleanup, read_function_argument(allocator, tokens, &current, filename));
      if (!arg) {
        THROW_LOCAL(cleanup, "expected function parameter");
      }
      vec_push(arguments, arg);

      skip_whitespace(tokens, &current);

      /* Check for comma or ')' */
      token_t comma_or_close = TRY_LOCAL(cleanup, vec_get(tokens, current));
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
        /* Otherwise continue parsing next parameter */
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

  /* 8. Expect ')' */
  token_t close_paren = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!close_paren || !token_is(close_paren, CUBEC_TOKEN_SYMBOL, ")")) {
    location_t *loc = token_get_location(close_paren);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected ')' after parameter list",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 9. Parse optional return type: -> type */
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

  /* 10. Parse function body or semicolon */
  token_t brace_or_semi = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (token_is(brace_or_semi, CUBEC_TOKEN_SYMBOL, "{")) {
    body = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
    if (!body) {
      THROW_LOCAL(cleanup, "expected function body");
    }
  } else if (token_is(brace_or_semi, CUBEC_TOKEN_SYMBOL, ";")) {
    current++;
    body = NULL;
  } else {
    location_t *loc = token_get_location(brace_or_semi);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected function body or ';'",
                filename, loc->begin.line + 1, loc->begin.column);
  }

  /* 11. Validate: C-style variadic only in extern functions */
  if (is_c_variadic && !is_extern) {
    THROW_LOCAL(cleanup, "C-style variadic '...' is only allowed in extern functions");
  }

  /* 12. Build location */
  location_t *end_loc = body ? &body->location : token_get_location(brace_or_semi);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  /* 13. Create node */
  cubec_statement_function_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .is_inline = is_inline,
      .is_extern = is_extern,
      .is_c_variadic = is_c_variadic,
      .name = name,
      .generic_params = generic_params,
      .arguments = arguments,
      .return_type = return_type,
      .body = body,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_function_type, &init));
  *position = current;
  return (node_t)&node->super;

cleanup:
  allocator_free(allocator, &body);
  allocator_free(allocator, &return_type);
  allocator_free(allocator, &arguments);
  allocator_free(allocator, &generic_params);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &body);
  allocator_free(allocator, &return_type);
  allocator_free(allocator, &arguments);
  allocator_free(allocator, &generic_params);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return NULL;
}
