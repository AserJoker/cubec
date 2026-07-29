#include "cubec/function_argument.h"
#include "cubec/ast_factory_internal.h"
#include "core/token.h"
#include "cubec/ast_factory.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_function_argument_init(
    cubec_function_argument_t self, allocator_t allocator,
    cubec_function_argument_init_t *init) {
  if (!init) return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_FUNCTION_ARGUMENT,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->identifier = init->identifier;
  self->type = init->type;
  self->is_rest = init->is_rest;
}

static void _cubec_function_argument_dispose(
    cubec_function_argument_t self, allocator_t allocator) {
  allocator_free(allocator, &self->type);
  allocator_free(allocator, &self->identifier);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_function_argument_clone(
    cubec_function_argument_t self, allocator_t allocator,
    cubec_function_argument_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->identifier = value_clone(allocator, another->identifier);
  self->type = another->type
                   ? value_clone(allocator, another->type)
                   : NULL;
  self->is_rest = another->is_rest;
}

static void _cubec_function_argument_move(
    cubec_function_argument_t self, allocator_t allocator,
    cubec_function_argument_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->identifier = value_move(allocator, another->identifier);
  self->type = another->type
                   ? value_move(allocator, another->type)
                   : NULL;
  self->is_rest = another->is_rest;
}

type_t g_cubec_function_argument_type = {
    .name = "cubec.cubec.function_argument",
    .size = sizeof(struct _cubec_function_argument_t),
    .init = (type_init_fn_t)_cubec_function_argument_init,
    .dispose = (type_dispose_fn_t)_cubec_function_argument_dispose,
    .clone = (type_clone_fn_t)_cubec_function_argument_clone,
    .move = (type_move_fn_t)_cubec_function_argument_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check symbol
 * -------------------------------------------------------------------------- */

static bool _is_symbol(vec_t tokens, size_t position, const char *symbol) {
  token_t token = vec_get(tokens, position);
  if (!token) return false;
  return token_is(token, CUBEC_TOKEN_SYMBOL, symbol);
}

/* --------------------------------------------------------------------------
 *  Parser: read_function_argument
 * -------------------------------------------------------------------------- */

node_t read_function_argument(context_t ctx, vec_t tokens,
                               size_t *position, const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t identifier = NULL;
  node_t type = NULL;
  bool is_rest = false;

  /* 1. Check for '...' prefix (pack parameter) */
  if (_is_symbol(tokens, current, "...")) {
    is_rest = true;
    current++;
    skip_whitespace(tokens, &current);
  }

  /* 2. Check for identifier — if not present, return NULL */
  token_t first = vec_get(tokens, current);
  if (!first || token_get_kind(first) != CUBEC_TOKEN_IDENTIFIER) {
    return NULL;
  }

  /* 3. Parse identifier */
  identifier = read_literal_identifier(ctx, tokens, &current, filename);
  if (!identifier) {
    return NULL;
  }

  skip_whitespace(tokens, &current);

  /* 4. Check for optional ': type' */
  if (_is_symbol(tokens, current, ":")) {
    current++;
    skip_whitespace(tokens, &current);
    type = read_type_expression_primary(ctx, tokens, &current, filename);
    if (!type) {
      goto fail;
    }
    skip_whitespace(tokens, &current);
  }

  /* 5. Build location */
  location_t *start_loc = token_get_location(first);
  location_t *end_loc = type ? &((node_t)type)->location : token_get_location(vec_get(tokens, current - 1));
  location_t loc = {
      .begin = start_loc->begin,
      .end = end_loc->end,
      .filename = filename,
  };

  /* 6. Create node */
  cubec_function_argument_t arg = NULL;
  arg = allocator_create(allocator, &g_cubec_function_argument_type,
      &(cubec_function_argument_init_t){
          .location = loc,
          .identifier = identifier,
          .type = type,
          .is_rest = is_rest,
      });
  if (!arg) goto fail;

  *position = current;
  return (node_t)&arg->super;

fail:
  allocator_free(allocator, &type);
  allocator_free(allocator, &identifier);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: cubec_ast_create_func_arg
 * -------------------------------------------------------------------------- */

node_t cubec_ast_create_func_arg(context_t ctx, location_t loc,
                                 const char *name, node_t type) {
  allocator_t alloc = ctx->allocator;
  cubec_literal_identifier_t name_node = _make_ident_node(ctx, loc, name);
  cubec_function_argument_init_t init = {.location = loc,
                                         .identifier = (node_t)name_node,
                                         .type = type};
  return (node_t)allocator_create(alloc, &g_cubec_function_argument_type,
                                  &init);
}
