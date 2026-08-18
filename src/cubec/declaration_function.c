#include "cubec/declaration_function.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "core/token_writer.h"
#include "cubec/expression.h"
#include "cubec/function_argument.h"
#include "cubec/function_capture.h"
#include "cubec/generic_param.h"
#include "cubec/statement.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/statement_block.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void
_cubec_declaration_function_init(cubec_declaration_function_t self,
                                 allocator_t allocator,
                                 cubec_declaration_function_init_t *init) {
  if (!init)
    return;
  cubec_declaration_init_t super_init = {
      .kind = CUBEC_NODE_DECLARATION_FUNCTION,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_cubec_declaration_class.init(&self->super, allocator, &super_init);
  self->name = init->name;
  self->captures = init->captures;
  self->generic_params = init->generic_params;
  self->arguments = init->arguments;
  self->return_type = init->return_type;
  self->body = init->body;
  self->is_inline = init->is_inline;
  self->is_extern = init->is_extern;
  self->is_builtin = init->is_builtin;
  self->is_comptime = init->is_comptime;
  self->is_c_variadic = init->is_c_variadic;
}

static void
_cubec_declaration_function_dispose(cubec_declaration_function_t self,
                                    allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->return_type);
  allocator_free(allocator, &self->arguments);
  allocator_free(allocator, &self->generic_params);
  allocator_free(allocator, &self->captures);
  allocator_free(allocator, &self->name);
  g_cubec_declaration_class.dispose(&self->super, allocator);
}

static void
_cubec_declaration_function_clone(cubec_declaration_function_t self,
                                  allocator_t allocator,
                                  cubec_declaration_function_t another) {
  g_cubec_declaration_class.clone(&self->super, allocator, &another->super);
  self->name = another->name ? alloc_clone(allocator, another->name) : NULL;
  self->captures =
      another->captures ? alloc_clone(allocator, another->captures) : NULL;
  self->generic_params = another->generic_params
                             ? alloc_clone(allocator, another->generic_params)
                             : NULL;
  self->arguments = alloc_clone(allocator, another->arguments);
  self->return_type = another->return_type
                          ? alloc_clone(allocator, another->return_type)
                          : NULL;
  self->body = another->body ? alloc_clone(allocator, another->body) : NULL;
  self->is_inline = another->is_inline;
  self->is_extern = another->is_extern;
  self->is_builtin = another->is_builtin;
  self->is_comptime = another->is_comptime;
  self->is_c_variadic = another->is_c_variadic;
  return;
}

static void
_cubec_declaration_function_move(cubec_declaration_function_t self,
                                 allocator_t allocator,
                                 cubec_declaration_function_t another) {
  g_cubec_declaration_class.move(&self->super, allocator, &another->super);
  self->name = another->name ? alloc_move(allocator, another->name) : NULL;
  self->captures =
      another->captures ? alloc_move(allocator, another->captures) : NULL;
  self->generic_params = another->generic_params
                             ? alloc_move(allocator, another->generic_params)
                             : NULL;
  self->arguments = alloc_move(allocator, another->arguments);
  self->return_type =
      another->return_type ? alloc_move(allocator, another->return_type) : NULL;
  self->body = another->body ? alloc_move(allocator, another->body) : NULL;
  self->is_inline = another->is_inline;
  self->is_extern = another->is_extern;
  self->is_builtin = another->is_builtin;
  self->is_comptime = another->is_comptime;
  self->is_c_variadic = another->is_c_variadic;
  return;
}

