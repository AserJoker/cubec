#include "cubec/statement_interface.h"
#include "cubec/ast_create_helpers.h"
#include "core/token.h"
#include "cubec/decorator.h"
#include "cubec/expression_type_interface.h"
#include "cubec/generic_param.h"
#include "cubec/interface_method.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/token.h"
#include <inttypes.h>
#include "cubec/node_error.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_interface_init(
    cubec_statement_interface_t self, allocator_t allocator,
    cubec_statement_interface_init_t *init) {
  if (!init) return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_INTERFACE,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->is_export = init->is_export;
  self->name = init->name;
  self->generic_params = init->generic_params;
  self->members = init->members;
  self->decorators = init->decorators;
}

static void _cubec_statement_interface_dispose(
    cubec_statement_interface_t self, allocator_t allocator) {
  allocator_free(allocator, &self->decorators);
  allocator_free(allocator, &self->members);
  allocator_free(allocator, &self->generic_params);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_interface_clone(
    cubec_statement_interface_t self, allocator_t allocator,
    cubec_statement_interface_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->name = value_clone(allocator, another->name);
  self->generic_params = another->generic_params
                             ? value_clone(allocator, another->generic_params)
                             : NULL;
  self->members = value_clone(allocator, another->members);
  return;
}

static void _cubec_statement_interface_move(
    cubec_statement_interface_t self, allocator_t allocator,
    cubec_statement_interface_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->name = value_move(allocator, another->name);
  self->generic_params = another->generic_params
                             ? value_move(allocator, another->generic_params)
                             : NULL;
  self->members = value_move(allocator, another->members);
  return;
}

type_t g_cubec_statement_interface_type = {
    .name = "cubec.cubec.statement_interface",
    .size = sizeof(struct _cubec_statement_interface_t),
    .init = (type_init_fn_t)_cubec_statement_interface_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_interface_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_interface_clone,
    .move = (type_move_fn_t)_cubec_statement_interface_move,
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
 *  Parser: read_statement_interface — delegates to read_expression_type_interface
 * -------------------------------------------------------------------------- */

node_t read_statement_interface(context_t ctx, vec_t tokens,
                                 size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  bool is_export = false;
  node_t name = NULL;
  node_t expr_node = NULL;
  cubec_statement_interface_t node = NULL;
  location_t start_location = {0};
  vec_t decorators = NULL;

  /* Collect decorators [[...]] */
  {
    while (true) {
      skip_whitespace(tokens, &current);
      node_t dec = read_decorator(ctx, tokens, &current, filename);
      if (node_is_error(dec)) return dec;
      if (!dec) break;
      if (!decorators) {
        decorators = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
      }
      vec_push(decorators, dec);
    }
  }

  /* 1. Parse optional 'export' modifier */
  if (_is_keyword(tokens, current, "export")) {
    is_export = true;
    token_t tok = vec_get(tokens, current);
    start_location = *token_get_location(tok);
    start_location.filename = filename;
    current++;
    skip_whitespace(tokens, &current);
  }

  /* 2. Expect 'interface' keyword */
  if (!_is_keyword(tokens, current, "interface")) {
    goto onerror;
  }
  if (start_location.begin.offset == 0) {
    token_t tok = vec_get(tokens, current);
    start_location = *token_get_location(tok);
    start_location.filename = filename;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse interface name (required for statement form) */
  name = read_literal_identifier(ctx, tokens, &current, filename);
  if (node_is_error(name)) { allocator_free(allocator, &decorators); return name; }
  if (!name) goto onerror;

  skip_whitespace(tokens, &current);

  /* 4. Delegate to read_expression_type_interface_body for [generic_params] { members }
   *    (interface keyword already consumed, pass start_location for span) */
  expr_node = read_expression_type_interface_body(ctx, tokens, &current, filename, start_location);
  if (node_is_error(expr_node)) { allocator_free(allocator, &decorators); allocator_free(allocator, &name); return expr_node; }
  if (!expr_node) goto onerror;
  cubec_expression_type_interface_t expr_iface = (cubec_expression_type_interface_t)expr_node;

  /* 5. Build location (use modifier start or interface keyword location) */
  location_t loc = expr_node->location;
  if (start_location.begin.offset != 0) {
    loc.begin = start_location.begin;
  }

  /* 6. Create statement_interface node, transferring ownership from expression_type_interface */
  cubec_statement_interface_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .name = name,
      .generic_params = expr_iface->generic_params,
      .members = expr_iface->members,
      .decorators = decorators,
  };

  /* Nullify fields in expression node to prevent double-free during dispose */
  expr_iface->generic_params = NULL;
  expr_iface->members = NULL;

  allocator_free(allocator, &expr_node);

  node = allocator_create(allocator, &g_cubec_statement_interface_type, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &name);
  allocator_free(allocator, &expr_node);
  allocator_free(allocator, &node);
  return cubec_ast_create_error(ctx, start_location);
}

node_t cubec_ast_create_iface_stmt(context_t ctx, location_t loc,
                                   const char *name, vec_t members,
                                   bool is_export) {
  allocator_t alloc = ctx->allocator;
  cubec_literal_identifier_t name_node = _make_ident_node(ctx, loc, name);
  cubec_statement_interface_init_t init = {
      .location = loc, .parent = NULL, .is_export = is_export,
      .name = (node_t)name_node, .generic_params = NULL, .members = members};
  return (node_t)allocator_create(alloc, &g_cubec_statement_interface_type,
                                  &init);
}
