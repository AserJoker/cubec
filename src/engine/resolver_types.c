#include "engine/resolver_types.h"
#include "engine/resolver.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_qualifier.h"
#include "cubec/declaration_struct.h"
#include "cubec/declaration_tuple.h"
#include "cubec/declaration_enum.h"
#include "cubec/declaration_union.h"
#include "cubec/declaration_interface.h"
#include "cubec/declaration_callable.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_typeof.h"
#include "cubec/struct_field.h"
#include "cubec/enum_item.h"
#include "cubec/interface_method.h"
#include "engine/checker_check_expr.h"
#include <string.h>

/* ===== identifier helper ===== */

const char *_resolver_ident_str(node_t id_node) {
  if (!id_node) return NULL;
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)id_node;
  return string_get(id->value);
}

/* ===== type resolution sub-functions ===== */

semantic_type_t _resolve_type_identifier(context_t ctx, node_t node) {
  const char *name = _resolver_ident_str(node);
  if (!name) return ctx->error_type;

  /* Wildcard type: ? in generic type args (e.g. Container[?]) */
  if (strcmp(name, "?") == 0) {
    semantic_type_t wt = semantic_type_create_wildcard(ctx->allocator, false);
    vec_push(ctx->all_types, wt);
    return wt;
  }

  /* Search type_name_table first */
  void *found = strmap_find(ctx->type_name_table, name);
  if (found) {
    return (semantic_type_t)found;
  }

  /* Search scope chain for types or generic params */
  struct symbol *sym = scope_lookup(ctx->current_scope, name);
  if (sym) {
    if (sym->kind == SYMBOL_TYPE && sym->type.type) {
      return sym->type.type;
    }
    /* Handle generic params in type position: create TYPE_GENERIC_PARAM or TYPE_GENERIC_PACK */
    if (sym->kind == SYMBOL_GENERIC_PARAM) {
      if (sym->generic_param.is_rest) {
        semantic_type_t pack_type = semantic_type_create_generic_pack(
            ctx->allocator, name);
        type_hash_ensure(pack_type);
        vec_push(ctx->all_types, pack_type);
        return pack_type;
      }
      semantic_type_t gp_type = semantic_type_create_generic_param(
          ctx->allocator, name,
          sym->generic_param.value_type,
          sym->generic_param.value_type != NULL);
      type_hash_ensure(gp_type);
      vec_push(ctx->all_types, gp_type);
      return gp_type;
    }
    /* SYMBOL_MODULE in type position → return TYPE_MODULE for namespace access */
    if (sym->kind == SYMBOL_MODULE) {
      semantic_type_t mt = semantic_type_create_named(ctx->allocator, name, TYPE_MODULE);
      type_layout_compute(mt, 8);
      type_hash_ensure(mt);
      vec_push(ctx->all_types, mt);
      return mt;
    }
  }

  /* Unknown type */
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                       "unknown type '%s'", name);
  ctx->error_count++;
  return ctx->error_type;
}

semantic_type_t _resolve_type_pointer(context_t ctx, node_t node) {
  cubec_declaration_pointer_t ptr = (cubec_declaration_pointer_t)node;
  semantic_type_t base = resolver_resolve_type(ctx, ptr->type);
  if (base->impl->kind == TYPE_ERROR) return ctx->error_type;

  /* *const T → POINTER(QUALIFIER(const, T)) — pointer to const T (C: const T*)
   * *volatile T → POINTER(QUALIFIER(volatile, T))
   * *const volatile T → POINTER(QUALIFIER(const|volatile, T)) */
  if (ptr->is_const || ptr->is_volatile) {
    semantic_type_t qualified_base = semantic_type_create_qualifier(
        ctx->allocator, base, ptr->is_const, ptr->is_volatile);
    type_hash_ensure(qualified_base);
    vec_push(ctx->all_types, qualified_base);
    base = qualified_base;
  }

  semantic_type_t result = semantic_type_create_pointer(ctx->allocator, base);
  type_hash_ensure(result);
  vec_push(ctx->all_types, result);
  return result;
}

semantic_type_t _resolve_type_slice(context_t ctx, node_t node) {
  cubec_declaration_slice_t sl = (cubec_declaration_slice_t)node;
  semantic_type_t elem = resolver_resolve_type(ctx, sl->type);
  if (elem->impl->kind == TYPE_ERROR) return ctx->error_type;

  /* []const T → SLICE(QUALIFIER(const, T)) — slice of const T */
  if (sl->is_const || sl->is_volatile) {
    semantic_type_t qualified_elem = semantic_type_create_qualifier(
        ctx->allocator, elem, sl->is_const, sl->is_volatile);
    type_hash_ensure(qualified_elem);
    vec_push(ctx->all_types, qualified_elem);
    elem = qualified_elem;
  }

  semantic_type_t result = semantic_type_create_slice(ctx->allocator, elem);
  type_hash_ensure(result);
  vec_push(ctx->all_types, result);
  return result;
}

