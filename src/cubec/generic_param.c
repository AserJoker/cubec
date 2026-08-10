#include "cubec/generic_param.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "core/token_writer.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_generic_param_init(cubec_generic_param_t self,
                                      allocator_t allocator,
                                      cubec_generic_param_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_GENERIC_PARAM,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->name = init->name;
  self->constraints = init->constraints;
  self->value_type = init->value_type;
  self->is_rest = init->is_rest;
}

static void _cubec_generic_param_dispose(cubec_generic_param_t self,
                                         allocator_t allocator) {
  allocator_free(allocator, &self->value_type);
  allocator_free(allocator, &self->constraints);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_generic_param_clone(cubec_generic_param_t self,
                                       allocator_t allocator,
                                       cubec_generic_param_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->name = alloc_clone(allocator, another->name);
  self->constraints = another->constraints
                          ? alloc_clone(allocator, another->constraints)
                          : NULL;
  self->value_type =
      another->value_type ? alloc_clone(allocator, another->value_type) : NULL;
  self->is_rest = another->is_rest;
}

static void _cubec_generic_param_move(cubec_generic_param_t self,
                                      allocator_t allocator,
                                      cubec_generic_param_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->name = alloc_move(allocator, another->name);
  self->constraints =
      another->constraints ? alloc_move(allocator, another->constraints) : NULL;
  self->value_type =
      another->value_type ? alloc_move(allocator, another->value_type) : NULL;
  self->is_rest = another->is_rest;
}

type_t g_cubec_generic_param_type = {
    .name = "cubec.cubec.generic_param",
    .size = sizeof(struct _cubec_generic_param_t),
    .init = (type_init_fn_t)_cubec_generic_param_init,
    .dispose = (type_dispose_fn_t)_cubec_generic_param_dispose,
    .clone = (type_clone_fn_t)_cubec_generic_param_clone,
    .move = (type_move_fn_t)_cubec_generic_param_move,
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
 *  Helper: parse a single generic parameter
 * -------------------------------------------------------------------------- */

static cubec_generic_param_t _parse_one_generic_param(context_t ctx,
                                                      vec_t tokens,
                                                      size_t *current,
                                                      const char *filename) {
  allocator_t allocator = ctx->allocator;

  node_t name = NULL;
  vec_t constraints = NULL;
  node_t value_type = NULL;
  bool is_rest = false;

  /* 0. Check for `...` rest prefix */
  if (_is_symbol(tokens, *current, "...")) {
    is_rest = true;
    (*current)++;
    skip_whitespace(tokens, current);
  }

  /* 1. Parse identifier (param name) */
  name = read_literal_identifier(ctx, tokens, current, filename);
  if (!name) {
    goto fail;
  }

  skip_whitespace(tokens, current);

  /* 2. Check for optional `extends <type> [& <type>]*` constraint */
  if (_is_keyword(tokens, *current, "extends")) {
    (*current)++;
    skip_whitespace(tokens, current);
    node_t first = read_type_expression_primary(ctx, tokens, current, filename);
    if (!first) {
      goto fail;
    }
    constraints = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
    vec_push(constraints, first);
    skip_whitespace(tokens, current);
    while (token_is(vec_get(tokens, *current), CUBEC_TOKEN_SYMBOL, "&")) {
      (*current)++;
      skip_whitespace(tokens, current);
      node_t next =
          read_type_expression_primary(ctx, tokens, current, filename);
      if (!next) {
        goto fail;
      }
      vec_push(constraints, next);
      skip_whitespace(tokens, current);
    }
  }
  /* 3. Check for optional `: <type>` value generic annotation */
  else if (_is_symbol(tokens, *current, ":")) {
    (*current)++;
    skip_whitespace(tokens, current);
    value_type = read_type_expression_primary(ctx, tokens, current, filename);
    if (!value_type) {
      goto fail;
    }
    skip_whitespace(tokens, current);
  }

  cubec_generic_param_t param = NULL;
  param = allocator_create(allocator, &g_cubec_generic_param_type,
                           &(cubec_generic_param_init_t){
                               .name = name,
                               .constraints = constraints,
                               .value_type = value_type,
                               .is_rest = is_rest,
                           });

  return param;

fail:
  allocator_free(allocator, &value_type);
  allocator_free(allocator, &constraints);
  allocator_free(allocator, &name);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Parser: read_generic_params
 * -------------------------------------------------------------------------- */

vec_t read_generic_params(context_t ctx, vec_t tokens, size_t *position,
                          const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  vec_t params = NULL;

  /* Check for opening '[' */
  token_t bracket = vec_get(tokens, current);
  if (!bracket || !token_is(bracket, CUBEC_TOKEN_SYMBOL, "[")) {
    return NULL;
  }
  current++;

  skip_whitespace(tokens, &current);

  /* Create params vec with auto_dispose */
  params = allocator_create(allocator, &g_vec_type, &(vec_init_t){true});

  /* Parse first param (required) */
  cubec_generic_param_t param =
      _parse_one_generic_param(ctx, tokens, &current, filename);
  if (!param) {
    goto cleanup_params;
  }
  vec_push(params, param);

  /* Parse additional params (optional, comma-separated) */
  while (true) {
    skip_whitespace(tokens, &current);
    token_t comma = vec_get(tokens, current);
    if (!comma || !token_is(comma, CUBEC_TOKEN_SYMBOL, ",")) {
      break;
    }
    current++;
    skip_whitespace(tokens, &current);

    param = _parse_one_generic_param(ctx, tokens, &current, filename);
    if (!param) {
      goto cleanup_params;
    }
    vec_push(params, param);
  }

  /* Expect closing ']' */
  token_t rbracket = vec_get(tokens, current);
  if (!rbracket || !token_is(rbracket, CUBEC_TOKEN_SYMBOL, "]")) {
    goto cleanup_params;
  }
  current++;

  *position = current;
  return params;

cleanup_params:
  allocator_free(allocator, &params);
onerror:
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: create_generic_param
 * -------------------------------------------------------------------------- */

node_t create_generic_param(context_t ctx, location_t loc, const char *name,
                            vec_t constraints, node_t value_type,
                            bool is_rest) {
  allocator_t alloc = ctx->allocator;
  node_t name_node = create_literal_identifier(ctx, loc, name);
  cubec_generic_param_init_t init = {
      .location = loc,
      .name = name_node,
      .constraints = constraints,
      .value_type = value_type,
      .is_rest = is_rest,
  };
  return (node_t)allocator_create(alloc, &g_cubec_generic_param_type, &init);
}

void emit_generic_param(emit_context_t ctx, node_t node) {
  cubec_generic_param_t param = (cubec_generic_param_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  if (param->is_rest) {
    emit_symbol(ctx, "...");
  }
  emit_expression(ctx, param->name);
  if (param->constraints) {
    emit_space(ctx);
    emit_keyword(ctx, "extends");
    emit_space(ctx);
    for (size_t i = 0; i < vec_get_size(param->constraints); i++) {
      if (i != 0) {
        emit_space(ctx);
        emit_symbol(ctx, "&");
        emit_space(ctx);
      }
      emit_expression(ctx, vec_get(param->constraints, i));
    }
  } else if (param->value_type) {
    emit_symbol(ctx, ":");
    emit_space(ctx);
    emit_expression(ctx, param->value_type);
  }
}
