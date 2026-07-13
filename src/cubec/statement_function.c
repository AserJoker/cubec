#include "cubec/statement_function.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "cubec/expression_function.h"
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
  self->is_builtin = init->is_builtin;
  self->is_comptime = init->is_comptime;
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
  self->is_builtin = another->is_builtin;
  self->is_comptime = another->is_comptime;
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
  self->is_builtin = another->is_builtin;
  self->is_comptime = another->is_comptime;
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
  bool is_builtin = false;
  bool is_comptime = false;
  location_t start_location = {0};
  node_t expr_node = NULL;
  cubec_statement_function_t node = NULL;

  /* 1. Parse optional modifiers: export / inline / extern / builtin / comptime */
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
    } else if (_is_keyword(tokens, current, "builtin")) {
      if (is_builtin) THROW_LOCAL(onerror, "duplicate 'builtin' modifier");
      is_builtin = true;
      if (start_location.begin.offset == 0) {
        token_t tok = TRY_LOCAL(onerror, vec_get(tokens, current));
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else if (_is_keyword(tokens, current, "comptime")) {
      if (is_comptime) THROW_LOCAL(onerror, "duplicate 'comptime' modifier");
      is_comptime = true;
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
  if (is_builtin && is_extern)
    THROW_LOCAL(onerror, "'builtin' and 'extern' are mutually exclusive");
  if (is_comptime && is_extern)
    THROW_LOCAL(onerror, "'comptime' and 'extern' are mutually exclusive");
  if (is_comptime && is_builtin)
    THROW_LOCAL(onerror, "'comptime' and 'builtin' are mutually exclusive");

  /* 3. Expect 'func' keyword */
  if (!_is_keyword(tokens, current, "func")) {
    return NULL;
  }

  /* 4. Delegate to read_expression_function for the actual func parsing */
  expr_node = TRY_LOCAL(onerror, read_expression_function(allocator, tokens, &current, filename));
  cubec_expression_function_t func = (cubec_expression_function_t)expr_node;

  /* 5. Validate: statement functions must have a name */
  if (!func->name) {
    THROW_LOCAL(onerror, "function declaration requires a name");
  }

  /* 6. Validate: statement functions cannot have captures */
  if (func->captures) {
    THROW_LOCAL(onerror, "function declaration cannot have capture list");
  }

  /* 7. Validate: C-style variadic only in extern functions */
  if (func->is_c_variadic && !is_extern) {
    THROW_LOCAL(onerror, "C-style variadic '...' is only allowed in extern functions");
  }

  /* 7b. Validate: comptime functions must have a body */
  if (is_comptime && !func->body) {
    THROW_LOCAL(onerror, "comptime func declaration requires a body");
  }

  /* 8. Build location (use modifier start or func node location) */
  location_t loc = expr_node->location;
  if (start_location.begin.offset != 0) {
    loc.begin = start_location.begin;
  }

  /* 9. Create statement_function node, transferring ownership from expression_function */
  cubec_statement_function_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .is_inline = is_inline,
      .is_extern = is_extern,
      .is_builtin = is_builtin,
      .is_comptime = is_comptime,
      .is_c_variadic = func->is_c_variadic,
      .name = func->name,
      .generic_params = func->generic_params,
      .arguments = func->arguments,
      .return_type = func->return_type,
      .body = func->body,
  };

  /* Nullify fields in expression_function to prevent double-free during dispose */
  func->name = NULL;
  func->generic_params = NULL;
  func->arguments = NULL;
  func->return_type = NULL;
  func->body = NULL;

  allocator_free(allocator, &expr_node);

  node = TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_statement_function_type, &init));
  *position = current;
  return (node_t)&node->super;

onerror:
  allocator_free(allocator, &expr_node);
  allocator_free(allocator, &node);
  return NULL;
}
