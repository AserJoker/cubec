#include "cubec/declaration_union.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "core/token_writer.h"
#include "cubec/expression_spread.h"
#include "cubec/generic_param.h"
#include "cubec/node_error.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include "cubec/union_field.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void
_cubec_declaration_union_init(cubec_declaration_union_t self,
                                  allocator_t allocator,
                                  cubec_declaration_union_init_t *init) {
  if (!init)
    return;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_DECLARATION_UNION,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_class.init(&self->super, allocator, &super_init);
  self->generic_params = init->generic_params;
  self->members = init->members;
}

static void
_cubec_declaration_union_dispose(cubec_declaration_union_t self,
                                     allocator_t allocator) {
  allocator_free(allocator, &self->members);
  allocator_free(allocator, &self->generic_params);
  g_cubec_expression_class.dispose(&self->super, allocator);
}

static void
_cubec_declaration_union_clone(cubec_declaration_union_t self,
                                   allocator_t allocator,
                                   cubec_declaration_union_t another) {
  g_cubec_expression_class.clone(&self->super, allocator, &another->super);
  self->generic_params = another->generic_params
                             ? alloc_clone(allocator, another->generic_params)
                             : NULL;
  self->members = alloc_clone(allocator, another->members);
  return;
}

static void
_cubec_declaration_union_move(cubec_declaration_union_t self,
                                  allocator_t allocator,
                                  cubec_declaration_union_t another) {
  g_cubec_expression_class.move(&self->super, allocator, &another->super);
  self->generic_params = another->generic_params
                             ? alloc_move(allocator, another->generic_params)
                             : NULL;
  self->members = alloc_move(allocator, another->members);
  return;
}