class_t g_cubec_declaration_function_class = {
    .name = "cubec.cubec.declaration_function",
    .size = sizeof(struct _cubec_declaration_function_t),
    .init = (class_init_fn_t)_cubec_declaration_function_init,
    .dispose = (class_dispose_fn_t)_cubec_declaration_function_dispose,
    .clone = (class_clone_fn_t)_cubec_declaration_function_clone,
    .move = (class_move_fn_t)_cubec_declaration_function_move,
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
 *  Parser: read_declaration_function
 * -------------------------------------------------------------------------- */

node_t read_declaration_function(vm_t vm, vec_t tokens, size_t *position,
                                const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;
  node_t name = NULL;
  vec_t captures = NULL;
  vec_t generic_params = NULL;
  vec_t arguments = NULL;
  node_t return_type = NULL;
  node_t body = NULL;
  bool is_c_variadic = false;
  cubec_declaration_function_t node = NULL;

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
    /* Empty capture list: || (tokenized as single || due to lexer
     * longest-match) */
    /* captures remains NULL — empty captures is semantically equivalent to no
     * captures */
    current++;
    skip_whitespace(tokens, &current);
  } else if (_is_symbol(tokens, current, "|")) {
    /* Non-empty capture list */
    current++;
    skip_whitespace(tokens, &current);

    captures = allocator_create(allocator, &g_vec_class, &(vec_init_t){true});

    while (true) {
      node_t cap = read_function_capture(vm, tokens, &current, filename);
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
        goto onerror;
      }
    }

    /* After capture list, try to parse function name if present */
    token_t name_tok = vec_get(tokens, current);
    if (name_tok && token_get_kind(name_tok) == CUBEC_TOKEN_IDENTIFIER) {
      name = read_literal_identifier(vm, tokens, &current, filename);
      skip_whitespace(tokens, &current);
    }
  } else {
    /* No capture list — captures remain NULL.
       Try to parse function name (identifier) if present. */
    token_t tok = vec_get(tokens, current);
    if (tok && token_get_kind(tok) == CUBEC_TOKEN_IDENTIFIER) {
      name = read_literal_identifier(vm, tokens, &current, filename);
      skip_whitespace(tokens, &current);
    }
  }

  /* 3. Parse optional generic parameters */
  generic_params = read_generic_params(vm, tokens, &current, filename);
  if (generic_params) {
    skip_whitespace(tokens, &current);
  }

  /* 4. Expect '(' */
  token_t open_paren = vec_get(tokens, current);
  if (!open_paren || !token_is(open_paren, CUBEC_TOKEN_SYMBOL, "(")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse parameter list */
  arguments = allocator_create(allocator, &g_vec_class, &(vec_init_t){true});

  if (_is_symbol(tokens, current, ")")) {
    /* no parameters */
  } else {
    /* Parse parameters (read_function_argument handles ... prefix for pack
     * params) */
    while (true) {
      node_t arg = read_function_argument(vm, tokens, &current, filename);
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
        goto onerror;
      }
    }
  }

  /* 6. Expect ')' */
  token_t close_paren = vec_get(tokens, current);
  if (!close_paren || !token_is(close_paren, CUBEC_TOKEN_SYMBOL, ")")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 7. Parse optional return type: : type */
  if (_is_symbol(tokens, current, ":")) {
    current++;
    skip_whitespace(tokens, &current);

    return_type = read_expression_base(vm, tokens, &current, filename);
    if (node_is_error(return_type))
      goto onerror;
    if (!return_type) {
      goto onerror;
    }
    skip_whitespace(tokens, &current);
  }

  /* 8. Parse function body or semicolon */
  token_t brace_or_semi = vec_get(tokens, current);
  if (token_is(brace_or_semi, CUBEC_TOKEN_SYMBOL, "{")) {
    body = read_statement_block(vm, tokens, &current, filename);
    if (node_is_error(body))
      goto onerror;
    if (!body) {
      goto onerror;
    }
  } else if (token_is(brace_or_semi, CUBEC_TOKEN_SYMBOL, ";")) {
    if (!name) {
      goto onerror;
    }
    current++;
    body = NULL;
  } else {
    goto onerror;
  }

  /* 9. Build location */
  location_t *end_loc =
      body ? &body->location : token_get_location(brace_or_semi);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  /* 10. Create node */
  cubec_declaration_function_init_t init = {
      .location = loc,
      .parent = NULL,
      .name = name,
      .captures = captures,
      .generic_params = generic_params,
      .arguments = arguments,
      .return_type = return_type,
      .body = body,
      .is_inline = false,
      .is_extern = false,
      .is_builtin = false,
      .is_comptime = false,
      .is_c_variadic = is_c_variadic,
  };
  node = allocator_create(allocator, &g_cubec_declaration_function_class, &init);
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
  return create_error(vm, start_location);
}