semantic_type_t _resolve_type_array(context_t ctx, node_t node) {
  cubec_declaration_array_t arr = (cubec_declaration_array_t)node;
  semantic_type_t elem = resolver_resolve_type(ctx, arr->type);
  if (elem->impl->kind == TYPE_ERROR) return ctx->error_type;

  /* Evaluate array size expression */
  size_t length = 0;
  if (arr->size) {
    if (arr->size->kind == CUBEC_NODE_LITERAL_NUMERIC) {
      cubec_literal_numeric_t num = (cubec_literal_numeric_t)arr->size;
      const char *numstr = string_get(num->value);
      long long val = numstr ? atoll(numstr) : 0;
      if (val < 0) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             arr->size->location,
                             "array size must be non-negative");
        ctx->error_count++;
        val = 0;
      }
      length = (size_t)val;
    } else {
      /* Try to resolve as a value generic param identifier */
      const char *size_name = NULL;
      if (arr->size->kind == CUBEC_NODE_LITERAL_IDENTIFIER)
        size_name = _resolver_ident_str(arr->size);
      struct symbol *size_sym = size_name
          ? scope_lookup(ctx->current_scope, size_name) : NULL;

      if (size_sym && size_sym->kind == SYMBOL_GENERIC_PARAM &&
          size_sym->generic_param.value_type) {
        /* Value generic param used as array length */
        semantic_type_t gp_type = semantic_type_create_generic_param(
            ctx->allocator, size_name,
            size_sym->generic_param.value_type, true);
        type_hash_ensure(gp_type);
        vec_push(ctx->all_types, gp_type);

        semantic_type_t result = semantic_type_create_array(
            ctx->allocator, elem, 0, size_name);
        type_hash_ensure(result);
        vec_push(ctx->all_types, result);
        return result;
      }

      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           arr->size->location,
                           "array size must be a compile-time integer or generic parameter");
      ctx->error_count++;
    }
  }

  semantic_type_t result =
      semantic_type_create_array(ctx->allocator, elem, length, NULL);
  type_hash_ensure(result);
  vec_push(ctx->all_types, result);
  return result;
}

semantic_type_t _resolve_type_qualifier(context_t ctx, node_t node) {
  cubec_declaration_qualifier_t q =
      (cubec_declaration_qualifier_t)node;
  semantic_type_t base = resolver_resolve_type(ctx, q->type);
  if (base->impl->kind == TYPE_ERROR) return ctx->error_type;

  semantic_type_t result = semantic_type_create_qualifier(
      ctx->allocator, base, q->is_const, q->is_volatile);
  type_hash_ensure(result);
  vec_push(ctx->all_types, result);
  return result;
}

semantic_type_t _resolve_type_struct(context_t ctx, node_t node) {
  cubec_declaration_struct_t st =
      (cubec_declaration_struct_t)node;
  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, NULL, TYPE_STRUCT);
  vec_push(ctx->all_types, t);

  vec_init_t vi = {.auto_dispose = true};
  t->impl->struct_type.fields =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  if (st->members) {
    size_t count = vec_get_size(st->members);
    for (size_t i = 0; i < count; i++) {
      node_t member = (node_t)vec_get(st->members, i);
      if (!member) continue;

      if (member->kind == CUBEC_NODE_STRUCT_FIELD) {
        cubec_struct_field_t field = (cubec_struct_field_t)member;
        const char *fname = _resolver_ident_str(field->name);
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

semantic_type_t _resolve_type_tuple(context_t ctx, node_t node) {
  cubec_declaration_tuple_t tt =
      (cubec_declaration_tuple_t)node;

  vec_init_t vi = {.auto_dispose = false};
  vec_t element_types = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  if (tt->element_types) {
    size_t count = vec_get_size(tt->element_types);
    for (size_t i = 0; i < count; i++) {
      node_t elem_node = (node_t)vec_get(tt->element_types, i);
      if (!elem_node) continue;
      semantic_type_t resolved = resolver_resolve_type(ctx, elem_node);
      vec_push(element_types, resolved);
    }
  }

  semantic_type_t t = semantic_type_create_tuple(ctx->allocator, element_types);
  vec_push(ctx->all_types, t);
  type_hash_ensure(t);
  return t;
}

semantic_type_t _resolve_type_enum(context_t ctx, node_t node) {
  cubec_declaration_enum_t en =
      (cubec_declaration_enum_t)node;
  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, NULL, TYPE_ENUM);
  vec_push(ctx->all_types, t);

  vec_init_t vi = {.auto_dispose = true};
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
      const char *iname = _resolver_ident_str(item->name);
      struct symbol *isym = symbol_create(ctx->allocator, iname,
                                           SYMBOL_ENUM_ITEM,
                                           item->super.location);
      isym->enum_item.owning_type = t;
      /* Evaluate explicit value if present, otherwise auto-increment */
      if (item->value) {
        if (item->value->kind == CUBEC_NODE_LITERAL_NUMERIC) {
          cubec_literal_numeric_t num = (cubec_literal_numeric_t)item->value;
          const char *numstr = string_get(num->value);
          isym->enum_item.value = numstr ? atoll(numstr) : auto_val;
        } else {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                               item->value->location,
                               "enum value must be a compile-time integer literal");
          ctx->error_count++;
          isym->enum_item.value = auto_val;
        }
        auto_val = isym->enum_item.value + 1;
      } else {
        isym->enum_item.value = auto_val++;
      }
      vec_push(t->impl->enum_type.items, isym);
    }
  }

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  return t;
}

