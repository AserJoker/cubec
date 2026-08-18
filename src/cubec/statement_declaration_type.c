#include "cubec/statement_declaration_type.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "core/token_writer.h"
#include "cubec/decorator.h"
#include "cubec/expression.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

static void _cubec_statement_declaration_type_init(
    cubec_statement_declaration_type_t self, allocator_t allocator,
    cubec_statement_declaration_type_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_DECLARATION_TYPE,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_class.init(&self->super, allocator, &super_init);
  self->is_export = init->is_export;
  self->is_builtin = init->is_builtin;
  self->name = init->name;
  self->params = init->params;
  self->type_value = init->type_value;
  self->decorators = init->decorators;
}

static void _cubec_statement_declaration_type_dispose(
    cubec_statement_declaration_type_t self, allocator_t allocator) {
  allocator_free(allocator, &self->decorators);
  allocator_free(allocator, &self->type_value);
  allocator_free(allocator, &self->params);
  allocator_free(allocator, &self->name);
  g_node_class.dispose(&self->super, allocator);
}

static void _cubec_statement_declaration_type_clone(
    cubec_statement_declaration_type_t self, allocator_t allocator,
    cubec_statement_declaration_type_t another) {
  g_node_class.clone(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->is_builtin = another->is_builtin;
  self->name = alloc_clone(allocator, another->name);
  self->params = alloc_clone(allocator, another->params);
  self->type_value =
      another->type_value ? alloc_clone(allocator, another->type_value) : NULL;
  return;
}

static void _cubec_statement_declaration_type_move(
    cubec_statement_declaration_type_t self, allocator_t allocator,
    cubec_statement_declaration_type_t another) {
  g_node_class.move(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->is_builtin = another->is_builtin;
  self->name = alloc_move(allocator, another->name);
  self->params = alloc_move(allocator, another->params);
  self->type_value =
      another->type_value ? alloc_move(allocator, another->type_value) : NULL;
  return;
}

class_t g_cubec_statement_decltype = {
    .name = "cubec.cubec.statement_declaration_type",
    .size = sizeof(struct _cubec_statement_declaration_type_t),
    .init = (class_init_fn_t)_cubec_statement_declaration_type_init,
    .dispose = (class_dispose_fn_t)_cubec_statement_declaration_type_dispose,
    .clone = (class_clone_fn_t)_cubec_statement_declaration_type_clone,
    .move = (class_move_fn_t)_cubec_statement_declaration_type_move,
};

/**
 * @brief Check if a token is a specific keyword.
 */
static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

node_t read_statement_declaration_type(vm_t vm, vec_t tokens,
                                       size_t *position, const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;
  cubec_statement_declaration_type_t node = NULL;
  node_t name = NULL;
  vec_t params = NULL;
  node_t type_value = NULL;
  location_t start_location = {0};
  bool is_export = false;
  bool is_builtin = false;
  vec_t decorators = NULL;

  /* Collect decorators [[...]] */
  {
    while (true) {
      skip_whitespace(tokens, &current);
      node_t dec = read_decorator(vm, tokens, &current, filename);
      if (node_is_error(dec))
        return dec;
      if (!dec)
        break;
      if (!decorators) {
        decorators =
            allocator_create(allocator, &g_vec_class, &(vec_init_t){true});
      }
      vec_push(decorators, dec);
    }
  }

  /* 1. Parse optional modifiers: export / builtin */
  while (true) {
    if (_is_keyword(tokens, current, "export")) {
      if (is_export)
        goto onerror;
      is_export = true;
      if (start_location.begin.offset == 0) {
        token_t tok = vec_get(tokens, current);
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else if (_is_keyword(tokens, current, "builtin")) {
      if (is_builtin)
        goto onerror;
      is_builtin = true;
      if (start_location.begin.offset == 0) {
        token_t tok = vec_get(tokens, current);
        start_location = *token_get_location(tok);
        start_location.filename = filename;
      }
      current++;
      skip_whitespace(tokens, &current);
    } else {
      break;
    }
  }

  /* 2. Expect 'type' keyword */
  if (!_is_keyword(tokens, current, "type")) {
    goto onerror;
  }
  token_t type_token = vec_get(tokens, current);
  if (start_location.begin.offset == 0) {
    start_location = *token_get_location(type_token);
    start_location.filename = filename;
  }
  current++;

  skip_whitespace(tokens, &current);

  /* 3. Parse type alias name (required) */
  name = read_literal_identifier(vm, tokens, &current, filename);
  if (node_is_error(name)) {
    allocator_free(allocator, &decorators);
    return name;
  }
  if (!name)
    goto onerror;

  skip_whitespace(tokens, &current);

  /* 4. Parse optional generic parameters */
  params = read_generic_params(vm, tokens, &current, filename);

  if (params) {
    skip_whitespace(tokens, &current);
  }

  /* 5. Parse optional '= type_expression' (required for non-builtin, absent for
   * builtin) */
  if (!is_builtin) {
    token_t eq = vec_get(tokens, current);
    if (!eq || !token_is(eq, CUBEC_TOKEN_SYMBOL, "=")) {
      goto onerror;
    }
    current++;

    skip_whitespace(tokens, &current);

    /* Parse type expression (no comma/assignment — terminated by ';') */
    type_value = read_expression_base(vm, tokens, &current, filename);
    if (node_is_error(type_value)) {
      allocator_free(allocator, &decorators);
      allocator_free(allocator, &params);
      allocator_free(allocator, &name);
      return type_value;
    }
    if (!type_value)
      goto onerror;

    skip_whitespace(tokens, &current);
  }

  /* 6. Expect ';' */
  token_t semi = vec_get(tokens, current);
  if (!semi || !token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    goto onerror;
  }
  current++;

  /* 7. Build location spanning from first modifier or 'type' to ';' */
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_location.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_statement_declaration_type_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .is_builtin = is_builtin,
      .name = name,
      .params = params,
      .type_value = type_value,
      .decorators = decorators,
  };
  node = allocator_create(allocator, &g_cubec_statement_decltype, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &type_value);
  allocator_free(allocator, &params);
  allocator_free(allocator, &name);
  allocator_free(allocator, &node);
  return create_error(vm, start_location);
}

node_t create_statement_declaration_type(vm_t vm, location_t loc,
                                         const char *name, node_t type_value,
                                         bool is_export, bool is_builtin,
                                         vec_t decorators) {
  allocator_t alloc = vm_get_allocator(vm);
  node_t name_node = create_literal_identifier(vm, loc, name);
  cubec_statement_declaration_type_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .is_builtin = is_builtin,
      .name = name_node,
      .params = NULL,
      .type_value = type_value,
      .decorators = decorators,
  };
  return (node_t)allocator_create(alloc, &g_cubec_statement_decltype, &init);
}

void emit_statement_declaration_type(emit_context_t ctx, node_t node) {
  cubec_statement_declaration_type_t decl = (cubec_statement_declaration_type_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  if (decl->decorators) {
    for (size_t i = 0; i < vec_get_size(decl->decorators); i++) {
      recover_comments_to(ctx, ((node_t)vec_get(decl->decorators, i))->location.begin.offset);
      emit_decorator(ctx, vec_get(decl->decorators, i));
      emit_newline(ctx);
    }
  }
  if (decl->is_export) {
    emit_keyword(ctx, "export");
    emit_space(ctx);
  }
  if (decl->is_builtin) {
    emit_keyword(ctx, "builtin");
    emit_space(ctx);
  }
  emit_keyword(ctx, "type");
  emit_space(ctx);
  emit_expression(ctx, decl->name);
  if (decl->params) {
    emit_symbol(ctx, "[");
    for (size_t i = 0; i < vec_get_size(decl->params); i++) {
      if (i != 0) {
        emit_symbol(ctx, ",");
        emit_space(ctx);
      }
      emit_generic_param(ctx, vec_get(decl->params, i));
    }
    emit_symbol(ctx, "]");
  }
  if (decl->type_value) {
    emit_space(ctx);
    emit_symbol(ctx, "=");
    emit_space(ctx);
    emit_expression(ctx, decl->type_value);
  }
  emit_symbol(ctx, ";");
}