/* --------------------------------------------------------------------------
 *  Factory: create_declaration_function
 * -------------------------------------------------------------------------- */

node_t create_declaration_function(vm_t vm, location_t loc, node_t name,
                                   vec_t captures, vec_t generic_params,
                                   vec_t args, node_t return_type, node_t body,
                                   bool is_inline, bool is_extern,
                                   bool is_builtin, bool is_comptime,
                                   bool is_c_variadic) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_declaration_function_init_t init = {
      .location = loc,
      .parent = NULL,
      .name = name,
      .captures = captures,
      .generic_params = generic_params,
      .arguments = args,
      .return_type = return_type,
      .body = body,
      .is_inline = is_inline,
      .is_extern = is_extern,
      .is_builtin = is_builtin,
      .is_comptime = is_comptime,
      .is_c_variadic = is_c_variadic};
  return (node_t)allocator_create(alloc, &g_cubec_declaration_function_class,
                                  &init);
}

/* --------------------------------------------------------------------------
 *  Emit: emit_declaration_function
 * -------------------------------------------------------------------------- */

void emit_declaration_function(emit_context_t vm, node_t node) {
  cubec_declaration_function_t decl = (cubec_declaration_function_t)node;
  recover_comments_to(vm, node->location.begin.offset);
  if (decl->is_inline) {
    emit_keyword(vm, "inline");
    emit_space(vm);
  }
  if (decl->is_extern) {
    emit_keyword(vm, "extern");
    emit_space(vm);
  }
  if (decl->is_builtin) {
    emit_keyword(vm, "builtin");
    emit_space(vm);
  }
  if (decl->is_comptime) {
    emit_keyword(vm, "comptime");
    emit_space(vm);
  }
  emit_keyword(vm, "func");
  if (decl->captures) {
    emit_symbol(vm, "|");
    for (size_t i = 0; i < vec_get_size(decl->captures); i++) {
      if (i != 0) {
        emit_symbol(vm, ",");
        emit_space(vm);
      }
      emit_function_capture(vm, vec_get(decl->captures, i));
    }
    emit_symbol(vm, "|");
  }
  if (decl->name) {
    emit_space(vm);
    emit_expression(vm, decl->name);
  }
  if (decl->generic_params) {
    emit_symbol(vm, "[");
    for (size_t i = 0; i < vec_get_size(decl->generic_params); i++) {
      if (i != 0) {
        emit_symbol(vm, ",");
        emit_space(vm);
      }
      emit_generic_param(vm, vec_get(decl->generic_params, i));
    }
    emit_symbol(vm, "]");
  }
  emit_symbol(vm, "(");
  for (size_t i = 0; i < vec_get_size(decl->arguments); i++) {
    if (i != 0) {
      emit_symbol(vm, ",");
      emit_space(vm);
    }
    emit_function_argument(vm, vec_get(decl->arguments, i));
  }
  if (decl->is_c_variadic) {
    if (vec_get_size(decl->arguments) > 0) {
      emit_symbol(vm, ",");
      emit_space(vm);
    }
    emit_symbol(vm, "...");
  }
  emit_symbol(vm, ")");
  if (decl->return_type) {
    emit_symbol(vm, ":");
    emit_space(vm);
    emit_expression(vm, decl->return_type);
  }
  if (decl->body) {
    emit_space(vm);
    emit_statement(vm, decl->body);
  } else {
    emit_symbol(vm, ";");
  }
}