semantic_type_t _resolve_type_union(context_t ctx, node_t node) {
  cubec_declaration_union_t un =
      (cubec_declaration_union_t)node;
  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, NULL, TYPE_UNION);
  vec_push(ctx->all_types, t);

  vec_init_t vi = {.auto_dispose = true};
  t->impl->struct_type.fields =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  if (un->members) {
    size_t count = vec_get_size(un->members);
    for (size_t i = 0; i < count; i++) {
      node_t member = (node_t)vec_get(un->members, i);
      if (!member) continue;

      if (member->kind == CUBEC_NODE_STRUCT_FIELD) {
        cubec_struct_field_t field = (cubec_struct_field_t)member;
        const char *fname = _resolver_ident_str(field->name);
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

semantic_type_t _resolve_type_interface(context_t ctx, node_t node) {
  cubec_declaration_interface_t iface =
      (cubec_declaration_interface_t)node;
  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, NULL, TYPE_INTERFACE);
  t->is_interface = true;
  vec_push(ctx->all_types, t);

  vec_init_t vi = {.auto_dispose = true};
  t->impl->interface_type.methods =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  if (iface->members) {
    size_t count = vec_get_size(iface->members);
    for (size_t i = 0; i < count; i++) {
      node_t member = (node_t)vec_get(iface->members, i);
      if (!member) continue;

      if (member->kind == CUBEC_NODE_INTERFACE_METHOD) {
        cubec_interface_method_t method = (cubec_interface_method_t)member;
        const char *mname = _resolver_ident_str(method->name);
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

semantic_type_t _resolve_type_function(context_t ctx, node_t node) {
  cubec_declaration_callable_t ft =
      (cubec_declaration_callable_t)node;

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
  vec_push(ctx->all_types, result);
  return result;
}

semantic_type_t _resolve_type_namespace_access(context_t ctx, node_t node) {
  cubec_expression_namespace_access_t ns =
      (cubec_expression_namespace_access_t)node;
  const char *field_name = _resolver_ident_str((node_t)ns->field);
  if (!field_name) return ctx->error_type;

  semantic_type_t host_type = resolver_resolve_type(ctx, ns->host);
  if (host_type->impl->kind == TYPE_ERROR) return ctx->error_type;

  /* Module scope access: module_name::TypeName */
  if (host_type->impl->kind == TYPE_MODULE) {
    const char *mod_name = host_type->name;
    struct symbol *mod_sym = scope_lookup(ctx->current_scope, mod_name);
    if (!mod_sym || mod_sym->kind != SYMBOL_MODULE) return ctx->error_type;
    scope_t mod_scope = mod_sym->module.scope;
    if (!mod_scope) return ctx->error_type;
    struct symbol *member = scope_lookup_local(mod_scope, field_name);
    if (!member || !member->is_export || member->kind != SYMBOL_TYPE || !member->type.type) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "module '%s' has no exported type '%s'",
                           mod_name, field_name);
      ctx->error_count++;
      return ctx->error_type;
    }
    return member->type.type;
  }

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

semantic_type_t _resolve_type_typeof(context_t ctx, node_t node) {
  cubec_expression_typeof_t to = (cubec_expression_typeof_t)node;
  semantic_type_t inner = context_check_expression(ctx, to->expression);
  if (!inner || inner->impl->kind == TYPE_ERROR) return ctx->error_type;
  semantic_type_t t =
      semantic_type_create_named(ctx->allocator, NULL, TYPE_TYPE);
  t->impl->type_of.inner = inner;
  t->is_incomplete = false;
  vec_push(ctx->all_types, t);
  return t;
}
