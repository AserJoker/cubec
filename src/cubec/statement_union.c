#include "cubec/statement_union.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/decorator.h"
#include "cubec/declaration_union.h"
#include "cubec/union_field.h"
#include "cubec/expression.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_union_init(cubec_statement_union_t self,
                                        allocator_t allocator,
                                        cubec_statement_union_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_UNION,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->is_export = init->is_export;
  self->name = init->name;
  self->generic_params = init->generic_params;
  self->implements = init->implements;
  self->members = init->members;
  self->decorators = init->decorators;
}

static void _cubec_statement_union_dispose(cubec_statement_union_t self,
                                           allocator_t allocator) {
  allocator_free(allocator, &self->decorators);
  allocator_free(allocator, &self->implements);
  allocator_free(allocator, &self->members);
  allocator_free(allocator, &self->generic_params);
  allocator_free(allocator, &self->name);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_union_clone(cubec_statement_union_t self,
                                         allocator_t allocator,
                                         cubec_statement_union_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->name = alloc_clone(allocator, another->name);
  self->generic_params = another->generic_params
                             ? alloc_clone(allocator, another->generic_params)
                             : NULL;
  self->implements =
      another->implements ? alloc_clone(allocator, another->implements) : NULL;
  self->members = alloc_clone(allocator, another->members);
  return;
}

static void _cubec_statement_union_move(cubec_statement_union_t self,
                                        allocator_t allocator,
                                        cubec_statement_union_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->name = alloc_move(allocator, another->name);
  self->generic_params = another->generic_params
                             ? alloc_move(allocator, another->generic_params)
                             : NULL;
  self->implements =
      another->implements ? alloc_move(allocator, another->implements) : NULL;
  self->members = alloc_move(allocator, another->members);
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
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

/* --------------------------------------------------------------------------
 *  Parser: read_statement_union — delegates to read_declaration_union
 * -------------------------------------------------------------------------- */

node_t read_statement_union(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename) {
  allocator_t allocator = ctx->allocator;
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

  /* 1. Parse optional 'export' modifier */
  if (_is_keyword(tokens, current, "export")) {
    is_export = true;
    token_t tok = vec_get(tokens, current);
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
    token_t tok = vec_get(tokens, current);
    start_location = *token_get_location(tok);
    start_location.filename = filename;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse union name (required for statement form) */
  name = read_literal_identifier(ctx, tokens, &current, filename);
  if (node_is_error(name)) {
    allocator_free(allocator, &decorators);
    return name;
  }
  if (!name)
    goto onerror;

  skip_whitespace(tokens, &current);

  /* 4. Delegate to read_declaration_union_body for [generic_params] {
   * members } */
  expr_node = read_declaration_union_body(ctx, tokens, &current, filename,
                                              start_location, &implements);
  if (node_is_error(expr_node)) {
    allocator_free(allocator, &decorators);
    allocator_free(allocator, &implements);
    allocator_free(allocator, &name);
    return expr_node;
  }
  if (!expr_node)
    goto onerror;
  cubec_declaration_union_t expr_union =
      (cubec_declaration_union_t)expr_node;

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

  node = allocator_create(allocator, &g_cubec_statement_union_type, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &implements);
  allocator_free(allocator, &name);
  allocator_free(allocator, &expr_node);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

node_t create_statement_union(context_t ctx, location_t loc, const char *name,
                              vec_t members, bool is_export, vec_t implements,
                              vec_t decorators) {
  allocator_t alloc = ctx->allocator;
  node_t name_node = create_literal_identifier(ctx, loc, name);
  cubec_statement_union_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .name = name_node,
      .generic_params = NULL,
      .implements = implements,
      .members = members,
      .decorators = decorators,
  };
  return (node_t)allocator_create(alloc, &g_cubec_statement_union_type, &init);
}

void emit_statement_union(emit_context_t ctx, node_t node) {
  cubec_statement_union_t stmt = (cubec_statement_union_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  if (stmt->decorators) {
    for (size_t i = 0; i < vec_get_size(stmt->decorators); i++) {
      recover_comments_to(ctx, ((node_t)vec_get(stmt->decorators, i))->location.begin.offset);
      emit_decorator(ctx, vec_get(stmt->decorators, i));
      emit_newline(ctx);
    }
  }
  if (stmt->is_export) {
    emit_keyword(ctx, "export");
    emit_space(ctx);
  }
  emit_keyword(ctx, "union");
  emit_space(ctx);
  emit_expression(ctx, stmt->name);
  if (stmt->generic_params) {
    emit_symbol(ctx, "[");
    for (size_t i = 0; i < vec_get_size(stmt->generic_params); i++) {
      if (i != 0) {
        emit_symbol(ctx, ",");
        emit_space(ctx);
      }
      emit_generic_param(ctx, vec_get(stmt->generic_params, i));
    }
    emit_symbol(ctx, "]");
  }
  if (stmt->implements) {
    emit_space(ctx);
    emit_keyword(ctx, "implement");
    emit_space(ctx);
    for (size_t i = 0; i < vec_get_size(stmt->implements); i++) {
      if (i != 0) {
        emit_symbol(ctx, ",");
        emit_space(ctx);
      }
      emit_expression(ctx, vec_get(stmt->implements, i));
    }
  }
  emit_space(ctx);
  emit_symbol(ctx, "{");
  if (vec_get_size(stmt->members)) {
    emit_indent(ctx, +1);
    emit_newline(ctx);
    size_t count = vec_get_size(stmt->members);
    for (size_t i = 0; i < count; i++) {
      recover_comments_to(ctx, ((node_t)vec_get(stmt->members, i))->location.begin.offset);
      emit_union_field(ctx, vec_get(stmt->members, i));
      if (i + 1 < count) {
        emit_newline(ctx);
      }
    }
    emit_indent(ctx, -1);
    emit_newline(ctx);
  } else {
    emit_newline(ctx);
  }
  emit_symbol(ctx, "}");
}
