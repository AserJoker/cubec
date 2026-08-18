#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/void_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "engine/struct_type.h"
#include "engine/tuple_type.h"
#include "engine/array_type.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/initialize_field.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"
#include "core/vec.h"

/* ---- forward helpers ---- */

static value_t _eval_field_value(vm_t vm, node_t item, bool shadow) {
  cubec_initialize_field_t f = (cubec_initialize_field_t)item;
  return run_expression(vm, f->value, shadow);
}

static const char *_field_name(node_t item) {
  cubec_initialize_field_t f = (cubec_initialize_field_t)item;
  return string_get(f->field->value);
}

static value_t _eval_positional(vm_t vm, node_t item, bool shadow) {
  return run_expression(vm, item, shadow);
}

/* ---- typed struct: reorder by declaration order, safe_cast each field ---- */

static value_t _build_typed_struct(vm_t vm, value_t type_val,
                                   cubec_expression_initialize_list_t node,
                                   bool shadow) {
  vec_t fields = vm_struct_get_fields(vm, type_val);
  size_t field_count = vec_get_size(fields);
  size_t item_count = vec_get_size(node->items);

  if (item_count != field_count)
    return create_exception_value(vm,
                                  "initialize_list: struct '%s' expects %zu fields, got %zu",
                                  type_get_name((type_t)value_get_data(type_val)),
                                  field_count, item_count);

  /* shadow path: return struct shadow, no value evaluation needed */
  if (shadow)
    return vm_create_struct_shadow(vm, type_val, true);

  /* evaluate field items, then reorder into declaration order */
  allocator_t alloc = vm_get_allocator(vm);
  value_t *ordered = (value_t *)allocator_alloc(alloc, field_count * sizeof(value_t));
  if (!ordered)
    return create_exception_value(vm, "initialize_list: out of memory");
  for (size_t i = 0; i < field_count; i++)
    ordered[i] = NULL;

  /* match each initialize_field item to a declared field by name, safe_cast */
  for (size_t i = 0; i < item_count; i++) {
    node_t item = (node_t)vec_get(node->items, i);
    const char *name = _field_name(item);
    field_info_t fi = vm_struct_find_field(vm, type_val, name);
    if (!fi) {
      for (size_t j = 0; j < field_count; j++)
        if (ordered[j]) ordered[j] = NULL;
      allocator_free(alloc, &ordered);
      return create_exception_value(vm,
                                    "initialize_list: struct '%s' has no field '%s'",
                                    type_get_name((type_t)value_get_data(type_val)),
                                    name);
    }
    /* find the field's declaration index */
    size_t decl_idx = 0;
    for (; decl_idx < field_count; decl_idx++) {
      field_info_t dj = (field_info_t)vec_get(fields, decl_idx);
      if (dj == fi) break;
    }
    value_t v = _eval_field_value(vm, item, false);
    if (value_is_error(v)) {
      allocator_free(alloc, &ordered);
      return v;
    }
    value_t cast = value_safe_cast(vm, v, field_info_get_type(fi));
    if (value_is_error(cast)) {
      allocator_free(alloc, &ordered);
      return cast;
    }
    ordered[decl_idx] = cast;
  }

  value_t result = vm_create_struct_value(vm, type_val, ordered);
  allocator_free(alloc, &ordered);
  return result;
}

/* ---- typed tuple: positional, safe_cast each element ---- */

static value_t _build_typed_tuple(vm_t vm, tuple_type_t tt,
                                  cubec_expression_initialize_list_t node,
                                  bool shadow) {
  size_t count = tuple_type_get_field_count(tt);
  size_t item_count = vec_get_size(node->items);
  if (item_count != count)
    return create_exception_value(vm,
                                  "initialize_list: tuple expects %zu elements, got %zu",
                                  count, item_count);

  if (shadow)
    return create_tuple_shadow(vm, tt, true);

  allocator_t alloc = vm_get_allocator(vm);
  value_t *elems = (value_t *)allocator_alloc(alloc, count * sizeof(value_t));
  if (!elems)
    return create_exception_value(vm, "initialize_list: out of memory");

  for (size_t i = 0; i < count; i++) {
    node_t item = (node_t)vec_get(node->items, i);
    value_t v = _eval_positional(vm, item, false);
    if (value_is_error(v)) {
      allocator_free(alloc, &elems);
      return v;
    }
    value_t cast = value_safe_cast(vm, v, tuple_type_get_element_type(tt, (uint64_t)i));
    if (value_is_error(cast)) {
      allocator_free(alloc, &elems);
      return cast;
    }
    elems[i] = cast;
  }

  value_t result = create_tuple_value(vm, tt, elems);
  allocator_free(alloc, &elems);
  return result;
}

