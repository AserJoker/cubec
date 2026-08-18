#include "cubec/statement_interface.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/decorator.h"
#include "cubec/declaration_interface.h"
#include "cubec/interface_method.h"
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

static void
_cubec_statement_interface_init(cubec_statement_interface_t self,
                                allocator_t allocator,
                                cubec_statement_interface_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_INTERFACE,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_class.init(&self->super, allocator, &super_init);
  self->is_export = init->is_export;
  self->name = init->name;
  self->generic_params = init->generic_params;
  self->members = init->members;
  self->decorators = init->decorators;
}

static void _cubec_statement_interface_dispose(cubec_statement_interface_t self,
                                               allocator_t allocator) {
  allocator_free(allocator, &self->decorators);
  allocator_free(allocator, &self->members);
  allocator_free(allocator, &self->generic_params);
  allocator_free(allocator, &self->name);
  g_node_class.dispose(&self->super, allocator);
}

static void
_cubec_statement_interface_clone(cubec_statement_interface_t self,
                                 allocator_t allocator,
                                 cubec_statement_interface_t another) {
  g_node_class.clone(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->name = alloc_clone(allocator, another->name);
  self->generic_params = another->generic_params
                             ? alloc_clone(allocator, another->generic_params)
                             : NULL;
  self->members = alloc_clone(allocator, another->members);
  return;
}

static void
_cubec_statement_interface_move(cubec_statement_interface_t self,
                                allocator_t allocator,
                                cubec_statement_interface_t another) {
  g_node_class.move(&self->super, allocator, &another->super);
  self->is_export = another->is_export;
  self->name = alloc_move(allocator, another->name);
  self->generic_params = another->generic_params
                             ? alloc_move(allocator, another->generic_params)
                             : NULL;
  self->members = alloc_move(allocator, another->members);
  return;
}

class_t g_cubec_statement_interface_class = {
    .name = "cubec.cubec.statement_interface",
    .size = sizeof(struct _cubec_statement_interface_t),
    .init = (class_init_fn_t)_cubec_statement_interface_init,
    .dispose = (class_dispose_fn_t)_cubec_statement_interface_dispose,
    .clone = (class_clone_fn_t)_cubec_statement_interface_clone,
    .move = (class_move_fn_t)_cubec_statement_interface_move,
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
 *  Parser: read_statement_interface — delegates to
 * read_declaration_interface
 * -------------------------------------------------------------------------- */

node_t read_statement_interface(vm_t vm, vec_t tokens, size_t *position,
                                const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
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
  name = read_literal_identifier(vm, tokens, &current, filename);
  if (node_is_error(name)) {
    allocator_free(allocator, &decorators);
    return name;
  }
  if (!name)
    goto onerror;

  skip_whitespace(tokens, &current);

  /* 4. Delegate to read_declaration_interface_body for [generic_params] {
   * members } (interface keyword already consumed, pass start_location for
   * span) */
  expr_node = read_declaration_interface_body(vm, tokens, &current,
                                                  filename, start_location);
  if (node_is_error(expr_node)) {
    allocator_free(allocator, &decorators);
    allocator_free(allocator, &name);
    return expr_node;
  }
  if (!expr_node)
    goto onerror;
  cubec_declaration_interface_t expr_iface =
      (cubec_declaration_interface_t)expr_node;

  /* 5. Build location (use modifier start or interface keyword location) */
  location_t loc = expr_node->location;
  if (start_location.begin.offset != 0) {
    loc.begin = start_location.begin;
  }

  /* 6. Create statement_interface node, transferring ownership from
   * declaration_interface */
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

  node = allocator_create(allocator, &g_cubec_statement_interface_class, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &decorators);
  allocator_free(allocator, &name);
  allocator_free(allocator, &expr_node);
  allocator_free(allocator, &node);
  return create_error(vm, start_location);
}

node_t create_statement_interface(vm_t vm, location_t loc,
                                  const char *name, vec_t members,
                                  bool is_export, vec_t decorators) {
  allocator_t alloc = vm_get_allocator(vm);
  node_t name_node = create_literal_identifier(vm, loc, name);
  cubec_statement_interface_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_export = is_export,
      .name = name_node,
      .generic_params = NULL,
      .members = members,
      .decorators = decorators,
  };
  return (node_t)allocator_create(alloc, &g_cubec_statement_interface_class,
                                  &init);
}

void emit_statement_interface(emit_context_t ctx, node_t node) {
  cubec_statement_interface_t stmt = (cubec_statement_interface_t)node;
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
  emit_keyword(ctx, "interface");
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
  emit_space(ctx);
  emit_symbol(ctx, "{");
  if (vec_get_size(stmt->members)) {
    emit_indent(ctx, +1);
    emit_newline(ctx);
    size_t count = vec_get_size(stmt->members);
    for (size_t i = 0; i < count; i++) {
      recover_comments_to(ctx, ((node_t)vec_get(stmt->members, i))->location.begin.offset);
      emit_interface_method(ctx, vec_get(stmt->members, i));
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
