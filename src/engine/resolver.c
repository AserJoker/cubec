#include "engine/resolver.h"
#include "engine/checker.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/node.h"
#include "cubec/literal_identifier.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/declaration_array.h"
#include "cubec/expression_type_qualifier.h"
#include "cubec/expression_type_struct.h"
#include "cubec/expression_type_enum.h"
#include "cubec/expression_type_union.h"
#include "cubec/expression_type_interface.h"
#include "cubec/expression_type_function.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_typeof.h"
#include "cubec/struct_field.h"
#include "cubec/enum_item.h"
#include "cubec/interface_method.h"
#include <string.h>

/* ===== helper: extract identifier string from AST identifier node ===== */

static const char *_ident_str(node_t id_node) {
  if (!id_node) return NULL;
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)id_node;
  return string_get(id->value);
}

/* ===== resolver_resolve_type ===== */

semantic_type_t resolver_resolve_type(checker_t ctx, node_t node) {
  if (!node) return ctx->builtin_void;

  switch (node->kind) {

  /* --- identifier lookup --- */
  case CUBEC_NODE_LITERAL_IDENTIFIER: {
    const char *name = _ident_str(node);
    if (!name) return ctx->error_type;

    /* Search type_name_table first */
    void *found = strmap_find(ctx->type_name_table, name);
    if (found) {
      return (semantic_type_t)found;
    }

    /* Search scope chain */
    struct symbol *sym = scope_lookup(ctx->current_scope, name);
    if (sym && sym->kind == SYMBOL_TYPE && sym->type.type) {
      return sym->type.type;
    }

    /* Unknown type */
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "unknown type '%s'", name);
    ctx->error_count++;
    return ctx->error_type;
  }

  /* --- pointer type --- */
  case CUBEC_NODE_DECLARATION_POINTER: {
    cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
    semantic_type_t base = resolver_resolve_type(ctx, ptr->type);
    if (base->impl->kind == TYPE_ERROR) return ctx->error_type;

    semantic_type_t result = semantic_type_create_pointer(ctx->allocator, base);
    if (ptr->is_const || ptr->is_volatile) {
      semantic_type_t qualified = semantic_type_create_qualifier(
          ctx->allocator, result, ptr->is_volatile);
      type_hash_ensure(qualified);
      return qualified;
    }
    type_hash_ensure(result);
    return result;
  }

  /* --- slice type --- */
  case CUBEC_NODE_DECLARATION_SLICE: {
    cubec_declaration_slice_t sl = (cubec_declaration_slice_t)node;
    semantic_type_t elem = resolver_resolve_type(ctx, sl->type);
    if (elem->impl->kind == TYPE_ERROR) return ctx->error_type;

    semantic_type_t result = semantic_type_create_slice(ctx->allocator, elem);
    if (sl->is_const || sl->is_volatile) {
      semantic_type_t qualified = semantic_type_create_qualifier(
          ctx->allocator, result, sl->is_volatile);
      type_hash_ensure(qualified);
      return qualified;
    }
    type_hash_ensure(result);
    return result;
  }

  /* --- array type --- */
  case CUBEC_NODE_DECLARATION_ARRAY: {
    cubec_declaration_array_t arr = (cubec_declaration_array_t)node;
    semantic_type_t elem = resolver_resolve_type(ctx, arr->type);
    if (elem->impl->kind == TYPE_ERROR) return ctx->error_type;

    /* TODO: evaluate array size expression (comptime) */
    size_t length = 0;
    semantic_type_t result =
        semantic_type_create_array(ctx->allocator, elem, length);
    type_hash_ensure(result);
    return result;
  }

  /* --- type qualifier (const/volatile) --- */
  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER: {
    cubec_expression_type_qualifier_t q =
        (cubec_expression_type_qualifier_t)node;
    semantic_type_t base = resolver_resolve_type(ctx, q->type);
    if (base->impl->kind == TYPE_ERROR) return ctx->error_type;

    semantic_type_t result = semantic_type_create_qualifier(
        ctx->allocator, base, q->is_volatile);
    type_hash_ensure(result);
    return result;
  }

  /* --- anonymous struct type --- */
  case CUBEC_NODE_EXPRESSION_TYPE_STRUCT: {
    cubec_expression_type_struct_t st =
        (cubec_expression_type_struct_t)node;
    semantic_type_t t =
        semantic_type_create_named(ctx->allocator, NULL, TYPE_STRUCT);

    vec_init_t vi = {.auto_dispose = false};
    t->impl->struct_type.fields =
        (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

    if (st->members) {
      size_t count = vec_get_size(st->members);
      for (size_t i = 0; i < count; i++) {
        node_t member = (node_t)vec_get(st->members, i);
        if (!member) continue;

        if (member->kind == CUBEC_NODE_STRUCT_FIELD) {
          cubec_struct_field_t field = (cubec_struct_field_t)member;
          const char *fname = _ident_str(field->name);
          struct symbol *fsym = symbol_create(ctx->allocator, fname,
                                               SYMBOL_FIELD, field->super.location);
          if (field->type) {
            fsym->field.type = resolver_resolve_type(ctx, field->type);
          }
          fsym->field.index = i;
          fsym->field.is_pub = field->is_pub;
          vec_push(t->impl->struct_type.fields, fsym);
        }
      }
    }

    type_layout_compute(t, 8);
    type_hash_ensure(t);
    return t;
  }

  /* --- anonymous enum type --- */
  case CUBEC_NODE_EXPRESSION_TYPE_ENUM: {
    cubec_expression_type_enum_t en =
        (cubec_expression_type_enum_t)node;
    semantic_type_t t =
        semantic_type_create_named(ctx->allocator, NULL, TYPE_ENUM);

    vec_init_t vi = {.auto_dispose = false};
    t->impl->enum_type.items =
        (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
    t->impl->enum_type.backing_type = ctx->builtin_i32;

    if (en->items) {
      size_t count = vec_get_size(en->items);
      long long auto_val = 0;
      for (size_t i = 0; i < count; i++) {
        node_t item_node = (node_t)vec_get(en->items, i);
        if (!item_node || item_node->kind != CUBEC_NODE_ENUM_ITEM) continue;

        cubec_enum_item_t item = (cubec_enum_item_t)item_node;
        const char *iname = _ident_str(item->name);
        struct symbol *isym = symbol_create(ctx->allocator, iname,
                                             SYMBOL_ENUM_ITEM,
                                             item->super.location);
        isym->enum_item.owning_type = t;
        /* TODO: evaluate item->value (comptime) */
        isym->enum_item.value = auto_val++;
        vec_push(t->impl->enum_type.items, isym);
      }
    }

    type_layout_compute(t, 8);
    type_hash_ensure(t);
    return t;
  }

  /* --- anonymous union type --- */
  case CUBEC_NODE_EXPRESSION_TYPE_UNION: {
    cubec_expression_type_union_t un =
        (cubec_expression_type_union_t)node;
    semantic_type_t t =
        semantic_type_create_named(ctx->allocator, NULL, TYPE_UNION);

    vec_init_t vi = {.auto_dispose = false};
    t->impl->struct_type.fields =
        (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

    if (un->members) {
      size_t count = vec_get_size(un->members);
      for (size_t i = 0; i < count; i++) {
        node_t member = (node_t)vec_get(un->members, i);
        if (!member) continue;

        if (member->kind == CUBEC_NODE_STRUCT_FIELD) {
          cubec_struct_field_t field = (cubec_struct_field_t)member;
          const char *fname = _ident_str(field->name);
          struct symbol *fsym = symbol_create(ctx->allocator, fname,
                                               SYMBOL_FIELD,
                                               field->super.location);
          if (field->type) {
            fsym->field.type = resolver_resolve_type(ctx, field->type);
          }
          fsym->field.index = i;
          vec_push(t->impl->struct_type.fields, fsym);
        }
      }
    }

    type_layout_compute(t, 8);
    type_hash_ensure(t);
    return t;
  }

  /* --- interface type --- */
  case CUBEC_NODE_EXPRESSION_TYPE_INTERFACE: {
    cubec_expression_type_interface_t iface =
        (cubec_expression_type_interface_t)node;
    semantic_type_t t =
        semantic_type_create_named(ctx->allocator, NULL, TYPE_INTERFACE);
    t->is_interface = true;

    vec_init_t vi = {.auto_dispose = false};
    t->impl->interface_type.methods =
        (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

    if (iface->members) {
      size_t count = vec_get_size(iface->members);
      for (size_t i = 0; i < count; i++) {
        node_t member = (node_t)vec_get(iface->members, i);
        if (!member) continue;

        if (member->kind == CUBEC_NODE_INTERFACE_METHOD) {
          cubec_interface_method_t method = (cubec_interface_method_t)member;
          const char *mname = _ident_str(method->name);
          struct symbol *msym = symbol_create(ctx->allocator, mname,
                                               SYMBOL_FUNCTION,
                                               method->super.location);
          msym->state = SYMBOL_NAME_KNOWN;
          vec_push(t->impl->interface_type.methods, msym);
        }
      }
    }

    type_hash_ensure(t);
    return t;
  }

  /* --- function type --- */
  case CUBEC_NODE_EXPRESSION_TYPE_FUNCTION: {
    cubec_expression_type_function_t ft =
        (cubec_expression_type_function_t)node;

    semantic_type_t ret_type = ft->return_type
        ? resolver_resolve_type(ctx, ft->return_type)
        : ctx->builtin_void;

    vec_init_t vi = {.auto_dispose = false};
    vec_t params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

    if (ft->parameters) {
      size_t count = vec_get_size(ft->parameters);
      for (size_t i = 0; i < count; i++) {
        node_t param = (node_t)vec_get(ft->parameters, i);
        semantic_type_t pt = resolver_resolve_type(ctx, param);
        vec_push(params, pt);
      }
    }

    semantic_type_t result = semantic_type_create_function(
        ctx->allocator, ret_type, params, ft->is_c_variadic);
    type_hash_ensure(result);
    return result;
  }

  /* --- namespace access (::) --- */
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS: {
    cubec_expression_namespace_access_t ns =
        (cubec_expression_namespace_access_t)node;
    const char *field_name = _ident_str((node_t)ns->field);
    if (!field_name) return ctx->error_type;

    semantic_type_t host_type = resolver_resolve_type(ctx, ns->host);
    if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

    /* Look up associated type */
    size_t count = vec_get_size(host_type->associated_types);
    for (size_t i = 0; i < count; i++) {
      struct symbol *sym = (struct symbol *)vec_get(host_type->associated_types, i);
      if (strcmp(sym->name, field_name) == 0 && sym->type.type) {
        return sym->type.type;
      }
    }

    /* Look up static scope */
    struct symbol *sym = scope_lookup_static(ctx->current_scope, field_name);
    if (sym && sym->kind == SYMBOL_TYPE && sym->type.type) {
      return sym->type.type;
    }

    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "type '%s' has no member '%s'",
                         host_type->name ? host_type->name : "<anonymous>",
                         field_name);
    ctx->error_count++;
    return ctx->error_type;
  }

  /* --- typeof --- */
  case CUBEC_NODE_EXPRESSION_TYPEOF: {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "typeof not yet supported in type resolution");
    ctx->error_count++;
    return ctx->error_type;
  }

  default:
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "invalid type expression (node kind %d)", node->kind);
    ctx->error_count++;
    return ctx->error_type;
  }
}