class_t g_cubec_declaration_union_class = {
    .name = "cubec.cubec.declaration_union",
    .size = sizeof(struct _cubec_declaration_union_t),
    .init = (class_init_fn_t)_cubec_declaration_union_init,
    .dispose = (class_dispose_fn_t)_cubec_declaration_union_dispose,
    .clone = (class_clone_fn_t)_cubec_declaration_union_clone,
    .move = (class_move_fn_t)_cubec_declaration_union_move,
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
 *  Internal: parse union body after 'union' keyword consumed
 *            [generic_params] { members }
 * -------------------------------------------------------------------------- */

node_t read_declaration_union_body(context_t ctx, vec_t tokens,
                                       size_t *position, const char *filename,
                                       location_t start_location,
                                       vec_t *out_implements) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  vec_t generic_params = NULL;
  vec_t members = NULL;
  vec_t implements = NULL;
  cubec_declaration_union_t node = NULL;

  /* 1. Parse optional generic parameters */
  generic_params = read_generic_params(ctx, tokens, &current, filename);
  if (generic_params) {
    skip_whitespace(tokens, &current);
  }

  /* 1b. Parse optional 'implement' clause (statement form only) */
  if (out_implements && _is_keyword(tokens, current, "implement")) {
    current++;
    skip_whitespace(tokens, &current);
    node_t iface_expr =
        read_type_expression_primary(ctx, tokens, &current, filename);
    if (node_is_error(iface_expr))
      goto onerror;
    if (!iface_expr) {
      goto cleanup;
    }
    implements = allocator_create(allocator, &g_vec_class, &(vec_init_t){true});
    vec_push(implements, iface_expr);
    skip_whitespace(tokens, &current);
    while (_is_symbol(tokens, current, ",")) {
      current++;
      skip_whitespace(tokens, &current);
      iface_expr =
          read_type_expression_primary(ctx, tokens, &current, filename);
      if (node_is_error(iface_expr))
        goto onerror;
      if (!iface_expr) {
        goto cleanup;
      }
      vec_push(implements, iface_expr);
      skip_whitespace(tokens, &current);
    }
  }

  /* 2. Expect '{' */
  if (!_is_symbol(tokens, current, "{")) {
    goto cleanup;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse members — union fields, spread, statements (func/var/type/etc.) */
  members = allocator_create(allocator, &g_vec_class, &(vec_init_t){true});
  while (!_is_symbol(tokens, current, "}")) {
    node_t member = NULL;

    /* Try spread: ...expr ; */
    token_t tok = vec_get(tokens, current);
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, "...")) {
      member = read_expression_spread(ctx, tokens, &current, filename);
      if (node_is_error(member))
        goto onerror;
      if (!member) {
        break;
      }
      skip_whitespace(tokens, &current);
      /* Expect ';' after spread */
      token_t semi = vec_get(tokens, current);
      if (!token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
        goto cleanup;
      }
      current++;
    }

    /* Try union field: <identifier> : <type> */
    if (!member) {
      member = read_union_field(ctx, tokens, &current, filename);
      if (node_is_error(member))
        goto onerror;
    }

    /* Try statement (var, type, func, struct, interface, etc.) */
    if (!member) {
      member = read_statement(ctx, tokens, &current, filename);
      if (node_is_error(member))
        goto onerror;
    }

    if (!member) {
      break;
    }

    vec_push(members, member);
    skip_whitespace(tokens, &current);
  }

  /* 4. Expect '}' */
  if (!_is_symbol(tokens, current, "}")) {
    token_t tok = vec_get(tokens, current);
    (void)tok;
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

  cubec_declaration_union_init_t init = {
      .location = loc,
      .parent = NULL,
      .generic_params = generic_params,
      .members = members,
  };
  node =
      allocator_create(allocator, &g_cubec_declaration_union_class, &init);
  if (out_implements)
    *out_implements = implements;
  *position = current;
  return (node_t)&node->super;

cleanup:
  allocator_free(allocator, &implements);
  allocator_free(allocator, &members);
  allocator_free(allocator, &generic_params);
  allocator_free(allocator, &node);
  return NULL;
onerror:
  allocator_free(allocator, &implements);
  allocator_free(allocator, &members);
  allocator_free(allocator, &generic_params);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Parser: read_declaration_union — entry point for type expressions
 * -------------------------------------------------------------------------- */

node_t read_declaration_union(context_t ctx, vec_t tokens, size_t *position,
                                  const char *filename) {
  allocator_t allocator = ctx->allocator;
  (void)allocator;
  size_t current = *position;

  /* Expect 'union' keyword — but NOT 'cunion' */
  if (!_is_keyword(tokens, current, "union")) {
    return NULL;
  }
  token_t union_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(union_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  node_t result = read_declaration_union_body(
      ctx, tokens, &current, filename, start_location, NULL);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                       "invalid union type expression");
  return create_error(ctx, start_location);
}

/* --------------------------------------------------------------------------
 *  Factory: create_declaration_union
 * -------------------------------------------------------------------------- */

node_t create_declaration_union(context_t ctx, location_t loc,
                                    vec_t generic_params, vec_t members) {
  allocator_t alloc = ctx->allocator;
  cubec_declaration_union_init_t init = {
      .location = loc,
      .parent = NULL,
      .generic_params = generic_params,
      .members = members,
  };
  return (node_t)allocator_create(alloc, &g_cubec_declaration_union_class,
                                  &init);
}

/* --------------------------------------------------------------------------
 *  Writer: write_declaration_union
 * -------------------------------------------------------------------------- */

void emit_declaration_union(emit_context_t ctx, node_t node) {
  cubec_declaration_union_t un = (cubec_declaration_union_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "union");
  if (un->generic_params) {
    emit_symbol(ctx, "[");
    for (size_t i = 0; i < vec_get_size(un->generic_params); i++) {
      if (i != 0) {
        emit_symbol(ctx, ",");
        emit_space(ctx);
      }
      emit_generic_param(ctx, vec_get(un->generic_params, i));
    }
    emit_symbol(ctx, "]");
  }
  emit_space(ctx);
  emit_symbol(ctx, "{");
  if (vec_get_size(un->members)) {
    emit_newline(ctx);
    emit_indent(ctx, +1);
    for (size_t i = 0; i < vec_get_size(un->members); i++) {
      recover_comments_to(ctx, ((node_t)vec_get(un->members, i))->location.begin.offset);
      emit_statement(ctx, vec_get(un->members, i));
      emit_newline(ctx);
    }
    emit_indent(ctx, -1);
  }
  emit_symbol(ctx, "}");
}