/* ---- typed array: positional, safe_cast each element ---- */

static value_t _build_typed_array(vm_t vm, array_type_t at,
                                  cubec_expression_initialize_list_t node,
                                  bool shadow) {
  uint64_t count = array_type_get_count(at);
  size_t item_count = vec_get_size(node->items);
  if (item_count != count)
    return create_exception_value(vm,
                                  "initialize_list: array expects %zu elements, got %zu",
                                  (size_t)count, item_count);

  if (shadow)
    return create_array_shadow(vm, at, true);

  allocator_t alloc = vm_get_allocator(vm);
  value_t *elems = (value_t *)allocator_alloc(alloc, count * sizeof(value_t));
  if (!elems)
    return create_exception_value(vm, "initialize_list: out of memory");

  type_t elem_type = array_type_get_element_type(at);
  for (size_t i = 0; i < count; i++) {
    node_t item = (node_t)vec_get(node->items, i);
    value_t v = _eval_positional(vm, item, false);
    if (value_is_error(v)) {
      allocator_free(alloc, &elems);
      return v;
    }
    value_t cast = value_safe_cast(vm, v, elem_type);
    if (value_is_error(cast)) {
      allocator_free(alloc, &elems);
      return cast;
    }
    elems[i] = cast;
  }

  value_t result = create_array_value(vm, at, elems);
  allocator_free(alloc, &elems);
  return result;
}

/* ---- anonymous struct (named fields, NULL name) ---- */

static value_t _build_anon_struct(vm_t vm,
                                  cubec_expression_initialize_list_t node,
                                  bool shadow) {
  const char *module_id = vm_get_current_module_id(vm);
  size_t item_count = vec_get_size(node->items);

  /* build the anonymous struct type (name=NULL) from field values */
  value_t type_val = vm_create_struct_type_value(vm, NULL, true, module_id);
  if (value_is_error(type_val))
    return type_val;

  /* evaluate field values to derive their types; keep values for later use.
   * shadow mode: no value evaluation, build field types from shadow values. */
  allocator_t alloc = vm_get_allocator(vm);
  value_t *field_vals = NULL;
  if (item_count > 0) {
    field_vals = (value_t *)allocator_alloc(alloc, item_count * sizeof(value_t));
    if (!field_vals)
      return create_exception_value(vm, "initialize_list: out of memory");
  }

  for (size_t i = 0; i < item_count; i++) {
    node_t item = (node_t)vec_get(node->items, i);
    const char *name = _field_name(item);
    value_t v = _eval_field_value(vm, item, shadow);
    if (value_is_error(v)) {
      allocator_free(alloc, &field_vals);
      return v;
    }
    field_vals[i] = v;
    type_t ft = value_get_type(v);
    value_t ft_val = create_type_value(vm, ft, NULL, false);
    if (value_is_error(ft_val)) {
      allocator_free(alloc, &field_vals);
      return ft_val;
    }
    value_t r = vm_struct_add_field(vm, type_val, name, ft_val, true);
    if (value_is_error(r)) {
      allocator_free(alloc, &field_vals);
      return r;
    }
  }

  value_t seal_res = vm_struct_seal(vm, type_val);
  if (value_is_error(seal_res)) {
    allocator_free(alloc, &field_vals);
    return seal_res;
  }

  if (shadow) {
    allocator_free(alloc, &field_vals);
    return vm_create_struct_shadow(vm, type_val, true);
  }

  /* vm_create_struct_value expects field_values in declaration order, which
   * matches our insertion order since we added fields in item order. */
  value_t result = vm_create_struct_value(vm, type_val, field_vals);
  allocator_free(alloc, &field_vals);
  return result;
}

