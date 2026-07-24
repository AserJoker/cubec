#include "cubec/statement_union.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/ast_factory.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/decorator.h"
#include "cubec/expression_type_union.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_union_init(
    cubec_statement_union_t self, allocator_t allocator,
    cubec_statement_union_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_UNION,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->is_export = init->is_export;
  self->name = init->name;
  self->generic_params = init->generic_params;
  self->implements = init->implements;
  self->members = init->members;
  self->decorators = init->decorators;
onerror:
  return;
}

static void _cubec_statement_union_dispose(
    cubec_statement_union_t self, allocator_t allocator) {
  allocator_free(allocator, &self->decorators);
  allocator_free(allocator, &self->implements);
  allocator_free(allocator, &self->members);
  allocator_free(allocator, &self->generic_params);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_union_clone(
    cubec_statement_union_t self, allocator_t allocator,
    cubec_statement_union_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->is_export = another->is_export;
  self->name = TRY_LOCAL(onerror, value_clone(allocator, another->name));
  self->generic_params = another->generic_params
                             ? TRY_LOCAL(onerror, value_clone(allocator, another->generic_params))
                             : NULL;
  self->implements = another->implements
                         ? TRY_LOCAL(onerror, value_clone(allocator, another->implements))
                         : NULL;
  self->members = TRY_LOCAL(onerror, value_clone(allocator, another->members));
  return;
onerror:
  return;
}

static void _cubec_statement_union_move(
    cubec_statement_union_t self, allocator_t allocator,
    cubec_statement_union_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->is_export = another->is_export;
  self->name = TRY_LOCAL(onerror, value_move(allocator, another->name));
  self->generic_params = another->generic_params
                             ? TRY_LOCAL(onerror, value_move(allocator, another->generic_params))
                             : NULL;
  self->implements = another->implements
                         ? TRY_LOCAL(onerror, value_move(allocator, another->implements))
                         : NULL;
  self->members = TRY_LOCAL(onerror, value_move(allocator, another->members));
  return;
onerror:
  return;
}

type_t g_cubec_statement_union_type = {
    .name = "cubec.cubec.statement_union",
    .size = sizeof(struct _cubec_statement_union_t),
    .init = (type_init_fn_t)_cubec_statement_union_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_union_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_union_clone,
    .move = (type_move_fn_t)_cubec_statement_union_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check keyword
 * -------------------------------------------------------------------------- */

static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token) return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD) return false;
  return location_is(token_get_location(token), keyword);
}

/* --------------------------------------------------------------------------
 *  Parser: read_statement_union — delegates to read_expression_type_union
 * -------------------------------------------------------------------------- */

node_t read_statement_union(allocator_t allocator, vec_t tokens,
                             size_t *position, const char *filename) {
  size_t current = *position;
  bool is_export = false;
  node_t name = NULL;
  node_t expr_node = NULL;
  cubec_statement_union_t node = NULL;
  location_t start_location = {0};
  vec_t decorators = NULL;
  vec_t implements = NULL;

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

  /* 1. Parse optional 'export' modifier */
  if (_is_keyword(tokens, current, "export")) {
    is_export = true;
    token_t tok = TRY_LOCAL(onerror, vec_get(tokens, current));
    start_location = *token_get_location(tok);
    start_location.filename = filename;
    current++;
    skip_whitespace(tokens, &current);
  }

  /* 2. Expect 'union' keyword (not 'cunion') */
  if (!_is_keyword(tokens, current, "union")) {
    goto onerror;
  }
  if (start_location.begin.offset == 0) {
    token_t tok = TRY_LOCAL(onerror, vec_get(tokens, current));
    start_location = *token_get_location(tok);
    start_location.filename = filename;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse union name (required for statement form) */
  name = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
  if (!name) {
    THROW_LOCAL(cleanup, "expected union name after 'union'");
  }

  skip_whitespace(tokens, &current);

  /* 4. Delegate to read_expression_type_union_body for [generic_params] { members } */
  expr_node = TRY_LOCAL(cleanup, read_expression_type_union_body(allocator, tokens, &current, filename, start_location, &implements));
  if (!expr_node) {
    THROW_LOCAL(cleanup, "expected '{' after union name");
  }
  cubec_expression_type_union_t expr_union = (cubec_expression_type_union_t)expr_node;

  /* 5. Build location */
  location_t loc = expr_node->location;
  if (start_location.begin.offset != 0) {
    loc.begin = start_location.begin;
  }

  /* 6. Create statement_union node, transferring ownership */
  cubec_statement_union_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .name = name,
      .generic_params = expr_union->generic_params,
      .implements = implements,
      .members = expr_union->members,
      .decorators = decorators,
  };

  /* Nullify fields to prevent double-free */
  expr_union->generic_params = NULL;
  expr_union->members = NULL;

  allocator_free(allocator, &expr_node);

  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_union_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &implements);
  allocator_free(allocator, &name);
  allocator_free(allocator, &expr_node);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &implements);
  allocator_free(allocator, &name);
  allocator_free(allocator, &expr_node);
  allocator_free(allocator, &node);
  return NULL;
}

node_t cubec_ast_create_union_stmt(allocator_t alloc, location_t loc,
                                   const char *name, vec_t members,
                                   bool is_export, vec_t implements) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_statement_union_init_t init = {
      .location = loc, .parent = NULL, .is_export = is_export,
      .name = name_node, .generic_params = NULL, .implements = implements,
      .members = members};
  return (node_t)allocator_create(alloc, &g_cubec_statement_union_type,
                                  &init);
}
