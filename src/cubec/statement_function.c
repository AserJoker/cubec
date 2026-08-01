#include "cubec/statement_function.h"
#include "core/token.h"
#include "cubec/decorator.h"
#include "cubec/expression_function.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void
_cubec_statement_function_init(cubec_statement_function_t self,
                               allocator_t allocator,
                               cubec_statement_function_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_FUNCTION,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->is_export = init->is_export;
  self->is_exportlib = init->is_exportlib;
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
  self->decorators = init->decorators;
  self->captures = init->captures;
}

static void _cubec_statement_function_dispose(cubec_statement_function_t self,
                                              allocator_t allocator) {
  allocator_free(allocator, &self->captures);
  allocator_free(allocator, &self->decorators);
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->return_type);
  allocator_free(allocator, &self->arguments);
  allocator_free(allocator, &self->generic_params);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void
_cubec_statement_function_clone(cubec_statement_function_t self,
                                allocator_t allocator,
                                cubec_statement_function_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->is_exportlib = another->is_exportlib;
  self->is_inline = another->is_inline;
  self->is_extern = another->is_extern;
  self->is_builtin = another->is_builtin;
  self->is_comptime = another->is_comptime;
  self->is_c_variadic = another->is_c_variadic;
  self->name = value_clone(allocator, another->name);
  self->generic_params = another->generic_params
                             ? value_clone(allocator, another->generic_params)
                             : NULL;
  self->arguments = value_clone(allocator, another->arguments);
  self->return_type = another->return_type
                          ? value_clone(allocator, another->return_type)
                          : NULL;
  self->body = another->body ? value_clone(allocator, another->body) : NULL;
  self->captures =
      another->captures ? value_clone(allocator, another->captures) : NULL;
  return;
}

