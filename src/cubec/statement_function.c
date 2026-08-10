#include "cubec/statement_function.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/declaration_function.h"
#include "cubec/decorator.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/statement.h"
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
  self->declarator = init->declarator;
  self->decorators = init->decorators;
}

static void _cubec_statement_function_dispose(cubec_statement_function_t self,
                                              allocator_t allocator) {
  allocator_free(allocator, &self->decorators);
  allocator_free(allocator, &self->declarator);
  g_node_type.dispose(&self->super, allocator);
}

static void
_cubec_statement_function_clone(cubec_statement_function_t self,
                                allocator_t allocator,
                                cubec_statement_function_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->is_exportlib = another->is_exportlib;
  self->declarator = another->declarator ? alloc_clone(allocator, another->declarator) : NULL;
  self->decorators =
      another->decorators ? alloc_clone(allocator, another->decorators) : NULL;
  return;
}

static void _cubec_statement_function_move(cubec_statement_function_t self,
                                           allocator_t allocator,
                                           cubec_statement_function_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->is_exportlib = another->is_exportlib;
  self->declarator = another->declarator ? alloc_move(allocator, another->declarator) : NULL;
  self->decorators =
      another->decorators ? alloc_move(allocator, another->decorators) : NULL;
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
  node_t decl_node = NULL;
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

  /* 3. Delegate to read_declaration_function for the actual func parsing */
  decl_node = read_declaration_function(ctx, tokens, &current, filename);
  if (node_is_error(decl_node)) {
    allocator_free(allocator, &decorators);
    return decl_node;
  }
  if (!decl_node)
    goto onerror;

  cubec_declaration_function_t decl = (cubec_declaration_function_t)decl_node;

  /* 4. Transfer modifiers from statement-level into the declaration */
  decl->is_inline = is_inline;
  decl->is_extern = is_extern;
  decl->is_builtin = is_builtin;
  decl->is_comptime = is_comptime;

  /* 5. Validate: statement functions must have a name */
  if (!decl->name) {
    goto onerror;
  }

  /* 6. Validate: C-style variadic only in extern functions */
  if (decl->is_c_variadic && !is_extern) {
    goto onerror;
  }

  /* 7. Validate: comptime functions must have a body */
  if (is_comptime && !decl->body) {
    goto onerror;
  }

  /* 8. Build location (use modifier start or func node location) */
  location_t loc = decl_node->location;
  if (start_location.begin.offset != 0) {
    loc.begin = start_location.begin;
  }

  /* 9. Create statement_function node */
  cubec_statement_function_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .is_exportlib = is_exportlib,
      .declarator = decl_node,
      .decorators = decorators,
  };

  node = allocator_create(allocator, &g_cubec_statement_function_type, &init);
  *position = current;
  return (node_t)&node->super;

onerror:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &decl_node);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

node_t create_statement_func(context_t ctx, location_t loc, const char *name,
                             vec_t args, node_t return_type, node_t body,
                             bool is_export, bool is_inline, bool is_extern,
                             bool is_builtin, bool is_comptime,
                             bool is_c_variadic, vec_t decorators) {
  allocator_t alloc = ctx->allocator;
  node_t name_node = create_literal_identifier(ctx, loc, name);
  node_t decl = create_declaration_function(ctx, loc, name_node, NULL, NULL,
                                            args, return_type, body, is_inline,
                                            is_extern, is_builtin, is_comptime,
                                            is_c_variadic);
  cubec_statement_function_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .is_exportlib = false,
      .declarator = decl,
      .decorators = decorators,
  };
  return (node_t)allocator_create(alloc, &g_cubec_statement_function_type,
                                  &init);
}

void emit_statement_function(emit_context_t ctx, node_t node) {
  cubec_statement_function_t stmt = (cubec_statement_function_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  if (stmt->decorators) {
    for (size_t i = 0; i < vec_get_size(stmt->decorators); i++) {
      recover_comments_to(ctx, ((node_t)vec_get(stmt->decorators, i))->location.begin.offset);
      emit_decorator(ctx, vec_get(stmt->decorators, i));
      emit_newline(ctx);
    }
  }
  if (stmt->is_export) {
    emit_keyword(ctx, "export");
    emit_space(ctx);
  }
  if (stmt->is_exportlib) {
    emit_keyword(ctx, "exportlib");
    emit_space(ctx);
  }
  /* Delegate the rest to declaration_function emit */
  emit_declaration_function(ctx, stmt->declarator);
}