/* ---- anonymous tuple (positional) ---- */

static value_t _build_anon_tuple(vm_t vm,
                                 cubec_expression_initialize_list_t node,
                                 bool shadow) {
  size_t count = vec_get_size(node->items);
  /* empty list handled by caller as empty struct */

  allocator_t alloc = vm_get_allocator(vm);
  vec_t elem_types = (vec_t)allocator_create(alloc, &g_vec_class,
                                             &(vec_init_t){.auto_dispose = false});
  value_t *elems = (value_t *)allocator_alloc(alloc, count * sizeof(value_t));
  if (!elems) {
    allocator_free(alloc, &elem_types);
    return create_exception_value(vm, "initialize_list: out of memory");
  }

  for (size_t i = 0; i < count; i++) {
    node_t item = (node_t)vec_get(node->items, i);
    value_t v = _eval_positional(vm, item, shadow);
    if (value_is_error(v)) {
      allocator_free(alloc, &elems);
      allocator_free(alloc, &elem_types);
      return v;
    }
    elems[i] = v;
    vec_push(elem_types, value_get_type(v));
  }

  value_t tv = vm_create_tuple_type_value(vm, elem_types, true);
  allocator_free(alloc, &elem_types);
  if (value_is_error(tv)) {
    allocator_free(alloc, &elems);
    return tv;
  }
  tuple_type_t tt = (tuple_type_t)value_get_data(tv);

  if (shadow) {
    allocator_free(alloc, &elems);
    return create_tuple_shadow(vm, tt, true);
  }

  value_t result = create_tuple_value(vm, tt, elems);
  allocator_free(alloc, &elems);
  return result;
}

/* ---- main entry ---- */

value_t run_expression_initialize_list(vm_t vm, node_t node, bool shadow) {
  cubec_expression_initialize_list_t init_list =
      (cubec_expression_initialize_list_t)node;
  size_t item_count = vec_get_size(init_list->items);

  /* typed path: type != NULL.
   * run_expression on a type name yields a TYPE_KIND_TYPE value whose
   * value.data is the concrete type_t. Type resolution is always concrete
   * (shadow=false) — the shadow flag only governs the resulting value. */
  if (init_list->type) {
    value_t type_val = run_expression(vm, init_list->type, false);
    if (value_is_error(type_val))
      return type_val;

    type_t concrete = (type_t)value_get_data(type_val);
    if (!concrete)
      return create_exception_value(vm,
                                    "initialize_list: type expression did not resolve to a type");
    type_kind_t kind = type_get_kind(concrete);

    switch (kind) {
    case TYPE_KIND_STRUCT:
      if (!init_list->is_field)
        return create_exception_value(vm,
                                      "initialize_list: struct requires named fields, got positional");
      return _build_typed_struct(vm, type_val, init_list, shadow);
    case TYPE_KIND_TUPLE:
      if (init_list->is_field)
        return create_exception_value(vm,
                                      "initialize_list: tuple requires positional elements, got named fields");
      return _build_typed_tuple(vm, (tuple_type_t)concrete, init_list, shadow);
    case TYPE_KIND_ARRAY:
      if (init_list->is_field)
        return create_exception_value(vm,
                                      "initialize_list: array requires positional elements, got named fields");
      return _build_typed_array(vm, (array_type_t)concrete, init_list, shadow);
    default:
      return create_exception_value(vm,
                                    "initialize_list: type '%s' (kind %d) does not support initialize_list",
                                    type_get_name(concrete), kind);
    }
  }

  /* anonymous path: type == NULL */
  /* empty list .{} → empty anonymous struct (per design decision) */
  if (item_count == 0 || init_list->is_field)
    return _build_anon_struct(vm, init_list, shadow);
  return _build_anon_tuple(vm, init_list, shadow);
}
