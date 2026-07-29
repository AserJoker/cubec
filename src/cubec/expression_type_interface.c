#include "cubec/expression_type_interface.h"
#include "core/token.h"
#include "cubec/ast_factory.h"
#include "cubec/generic_param.h"
#include "cubec/interface_method.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_type_interface_init(
    cubec_expression_type_interface_t self, allocator_t allocator,
    cubec_expression_type_interface_init_t *init) {
  if (!init) return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TYPE_INTERFACE,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_type.init(&self->super, allocator, &super_init);
  self->generic_params = init->generic_params;
  self->members = init->members;
}

static void _cubec_expression_type_interface_dispose(
    cubec_expression_type_interface_t self, allocator_t allocator) {
  allocator_free(allocator, &self->members);
  allocator_free(allocator, &self->generic_params);
  g_cubec_expression_type.dispose(&self->super, allocator);
}

static void _cubec_expression_type_interface_clone(
    cubec_expression_type_interface_t self, allocator_t allocator,
    cubec_expression_type_interface_t another) {
  g_cubec_expression_type.clone(&self->super, allocator, &another->super);
  self->generic_params = another->generic_params
                             ? value_clone(allocator, another->generic_params)
                             : NULL;
  self->members = value_clone(allocator, another->members);
  return;
}

static void _cubec_expression_type_interface_move(
    cubec_expression_type_interface_t self, allocator_t allocator,
    cubec_expression_type_interface_t another) {
  g_cubec_expression_type.move(&self->super, allocator, &another->super);
  self->generic_params = another->generic_params
                             ? value_move(allocator, another->generic_params)
                             : NULL;
  self->members = value_move(allocator, another->members);
  return;
}

type_t g_cubec_expression_type_interface_type = {
    .name = "cubec.cubec.expression_type_interface",
    .size = sizeof(struct _cubec_expression_type_interface_t),
    .init = (type_init_fn_t)_cubec_expression_type_interface_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_type_interface_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_type_interface_clone,
    .move = (type_move_fn_t)_cubec_expression_type_interface_move,
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
 *  Helper: parse associated type inside interface body
 *          type <name> [<generic_params>] ;
 * -------------------------------------------------------------------------- */

static node_t _read_associated_type(context_t ctx, vec_t tokens,
                                     size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t name = NULL;
  vec_t params = NULL;
  cubec_statement_declaration_type_t node = NULL;

  /* Expect 'type' keyword */
  if (!_is_keyword(tokens, current, "type")) {
    return NULL;
  }
  token_t type_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(type_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* Parse type name (required) */
  name = read_literal_identifier(ctx, tokens, &current, filename);
  if (node_is_error(name)) goto onerror;
  if (!name) {
    goto cleanup;
  }

  skip_whitespace(tokens, &current);

  /* Parse optional generic parameters */
  params = read_generic_params(ctx, tokens, &current, filename);
  if (params) {
    skip_whitespace(tokens, &current);
  }

  /* Expect ';' */
  token_t semi = vec_get(tokens, current);
  if (!token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    goto cleanup;
  }
  current++;

  /* Build location */
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_statement_declaration_type_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = false,
      .is_builtin = false,
      .name = name,
      .params = params,
      .type_value = NULL,
  };
  node = allocator_create(allocator, &g_cubec_statement_decltype, &init);
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &params);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return NULL;
onerror:
  allocator_free(allocator, &params);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return cubec_ast_create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Internal: parse interface body after 'interface' keyword consumed
 *            [generic_params] { members }
 * -------------------------------------------------------------------------- */

node_t read_expression_type_interface_body(context_t ctx, vec_t tokens,
                                             size_t *position, const char *filename,
                                             location_t start_location) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  vec_t generic_params = NULL;
  vec_t members = NULL;
  cubec_expression_type_interface_t node = NULL;

  /* 1. Parse optional generic parameters */
  generic_params = read_generic_params(ctx, tokens, &current, filename);
  if (generic_params) {
    skip_whitespace(tokens, &current);
  }

  /* 2. Expect '{' */
  if (!_is_symbol(tokens, current, "{")) {
    goto cleanup;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse members */
  members = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
  while (!_is_symbol(tokens, current, "}")) {
    node_t member = NULL;

    /* Try associated type: type <name> [<params>] ; */
    member = _read_associated_type(ctx, tokens, &current, filename);
    if (node_is_error(member)) goto onerror;

    /* Try method signature: func <name> [<params>] (<args>) [: <type>] ; */
    if (!member) {
      member = read_interface_method(ctx, tokens, &current, filename);
      if (node_is_error(member)) goto onerror;
    }

    if (!member) {
      break;
    }

    vec_push(members, member);
    skip_whitespace(tokens, &current);
  }

  /* 4. Expect '}' */
  if (!_is_symbol(tokens, current, "}")) {
    goto cleanup;
  }
  token_t close_brace = vec_get(tokens, current);
  current++;

  /* 5. Build location spanning from start to '}' */
  location_t *end_loc = token_get_location(close_brace);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_expression_type_interface_init_t init = {
      .location = loc,
      .parent = NULL,
      .generic_params = generic_params,
      .members = members,
  };
  node = allocator_create(allocator, &g_cubec_expression_type_interface_type, &init);
  *position = current;
  return (node_t)&node->super;

cleanup:
  allocator_free(allocator, &members);
  allocator_free(allocator, &generic_params);
  allocator_free(allocator, &node);
  return NULL;
onerror:
  allocator_free(allocator, &members);
  allocator_free(allocator, &generic_params);
  allocator_free(allocator, &node);
  return cubec_ast_create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Parser: read_expression_type_interface — entry point for type expressions
 * -------------------------------------------------------------------------- */

node_t read_expression_type_interface(context_t ctx, vec_t tokens,
                                       size_t *position, const char *filename) {
  size_t current = *position;

  /* Expect 'interface' keyword */
  if (!_is_keyword(tokens, current, "interface")) {
    return NULL;
  }
  token_t interface_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(interface_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  node_t result = read_expression_type_interface_body(ctx, tokens, &current, filename, start_location);
  if (node_is_error(result)) return result;
  if (result) {
    *position = current;
    return result;
  }

  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                       start_location, "invalid interface type expression");
  ctx->error_count++;
  return cubec_ast_create_error(ctx, start_location);
}
