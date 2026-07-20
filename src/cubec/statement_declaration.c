#include "cubec/statement_declaration.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/ast_factory.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/decorator.h"
#include "cubec/declaration_variable.h"
#include "cubec/node.h"
#include "cubec/token.h"

static void _cubec_statement_declaration_init(
    cubec_statement_declaration_t self, allocator_t allocator,
    cubec_statement_declaration_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_DECLARATION,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->is_export = init->is_export;
  self->is_extern = init->is_extern;
  self->is_builtin = init->is_builtin;
  self->is_comptime = init->is_comptime;
  self->is_using = init->is_using;
  self->declarator = init->declarator;
  self->decorators = init->decorators;
onerror:
  return;
}

static void _cubec_statement_declaration_dispose(
    cubec_statement_declaration_t self, allocator_t allocator) {
  allocator_free(allocator, &self->decorators);
  allocator_free(allocator, &self->declarator);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_declaration_clone(
    cubec_statement_declaration_t self, allocator_t allocator,
    cubec_statement_declaration_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->is_export = another->is_export;
  self->is_extern = another->is_extern;
  self->is_builtin = another->is_builtin;
  self->is_comptime = another->is_comptime;
  self->is_using = another->is_using;
  self->declarator = TRY_LOCAL(onerror, value_clone(allocator, another->declarator));
  return;
onerror:
  return;
}

static void _cubec_statement_declaration_move(
    cubec_statement_declaration_t self, allocator_t allocator,
    cubec_statement_declaration_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->is_export = another->is_export;
  self->is_extern = another->is_extern;
  self->is_builtin = another->is_builtin;
  self->is_comptime = another->is_comptime;
  self->is_using = another->is_using;
  self->declarator = TRY_LOCAL(onerror, value_move(allocator, another->declarator));
  return;
onerror:
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
  if (!token) return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD) return false;
  return location_is(token_get_location(token), keyword);
}

node_t read_statement_declaration(allocator_t allocator, vec_t tokens,
                                  size_t *position, const char *filename) {
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
      node_t dec = read_decorator(allocator, tokens, &current, filename);
      if (!dec) break;
      if (!decorators) {
        decorators = TRY_LOCAL(onerror, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));
      }
      vec_push(decorators, dec);
    }
  }

  /* 1. Parse optional modifiers: export / extern / builtin / comptime */
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
    } else if (_is_keyword(tokens, current, "using")) {
      if (is_using) THROW_LOCAL(onerror, "duplicate 'using' modifier");
      is_using = true;
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
  if (is_extern && is_export)
    THROW_LOCAL(onerror, "'extern' and 'export' are mutually exclusive");
  if (is_extern && is_builtin)
    THROW_LOCAL(onerror, "'extern' and 'builtin' are mutually exclusive");
  if (is_extern && is_comptime)
    THROW_LOCAL(onerror, "'extern' and 'comptime' are mutually exclusive");
  if (is_builtin && is_comptime)
    THROW_LOCAL(onerror, "'builtin' and 'comptime' are mutually exclusive");
  if (is_using && is_extern)
    THROW_LOCAL(onerror, "'using' and 'extern' are mutually exclusive");
  if (is_using && is_builtin)
    THROW_LOCAL(onerror, "'using' and 'builtin' are mutually exclusive");
  if (is_using && is_comptime)
    THROW_LOCAL(onerror, "'using' and 'comptime' are mutually exclusive");

  /* 3. Expect 'var' keyword (skip if 'using' takes its place) */
  if (!is_using) {
    if (!_is_keyword(tokens, current, "var")) {
      return NULL;
    }
    token_t var_token = TRY_LOCAL(onerror, vec_get(tokens, current));
    if (start_location.begin.offset == 0) {
      start_location = *token_get_location(var_token);
      start_location.filename = filename;
    }
    current++;
    skip_whitespace(tokens, &current);
  }

  /* 4. Parse single declarator */
  declarator = TRY_LOCAL(cleanup, read_declaration_variable(allocator, tokens, &current, filename));
  if (!declarator) {
    THROW_LOCAL(cleanup, "expected variable declarator after 'var'");
  }

  /* 5. Validate: extern/builtin declarations must not have initializer */
  if (is_extern || is_builtin) {
    cubec_declaration_variable_t dv = (cubec_declaration_variable_t)declarator;
    if (dv->expression) {
      THROW_LOCAL(cleanup, "%s var declaration cannot have an initializer",
                  is_extern ? "extern" : "builtin");
    }
    if (!dv->type) {
      THROW_LOCAL(cleanup, "%s var declaration requires a type annotation",
                  is_extern ? "extern" : "builtin");
    }
  }

  /* 5b. Validate: comptime declarations must have an initializer */
  if (is_comptime) {
    cubec_declaration_variable_t dv = (cubec_declaration_variable_t)declarator;
    if (!dv->expression) {
      THROW_LOCAL(cleanup, "comptime var declaration requires an initializer");
    }
  }

  skip_whitespace(tokens, &current);

  /* 6. Expect semicolon */
  token_t semi = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    location_t *loc = token_get_location(semi);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected ';' after declaration statement",
                filename, loc->begin.line + 1, loc->begin.column);
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
      .is_extern = is_extern,
      .is_builtin = is_builtin,
      .is_comptime = is_comptime,
      .is_using = is_using,
      .declarator = declarator,
      .decorators = decorators,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_declaration_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &declarator);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &declarator);
  allocator_free(allocator, &node);
  return NULL;
}

node_t cubec_ast_create_var_decl_stmt(allocator_t alloc, location_t loc,
                                      const char *name, node_t type,
                                      node_t expr, bool is_export,
                                      bool is_extern, bool is_builtin,
                                      bool is_comptime, bool is_using) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_declaration_variable_init_t dv_init = {
      .location = loc, .parent = NULL, .identifier = name_node,
      .type = type, .expression = expr};
  node_t decl_node = (node_t)allocator_create(
      alloc, &g_cubec_declaration_variable_type, &dv_init);
  cubec_statement_declaration_init_t sd_init = {
      .location = loc, .parent = NULL, .is_export = is_export,
      .is_extern = is_extern, .is_builtin = is_builtin,
      .is_comptime = is_comptime, .is_using = is_using, .declarator = decl_node};
  return (node_t)allocator_create(alloc, &g_cubec_statement_declaration_type,
                                  &sd_init);
}
