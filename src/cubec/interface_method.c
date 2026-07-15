#include "cubec/interface_method.h"
#include "cubec/ast_factory_internal.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/ast_factory.h"
#include "cubec/expression.h"
#include "cubec/function_argument.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_interface_method_init(
    cubec_interface_method_t self, allocator_t allocator,
    cubec_interface_method_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_INTERFACE_METHOD,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->name = init->name;
  self->generic_params = init->generic_params;
  self->arguments = init->arguments;
  self->return_type = init->return_type;
onerror:
  return;
}

static void _cubec_interface_method_dispose(
    cubec_interface_method_t self, allocator_t allocator) {
  allocator_free(allocator, &self->return_type);
  allocator_free(allocator, &self->arguments);
  allocator_free(allocator, &self->generic_params);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_interface_method_clone(
    cubec_interface_method_t self, allocator_t allocator,
    cubec_interface_method_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->name = TRY_LOCAL(onerror, value_clone(allocator, another->name));
  self->generic_params = another->generic_params
                             ? TRY_LOCAL(onerror, value_clone(allocator, another->generic_params))
                             : NULL;
  self->arguments = TRY_LOCAL(onerror, value_clone(allocator, another->arguments));
  self->return_type = another->return_type
                          ? TRY_LOCAL(onerror, value_clone(allocator, another->return_type))
                          : NULL;
  return;
onerror:
  return;
}

static void _cubec_interface_method_move(
    cubec_interface_method_t self, allocator_t allocator,
    cubec_interface_method_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->name = TRY_LOCAL(onerror, value_move(allocator, another->name));
  self->generic_params = another->generic_params
                             ? TRY_LOCAL(onerror, value_move(allocator, another->generic_params))
                             : NULL;
  self->arguments = TRY_LOCAL(onerror, value_move(allocator, another->arguments));
  self->return_type = another->return_type
                          ? TRY_LOCAL(onerror, value_move(allocator, another->return_type))
                          : NULL;
  return;
onerror:
  return;
}

type_t g_cubec_interface_method_type = {
    .name = "cubec.cubec.interface_method",
    .size = sizeof(struct _cubec_interface_method_t),
    .init = (type_init_fn_t)_cubec_interface_method_init,
    .dispose = (type_dispose_fn_t)_cubec_interface_method_dispose,
    .clone = (type_clone_fn_t)_cubec_interface_method_clone,
    .move = (type_move_fn_t)_cubec_interface_method_move,
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
 *  Parser: read_interface_method
 * -------------------------------------------------------------------------- */

node_t read_interface_method(allocator_t allocator, vec_t tokens,
                              size_t *position, const char *filename) {
  size_t current = *position;
  node_t name = NULL;
  vec_t generic_params = NULL;
  vec_t arguments = NULL;
  node_t return_type = NULL;
  cubec_interface_method_t node = NULL;

  /* 1. Expect 'func' keyword */
  if (!_is_keyword(tokens, current, "func")) {
    return NULL;
  }
  token_t func_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  location_t start_location = *token_get_location(func_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse method name (required) */
  name = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
  if (!name) {
    THROW_LOCAL(cleanup, "expected method name after 'func'");
  }

  skip_whitespace(tokens, &current);

  /* 3. Parse optional generic parameters */
  generic_params = TRY_LOCAL(cleanup, read_generic_params(allocator, tokens, &current, filename));
  if (generic_params) {
    skip_whitespace(tokens, &current);
  }

  /* 4. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    THROW_LOCAL(cleanup, "expected '(' after method name");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse parameter list */
  arguments = TRY_LOCAL(cleanup, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));
  while (!_is_symbol(tokens, current, ")")) {
    node_t arg = TRY_LOCAL(cleanup, read_function_argument(allocator, tokens, &current, filename));
    if (!arg) {
      THROW_LOCAL(cleanup, "expected parameter in method signature");
    }
    vec_push(arguments, arg);
    skip_whitespace(tokens, &current);

    if (_is_symbol(tokens, current, ",")) {
      current++;
      skip_whitespace(tokens, &current);
    }
  }

  /* 6. Expect ')' */
  current++;
  skip_whitespace(tokens, &current);

  /* 7. Parse optional return type ': <type>' */
  if (_is_symbol(tokens, current, ":")) {
    current++;
    skip_whitespace(tokens, &current);
    return_type = TRY_LOCAL(cleanup, read_expression_type(allocator, tokens, &current, filename));
    if (!return_type) {
      THROW_LOCAL(cleanup, "expected type expression after ':'");
    }
    skip_whitespace(tokens, &current);
  }

  /* 8. Expect ';' (method signature has no body) */
  token_t semi = TRY_LOCAL(cleanup, vec_get(tokens, current));
  if (!token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    location_t *loc = token_get_location(semi);
    THROW_LOCAL(cleanup,
                "%s:%" PRIuPTR ":%" PRIuPTR " expected ';' after method signature",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;

  /* 9. Build location and node */
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_interface_method_init_t init = {
      .location = loc,
      .name = name,
      .generic_params = generic_params,
      .arguments = arguments,
      .return_type = return_type,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_interface_method_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &return_type);
  allocator_free(allocator, &arguments);
  allocator_free(allocator, &generic_params);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &return_type);
  allocator_free(allocator, &arguments);
  allocator_free(allocator, &generic_params);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_iface_method
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_iface_method(allocator_t alloc, location_t loc,
                                     const char *name, vec_t args,
                                     node_t return_type) {
  node_t name_node = (node_t)_make_ident_node(alloc, loc, name);
  cubec_interface_method_init_t init = {
      .location = loc, .name = name_node, .generic_params = NULL,
      .arguments = args, .return_type = return_type};
  return (node_t)allocator_create(alloc, &g_cubec_interface_method_type,
                                  &init);
}