static void _cubec_statement_function_move(cubec_statement_function_t self,
                                           allocator_t allocator,
                                           cubec_statement_function_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->is_exportlib = another->is_exportlib;
  self->is_inline = another->is_inline;
  self->is_extern = another->is_extern;
  self->is_builtin = another->is_builtin;
  self->is_comptime = another->is_comptime;
  self->is_c_variadic = another->is_c_variadic;
  self->name = value_move(allocator, another->name);
  self->generic_params = another->generic_params
                             ? value_move(allocator, another->generic_params)
                             : NULL;
  self->arguments = value_move(allocator, another->arguments);
  self->return_type =
      another->return_type ? value_move(allocator, another->return_type) : NULL;
  self->body = another->body ? value_move(allocator, another->body) : NULL;
  self->captures =
      another->captures ? value_move(allocator, another->captures) : NULL;
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
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

/* --------------------------------------------------------------------------
 *  Parser: read_statement_function
 * -------------------------------------------------------------------------- */

node_t read_statement_function(context_t ctx, vec_t tokens, size_t *position,
                               const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  bool is_export = false;
  bool is_inline = false;
  bool is_extern = false;
  bool is_builtin = false;
  bool is_comptime = false;
  location_t start_location = {0};
  node_t expr_node = NULL;
  cubec_statement_function_t node = NULL;
  vec_t decorators = NULL;

  /* Collect decorators [[...]] */
  {
    while (true) {
      skip_whitespace(tokens, &current);
      node_t dec = read_decorator(ctx, tokens, &current, filename);
      if (node_is_error(dec))
        return dec;
      if (!dec)
        break;
      if (!decorators) {
        decorators =
            allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
      }
      vec_push(decorators, dec);
    }
  }

  /* 1. Parse optional modifiers: export / exportlib / inline / extern / builtin
   * / comptime */
  bool is_exportlib = false;
  while (true) {
    if (_is_keyword(tokens, current, "export")) {
      if (is_export)
        goto onerror;
      is_export = true;
      if (start_location.begin.offset == 0) {
        token_t tok = vec_get(tokens, current);
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else if (_is_keyword(tokens, current, "exportlib")) {
      if (is_exportlib)
        goto onerror;
      is_exportlib = true;
      if (start_location.begin.offset == 0) {
        token_t tok = vec_get(tokens, current);
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else if (_is_keyword(tokens, current, "inline")) {
      if (is_inline)
        goto onerror;
      is_inline = true;
      if (start_location.begin.offset == 0) {
        token_t tok = vec_get(tokens, current);
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else if (_is_keyword(tokens, current, "extern")) {
      if (is_extern)
        goto onerror;
      is_extern = true;
      if (start_location.begin.offset == 0) {
        token_t tok = vec_get(tokens, current);
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else if (_is_keyword(tokens, current, "builtin")) {
      if (is_builtin)
        goto onerror;
      is_builtin = true;
      if (start_location.begin.offset == 0) {
        token_t tok = vec_get(tokens, current);
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else if (_is_keyword(tokens, current, "comptime")) {
      if (is_comptime)
        goto onerror;
      is_comptime = true;
      if (start_location.begin.offset == 0) {
        token_t tok = vec_get(tokens, current);
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
  if (is_export && is_exportlib)
    goto onerror;
  if (is_export && is_extern)
    goto onerror;
  if (is_exportlib && is_extern)
    goto onerror;
  if (is_exportlib && is_builtin)
    goto onerror;
  if (is_exportlib && is_comptime)
    goto onerror;
  if (is_exportlib && is_inline)
    goto onerror;
  if (is_inline && is_extern)
    goto onerror;
  if (is_inline && is_builtin)
    goto onerror;
  if (is_builtin && is_extern)
    goto onerror;
  if (is_comptime && is_extern)
    goto onerror;
  if (is_comptime && is_builtin)
    goto onerror;
  /* inline + comptime: comptime takes precedence; ignore inline silently */

  /* 3. Expect 'func' keyword */
  if (!_is_keyword(tokens, current, "func")) {
    goto onerror;
  }

  /* 4. Delegate to read_expression_function for the actual func parsing */
  expr_node = read_expression_function(ctx, tokens, &current, filename);
  if (node_is_error(expr_node)) {
    allocator_free(allocator, &decorators);
    return expr_node;
  }
  if (!expr_node)
    goto onerror;
  cubec_expression_function_t func = (cubec_expression_function_t)expr_node;

  /* 5. Validate: statement functions must have a name */
  if (!func->name) {
    goto onerror;
  }

  /* 7. Validate: C-style variadic only in extern functions */
  if (func->is_c_variadic && !is_extern) {
    goto onerror;
  }

  /* 7b. Validate: comptime functions must have a body */
  if (is_comptime && !func->body) {
    goto onerror;
  }

  /* 8. Build location (use modifier start or func node location) */
  location_t loc = expr_node->location;
  if (start_location.begin.offset != 0) {
    loc.begin = start_location.begin;
  }

  /* 9. Create statement_function node, transferring ownership from
   * expression_function */
  cubec_statement_function_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .is_exportlib = is_exportlib,
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
      .decorators = decorators,
      .captures = func->captures,
  };

  /* Nullify fields in expression_function to prevent double-free during dispose
   */
  func->name = NULL;
  func->generic_params = NULL;
  func->arguments = NULL;
  func->return_type = NULL;
  func->body = NULL;
  func->captures = NULL;

  allocator_free(allocator, &expr_node);

  node = allocator_create(allocator, &g_cubec_statement_function_type, &init);
  *position = current;
  return (node_t)&node->super;

onerror:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &expr_node);
  allocator_free(allocator, &node);
  return cubec_ast_create_error(ctx, start_location);
}

node_t cubec_ast_create_func_stmt(context_t ctx, location_t loc,
                                  const char *name, vec_t args,
                                  node_t return_type, node_t body,
                                  bool is_export, bool is_inline,
                                  bool is_extern, bool is_builtin,
                                  bool is_comptime, bool is_c_variadic) {
  allocator_t alloc = ctx->allocator;
  node_t name_node = cubec_ast_create_identifier(ctx, loc, name);
  cubec_statement_function_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .is_inline = is_inline,
      .is_extern = is_extern,
      .is_builtin = is_builtin,
      .is_comptime = is_comptime,
      .is_c_variadic = is_c_variadic,
      .name = name_node,
      .generic_params = NULL,
      .arguments = args,
      .return_type = return_type,
      .body = body,
  };
  return (node_t)allocator_create(alloc, &g_cubec_statement_function_type,
                                  &init);
}
