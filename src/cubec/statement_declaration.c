#include "cubec/statement_declaration.h"
#include "core/token.h"
#include "cubec/declaration_variable.h"
#include "cubec/decorator.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

static void
_cubec_statement_declaration_init(cubec_statement_declaration_t self,
                                  allocator_t allocator,
                                  cubec_statement_declaration_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_DECLARATION,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->is_export = init->is_export;
  self->is_exportlib = init->is_exportlib;
  self->is_extern = init->is_extern;
  self->is_builtin = init->is_builtin;
  self->is_comptime = init->is_comptime;
  self->is_using = init->is_using;
  self->declarator = init->declarator;
  self->decorators = init->decorators;
}

static void
_cubec_statement_declaration_dispose(cubec_statement_declaration_t self,
                                     allocator_t allocator) {
  allocator_free(allocator, &self->decorators);
  allocator_free(allocator, &self->declarator);
  g_node_type.dispose(&self->super, allocator);
}

static void
_cubec_statement_declaration_clone(cubec_statement_declaration_t self,
                                   allocator_t allocator,
                                   cubec_statement_declaration_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->is_exportlib = another->is_exportlib;
  self->is_extern = another->is_extern;
  self->is_builtin = another->is_builtin;
  self->is_comptime = another->is_comptime;
  self->is_using = another->is_using;
  self->declarator = value_clone(allocator, another->declarator);
  return;
}

static void
_cubec_statement_declaration_move(cubec_statement_declaration_t self,
                                  allocator_t allocator,
                                  cubec_statement_declaration_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->is_exportlib = another->is_exportlib;
  self->is_extern = another->is_extern;
  self->is_builtin = another->is_builtin;
  self->is_comptime = another->is_comptime;
  self->is_using = another->is_using;
  self->declarator = value_move(allocator, another->declarator);
  return;
}

type_t g_cubec_statement_declaration_type = {
    .name = "cubec.cubec.statement_declaration",
    .size = sizeof(struct _cubec_statement_declaration_t),
    .init = (type_init_fn_t)_cubec_statement_declaration_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_declaration_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_declaration_clone,
    .move = (type_move_fn_t)_cubec_statement_declaration_move,
};

/**
 * @brief Check if a token is a specific keyword.
 */
static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

node_t read_statement_declaration(context_t ctx, vec_t tokens, size_t *position,
                                  const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  cubec_statement_declaration_t node = NULL;
  node_t declarator = NULL;
  location_t start_location = {0};
  bool is_export = false;
  bool is_extern = false;
  bool is_builtin = false;
  bool is_comptime = false;
  bool is_using = false;
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

  /* 1. Parse optional modifiers: export / exportlib / extern / builtin /
   * comptime / using */
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
    } else if (_is_keyword(tokens, current, "using")) {
      if (is_using)
        goto onerror;
      is_using = true;
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
  if (is_extern && is_export)
    goto onerror;
  if (is_extern && is_exportlib)
    goto onerror;
  if (is_extern && is_builtin)
    goto onerror;
  if (is_extern && is_comptime)
    goto onerror;
  if (is_builtin && is_comptime)
    goto onerror;
  if (is_exportlib && is_builtin)
    goto onerror;
  if (is_exportlib && is_comptime)
    goto onerror;
  if (is_using && is_extern)
    goto onerror;
  if (is_using && is_builtin)
    goto onerror;
  if (is_using && is_comptime)
    goto onerror;

  /* 3. Expect 'var' keyword (skip if 'using' takes its place) */
  if (!is_using) {
    if (!_is_keyword(tokens, current, "var")) {
      goto onerror;
    }
    token_t var_token = vec_get(tokens, current);
    if (start_location.begin.offset == 0) {
      start_location = *token_get_location(var_token);
      start_location.filename = filename;
    }
    current++;
    skip_whitespace(tokens, &current);
  }

  /* 4. Parse single declarator */
  declarator = read_declaration_variable(ctx, tokens, &current, filename);
  if (node_is_error(declarator)) {
    allocator_free(allocator, &decorators);
    return declarator;
  }
  if (!declarator)
    goto onerror;

  /* 5. Validate: extern/builtin declarations must not have initializer */
  if (is_extern || is_builtin) {
    cubec_declaration_variable_t dv = (cubec_declaration_variable_t)declarator;
    if (dv->expression) {
      goto onerror;
    }
    if (!dv->type) {
      goto onerror;
    }
  }

  /* 5b. Validate: comptime declarations must have an initializer */
  if (is_comptime) {
    cubec_declaration_variable_t dv = (cubec_declaration_variable_t)declarator;
    if (!dv->expression) {
      goto onerror;
    }
  }

  skip_whitespace(tokens, &current);

  /* 6. Expect semicolon */
  token_t semi = vec_get(tokens, current);
  if (!token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    goto onerror;
  }
  current++;

  /* 7. Build location spanning from first modifier or 'var' to semicolon */
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_statement_declaration_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .is_exportlib = is_exportlib,
      .is_extern = is_extern,
      .is_builtin = is_builtin,
      .is_comptime = is_comptime,
      .is_using = is_using,
      .declarator = declarator,
      .decorators = decorators,
  };
  node =
      allocator_create(allocator, &g_cubec_statement_declaration_type, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &declarator);
  allocator_free(allocator, &node);
  return cubec_ast_create_error(ctx, start_location);
}

node_t cubec_ast_create_var_decl_stmt(context_t ctx, location_t loc,
                                      const char *name, node_t type,
                                      node_t expr, bool is_export,
                                      bool is_extern, bool is_builtin,
                                      bool is_comptime, bool is_using) {
  allocator_t alloc = ctx->allocator;
  node_t name_node = cubec_ast_create_identifier(ctx, loc, name);
  cubec_declaration_variable_init_t dv_init = {
      .location = loc,
      .parent = NULL,
      .identifier = name_node,
      .type = type,
      .expression = expr,
  };
  node_t decl_node = (node_t)allocator_create(
      alloc, &g_cubec_declaration_variable_type, &dv_init);
  cubec_statement_declaration_init_t sd_init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .is_extern = is_extern,
      .is_builtin = is_builtin,
      .is_comptime = is_comptime,
      .is_using = is_using,
      .declarator = decl_node,
  };
  return (node_t)allocator_create(alloc, &g_cubec_statement_declaration_type,
                                  &sd_init);
}
