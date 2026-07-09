#include "cubec/generic_param.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_generic_param_init(
    cubec_generic_param_t self, allocator_t allocator,
    cubec_generic_param_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_GENERIC_PARAM,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->name = init->name;
  self->constraint = init->constraint;
  self->value_type = init->value_type;
  self->is_rest = init->is_rest;
onerror:
  return;
}

static void _cubec_generic_param_dispose(
    cubec_generic_param_t self, allocator_t allocator) {
  allocator_free(allocator, &self->value_type);
  allocator_free(allocator, &self->constraint);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_generic_param_clone(
    cubec_generic_param_t self, allocator_t allocator,
    cubec_generic_param_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->name = TRY_LOCAL(onerror, value_clone(allocator, another->name));
  self->constraint = another->constraint
                         ? TRY_LOCAL(onerror, value_clone(allocator, another->constraint))
                         : NULL;
  self->value_type = another->value_type
                         ? TRY_LOCAL(onerror, value_clone(allocator, another->value_type))
                         : NULL;
  self->is_rest = another->is_rest;
onerror:
  return;
}

static void _cubec_generic_param_move(
    cubec_generic_param_t self, allocator_t allocator,
    cubec_generic_param_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->name = TRY_LOCAL(onerror, value_move(allocator, another->name));
  self->constraint = another->constraint
                         ? TRY_LOCAL(onerror, value_move(allocator, another->constraint))
                         : NULL;
  self->value_type = another->value_type
                         ? TRY_LOCAL(onerror, value_move(allocator, another->value_type))
                         : NULL;
  self->is_rest = another->is_rest;
onerror:
  return;
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
 *  Parser: read_generic_params
 * -------------------------------------------------------------------------- */

vec_t read_generic_params(allocator_t allocator, vec_t tokens,
                          size_t *position, const char *filename) {
  size_t current = *position;
  vec_t params = NULL;
  cubec_generic_param_t param = NULL;

  /* Check for opening '[' */
  token_t bracket = vec_get(tokens, current);
  if (!bracket || !token_is(bracket, CUBEC_TOKEN_SYMBOL, "[")) {
    return NULL;
  }
  current++;

  skip_whitespace(tokens, &current);

  /* Create params vec with auto_dispose */
  params = TRY_LOCAL(onerror, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));

  /* Parse first param (required) */
  {
    node_t name = NULL;
    node_t constraint = NULL;
    node_t value_type = NULL;
    bool is_rest = false;

    /* 0. Check for `...` rest prefix */
    if (_is_symbol(tokens, current, "...")) {
      is_rest = true;
      current++;
      skip_whitespace(tokens, &current);
    }

    /* 1. Parse identifier (param name) */
    name = TRY_LOCAL(cleanup_params, read_literal_identifier(allocator, tokens, &current, filename));
    if (!name) {
      THROW_LOCAL(cleanup_params, "expected generic parameter name after '['");
    }

    skip_whitespace(tokens, &current);

    /* 2. Check for optional `extends <type>` constraint */
    if (_is_keyword(tokens, current, "extends")) {
      current++;
      skip_whitespace(tokens, &current);
      constraint = TRY_LOCAL(cleanup_name, read_type_expression_primary(allocator, tokens, &current, filename));
      if (!constraint) {
        THROW_LOCAL(cleanup_name, "expected type after 'extends'");
      }
      skip_whitespace(tokens, &current);
    }
    /* 3. Check for optional `: <type>` value generic annotation */
    else {
      token_t colon = vec_get(tokens, current);
      if (colon && token_is(colon, CUBEC_TOKEN_SYMBOL, ":")) {
        current++;
        skip_whitespace(tokens, &current);
        value_type = TRY_LOCAL(cleanup_name, read_type_expression_primary(allocator, tokens, &current, filename));
        if (!value_type) {
          THROW_LOCAL(cleanup_name, "expected type after ':'");
        }
        skip_whitespace(tokens, &current);
      }
    }

    param = TRY_LOCAL(cleanup_vt,
                       allocator_create(allocator, &g_cubec_generic_param_type,
                                        &(cubec_generic_param_init_t){
                                            .name = name,
                                            .constraint = constraint,
                                            .value_type = value_type,
                                            .is_rest = is_rest,
                                        }));
    vec_push(params, param);
    /* Ownership transferred to param — skip cleanup of child nodes */
    goto first_param_done;

  cleanup_vt:
    allocator_free(allocator, &value_type);
  cleanup_name:
    allocator_free(allocator, &constraint);
    allocator_free(allocator, &name);
    goto cleanup_params;

  first_param_done:;
  }

  /* Parse additional params (optional, comma-separated) */
  while (true) {
    skip_whitespace(tokens, &current);
    token_t comma = vec_get(tokens, current);
    if (!comma || !token_is(comma, CUBEC_TOKEN_SYMBOL, ",")) {
      break;
    }
    current++;

    skip_whitespace(tokens, &current);

    node_t name = NULL;
    node_t constraint = NULL;
    node_t value_type = NULL;
    bool is_rest = false;
    param = NULL;

    /* Check for `...` rest prefix */
    if (_is_symbol(tokens, current, "...")) {
      is_rest = true;
      current++;
      skip_whitespace(tokens, &current);
    }

    /* Parse identifier */
    name = TRY_LOCAL(cleanup_params, read_literal_identifier(allocator, tokens, &current, filename));
    if (!name) {
      THROW_LOCAL(cleanup_params, "expected generic parameter name after ','");
    }

    skip_whitespace(tokens, &current);

    /* Check for optional constraint or value type */
    if (_is_keyword(tokens, current, "extends")) {
      current++;
      skip_whitespace(tokens, &current);
      constraint = TRY_LOCAL(cleanup_name_extra,
                             read_type_expression_primary(allocator, tokens, &current, filename));
      if (!constraint) {
        THROW_LOCAL(cleanup_name_extra, "expected type after 'extends'");
      }
      skip_whitespace(tokens, &current);
    } else {
      token_t colon = vec_get(tokens, current);
      if (colon && token_is(colon, CUBEC_TOKEN_SYMBOL, ":")) {
        current++;
        skip_whitespace(tokens, &current);
        value_type = TRY_LOCAL(cleanup_name_extra,
                               read_type_expression_primary(allocator, tokens, &current, filename));
        if (!value_type) {
          THROW_LOCAL(cleanup_name_extra, "expected type after ':'");
        }
        skip_whitespace(tokens, &current);
      }
    }

    param = TRY_LOCAL(cleanup_vt_extra,
                       allocator_create(allocator, &g_cubec_generic_param_type,
                                        &(cubec_generic_param_init_t){
                                            .name = name,
                                            .constraint = constraint,
                                            .value_type = value_type,
                                            .is_rest = is_rest,
                                        }));
    vec_push(params, param);
    /* Ownership transferred to param — skip cleanup of child nodes */
    goto extra_param_done;

  cleanup_vt_extra:
    allocator_free(allocator, &value_type);
  cleanup_name_extra:
    allocator_free(allocator, &constraint);
    allocator_free(allocator, &name);
    goto cleanup_params;

  extra_param_done:;
    continue;
  }

  /* Expect closing ']' */
  {
    token_t rbracket = vec_get(tokens, current);
    if (!rbracket || !token_is(rbracket, CUBEC_TOKEN_SYMBOL, "]")) {
      THROW_LOCAL(cleanup_params, "expected ']' after generic parameters");
    }
    current++;
  }

  *position = current;
  return params;

cleanup_params:
  allocator_free(allocator, &params);
onerror:
  return NULL;
}
