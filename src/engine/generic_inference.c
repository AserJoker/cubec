#include "engine/generic_inference.h"
#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/exception_type.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/callable_type.h"
#include "engine/array_type.h"
#include "engine/slice_type.h"
#include "engine/pointer_type.h"
#include "engine/tuple_type.h"
#include "engine/pack_type.h"
#include "engine/struct_type.h"
#include "engine/union_type.h"
#include "engine/cunion_type.h"
#include "engine/enum_type.h"
#include "engine/integer_type.h"
#include "run/run.h"
#include "cubec/declaration_function.h"
#include "cubec/function_argument.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"
#include "core/vec.h"
#include "core/class.h"
#include <string.h>

/* ===================================================================
 *  Placeholder type creation
 * =================================================================== */

/* ---- vtable for TYPE_KIND_GENERIC_PARAM ---- */

static value_t _generic_param_type_equal(vm_t vm, type_t a, type_t b) {
  /* Two generic param placeholders are equal iff they have the same name */
  if (b->kind == TYPE_KIND_GENERIC_PARAM)
    return create_bool_value(vm, strcmp(a->name, b->name) == 0);
  return create_bool_value(vm, false);
}

static value_t _generic_param_type_extends(vm_t vm, type_t sub, type_t super) {
  (void)sub;
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  /* placeholder extends nothing concrete */
  return create_bool_value(vm, super->kind == TYPE_KIND_GENERIC_PARAM &&
                                strcmp(sub->name, super->name) == 0);
}

static type_t _generic_param_type_clone(vm_t vm, type_t self) {
  return generic_param_placeholder_create(vm_get_allocator(vm), self->name);
}

/* Placeholder infer_walk: when formal is a GENERIC_PARAM, assign the actual
 * type to the named parameter. */
static bool _generic_param_infer_walk(vm_t vm, type_t formal, type_t actual,
                                      void *ctx) {
  const char *pname = type_get_name(formal);
  return infer_walk_assign_param(vm, (infer_ctx_t)ctx, pname, actual);
}

/* ---- vtable for TYPE_KIND_GENERIC_PACK ---- */

static value_t _generic_pack_type_equal(vm_t vm, type_t a, type_t b) {
  if (b->kind == TYPE_KIND_GENERIC_PACK)
    return create_bool_value(vm, strcmp(a->name, b->name) == 0);
  return create_bool_value(vm, false);
}

static value_t _generic_pack_type_extends(vm_t vm, type_t sub, type_t super) {
  (void)sub;
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, super->kind == TYPE_KIND_GENERIC_PACK &&
                                strcmp(sub->name, super->name) == 0);
}

static type_t _generic_pack_type_clone(vm_t vm, type_t self) {
  return generic_pack_placeholder_create(vm_get_allocator(vm), self->name);
}

/* Pack placeholder cannot be walked recursively — handled at param level */
static bool _generic_pack_infer_walk(vm_t vm, type_t formal, type_t actual,
                                     void *ctx) {
  (void)vm; (void)formal; (void)actual; (void)ctx;
  return false;
}

/* ---- Creation functions ---- */

type_t generic_param_placeholder_create(allocator_t allocator, const char *name) {
  type_init_t init = {
      .kind   = TYPE_KIND_GENERIC_PARAM,
      .name   = name,
      .size   = 0,
      .align  = 0,
      .mut    = false,
      .vtable = {
          .type_equal   = _generic_param_type_equal,
          .type_extends = _generic_param_type_extends,
          .type_clone   = _generic_param_type_clone,
          .infer_walk   = _generic_param_infer_walk,
      },
  };
  return (type_t)allocator_create(allocator, &g_type_class, &init);
}

type_t generic_pack_placeholder_create(allocator_t allocator, const char *name) {
  type_init_t init = {
      .kind   = TYPE_KIND_GENERIC_PACK,
      .name   = name,
      .size   = 0,
      .align  = 0,
      .mut    = false,
      .vtable = {
          .type_equal   = _generic_pack_type_equal,
          .type_extends = _generic_pack_type_extends,
          .type_clone   = _generic_pack_type_clone,
          .infer_walk   = _generic_pack_infer_walk,
      },
  };
  return (type_t)allocator_create(allocator, &g_type_class, &init);
}

bool type_is_generic_param_placeholder(type_t t) {
  return t && type_get_kind(t) == TYPE_KIND_GENERIC_PARAM;
}

bool type_is_generic_pack_placeholder(type_t t) {
  return t && type_get_kind(t) == TYPE_KIND_GENERIC_PACK;
}

const char *generic_placeholder_get_name(type_t t) {
  return t ? type_get_name(t) : NULL;
}

/* ===================================================================
 *  Inference state
 * =================================================================== */

/* Opaque context wrapper — exposed as void* to vtable consumers */
typedef struct _infer_ctx_impl_t {
  size_t          entry_count;
  infer_entry_t  *entries;
} infer_ctx_impl_t;

/* ===================================================================
 *  Helper functions for vtable.infer_walk
 * =================================================================== */

bool infer_walk_recurse(vm_t vm, infer_ctx_t ctx, type_t formal, type_t actual) {
  (void)ctx;
  /* If the formal type has a custom infer_walk, use it */
  vtable_t vt = type_get_vtable(formal);
  if (vt.infer_walk)
    return vt.infer_walk(vm, formal, actual, ctx);

  /* Default: kinds must match exactly (primitive types) */
  type_kind_t fk = type_get_kind(formal);
  type_kind_t ak = type_get_kind(actual);
  return fk == ak;
}

bool infer_walk_assign_param(vm_t vm, infer_ctx_t ctx,
                             const char *name, type_t actual) {
  infer_ctx_impl_t *impl = (infer_ctx_impl_t *)ctx;
  infer_entry_t *entries = impl->entries;
  size_t entry_count = impl->entry_count;

  for (size_t i = 0; i < entry_count; i++) {
    if (strcmp(entries[i].name, name) == 0) {
      if (entries[i].is_pack) {
        /* Pack param: collect each actual type into inferred_pack_values */
        type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
        value_t type_val = vm_create_value_ref(vm, type_type, actual, NULL);
        vec_push(entries[i].inferred_pack_values, type_val);
        return true;
      }
      if (!entries[i].inferred_value) {
        type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
        value_t type_val = vm_create_value_ref(vm, type_type, actual, NULL);
        entries[i].inferred_value = type_val;
        return true;
      }
      value_t existing = entries[i].inferred_value;
      if (type_get_kind(value_get_type(existing)) != TYPE_KIND_TYPE)
        return false;
      type_t existing_type = (type_t)value_get_data(existing);
      return existing_type == actual;
    }
  }
  return false;
}

bool infer_walk_assign_value(vm_t vm, infer_ctx_t ctx,
                             type_t value_type, void *data) {
  infer_ctx_impl_t *impl = (infer_ctx_impl_t *)ctx;
  infer_entry_t *entries = impl->entries;
  size_t entry_count = impl->entry_count;

  int match = -1;
  for (size_t i = 0; i < entry_count; i++) {
    if (entries[i].is_value_param && !entries[i].inferred_value &&
        entries[i].param_type == value_type) {
      if (match >= 0) return false; /* ambiguous */
      match = (int)i;
    }
  }
  if (match < 0) return false;

  /* Create a value of the declared type with the actual data.
   * Unlike type params (which produce TYPE_KIND_TYPE values wrapping a type),
   * value params produce values of the declared type (e.g. u64) with concrete data.
   * vm_create_value copies the data, so we pass data directly. */
  entries[match].inferred_value =
      vm_create_value(vm, value_type, data, NULL);
  return true;
}

static infer_ctx_t _make_infer_ctx(allocator_t allocator,
                                   infer_entry_t *entries,
                                   size_t entry_count) {
  infer_ctx_impl_t *impl = (infer_ctx_impl_t *)allocator_alloc(
      allocator, sizeof(infer_ctx_impl_t));
  impl->entry_count = entry_count;
  impl->entries = entries;
  return (infer_ctx_t)impl;
}

infer_ctx_t infer_ctx_create(allocator_t allocator,
                             void *entries, size_t count) {
  return _make_infer_ctx(allocator, (infer_entry_t *)entries, count);
}

value_t infer_ctx_get_inferred(infer_ctx_t ctx, size_t index) {
  infer_ctx_impl_t *impl = (infer_ctx_impl_t *)ctx;
  if (index >= impl->entry_count) return NULL;
  return impl->entries[index].inferred_value;
}

/* ===================================================================
 *  Per-type infer_walk implementations
 *
 *  These are non-static so that type .c files can reference them
 *  from their _make_*_vtable() factory functions.
 * =================================================================== */

/* ---- pointer ---- */

bool _pointer_infer_walk(vm_t vm, type_t formal, type_t actual,
                         void *ctx) {
  if (type_get_kind(actual) != TYPE_KIND_POINTER) return false;
  type_t form_pointee = pointer_type_get_pointee_type((pointer_type_t)formal);
  type_t act_pointee = pointer_type_get_pointee_type((pointer_type_t)actual);
  return infer_walk_recurse(vm, (infer_ctx_t)ctx, form_pointee, act_pointee);
}

/* ---- array ---- */

bool _array_infer_walk(vm_t vm, type_t formal, type_t actual,
                       void *ctx) {
  if (type_get_kind(actual) != TYPE_KIND_ARRAY) return false;
  type_t form_elem = array_type_get_element_type((array_type_t)formal);
  type_t act_elem = array_type_get_element_type((array_type_t)actual);
  if (!infer_walk_recurse(vm, (infer_ctx_t)ctx, form_elem, act_elem))
    return false;

  /* Array count matching */
  value_t form_count = array_type_get_count((array_type_t)formal);
  value_t act_count = array_type_get_count((array_type_t)actual);
  if (array_type_is_count_wildcard((array_type_t)formal))
    return true;

  /* If formal count is a sentinel value (UINT64_MAX = pending inference),
   * infer the value param from the actual count.
   * Use safe_cast to handle type-compatible integer types (e.g. formal=u64, actual=i32). */
  {
    type_t fc_type = value_get_type(form_count);
    type_kind_t fc_kind = type_get_kind(fc_type);
    bool is_sentinel = false;
    if (fc_kind >= TYPE_KIND_I8 && fc_kind <= TYPE_KIND_U64) {
      uint64_t fv = 0;
      size_t sz = (size_t)type_get_size(fc_type);
      memcpy(&fv, value_get_data(form_count), sz);
      /* Mask to the actual type width to detect sentinel */
      if (sz < sizeof(uint64_t))
        fv &= ((uint64_t)1 << (sz * 8)) - 1;
      if (fv == UINT64_MAX || (sz < sizeof(uint64_t) && fv == ((uint64_t)1 << (sz * 8)) - 1))
        is_sentinel = true;
    }
    if (is_sentinel) {
      value_t cast_count = value_safe_cast(vm, act_count, fc_type);
      if (value_is_abnormal(cast_count))
        return false;
      return infer_walk_assign_value(vm, (infer_ctx_t)ctx, fc_type,
                                     value_get_data(cast_count));
    }
  }

  /* Both concrete: must match exactly */
  value_t eq = value_equal(vm, form_count, act_count);
  if (value_is_abnormal(eq)) return false;
  if (value_is_shadow(eq)) return true;
  return *(bool *)value_get_data(eq);
}

/* ---- slice ---- */

bool _slice_infer_walk(vm_t vm, type_t formal, type_t actual,
                       void *ctx) {
  if (type_get_kind(actual) != TYPE_KIND_SLICE) return false;
  type_t form_elem = slice_type_get_element_type((slice_type_t)formal);
  type_t act_elem = slice_type_get_element_type((slice_type_t)actual);
  return infer_walk_recurse(vm, (infer_ctx_t)ctx, form_elem, act_elem);
}

/* ---- tuple ---- */

bool _tuple_infer_walk(vm_t vm, type_t formal, type_t actual,
                       void *ctx) {
  if (type_get_kind(actual) != TYPE_KIND_TUPLE) return false;
  uint64_t fc = tuple_type_get_field_count((tuple_type_t)formal);
  uint64_t ac = tuple_type_get_field_count((tuple_type_t)actual);
  if (fc != ac) return false;
  for (uint64_t i = 0; i < fc; i++) {
    type_t ft = tuple_type_get_element_type((tuple_type_t)formal, i);
    type_t at = tuple_type_get_element_type((tuple_type_t)actual, i);
    if (!infer_walk_recurse(vm, (infer_ctx_t)ctx, ft, at))
      return false;
  }
  return true;
}

/* ---- callable ---- */

bool _callable_infer_walk(vm_t vm, type_t formal, type_t actual,
                          void *ctx) {
  if (type_get_kind(actual) != TYPE_KIND_CALLABLE) return false;
  callable_type_t fct = (callable_type_t)formal;
  callable_type_t act = (callable_type_t)actual;
  uint64_t fpc = callable_type_get_param_count(fct);
  uint64_t apc = callable_type_get_param_count(act);

  /* Check if formal has a GENERIC_PACK param — it absorbs remaining actual params */
  int pack_idx = -1;
  for (uint64_t i = 0; i < fpc; i++) {
    if (type_get_kind(callable_type_get_param_type(fct, i)) == TYPE_KIND_GENERIC_PACK) {
      pack_idx = (int)i;
      break;
    }
  }

  if (pack_idx >= 0) {
    /* Pack param: match fixed params before pack, collect remaining for pack,
     * match fixed params after pack from the end. */
    uint64_t fixed_before = (uint64_t)pack_idx;
    uint64_t fixed_after = fpc - fixed_before - 1;
    if (apc < fixed_before + fixed_after)
      return false;

    /* Match params before pack */
    for (uint64_t i = 0; i < fixed_before; i++) {
      type_t ft = callable_type_get_param_type(fct, i);
      type_t at = callable_type_get_param_type(act, i);
      if (!infer_walk_recurse(vm, (infer_ctx_t)ctx, ft, at))
        return false;
    }

    /* Collect pack types — assign each actual type in the pack range */
    type_t pack_type = callable_type_get_param_type(fct, (uint64_t)pack_idx);
    const char *pack_name = generic_placeholder_get_name(pack_type);
    uint64_t pack_end = apc - fixed_after;
    for (uint64_t i = fixed_before; i < pack_end; i++) {
      type_t at = callable_type_get_param_type(act, i);
      if (!infer_walk_assign_param(vm, (infer_ctx_t)ctx, pack_name, at))
        return false;
    }

    /* Match params after pack */
    for (uint64_t i = 0; i < fixed_after; i++) {
      type_t ft = callable_type_get_param_type(fct, fpc - fixed_after + i);
      type_t at = callable_type_get_param_type(act, pack_end + i);
      if (!infer_walk_recurse(vm, (infer_ctx_t)ctx, ft, at))
        return false;
    }
  } else {
    /* No pack: exact param count match required */
    if (fpc != apc) return false;
    for (uint64_t i = 0; i < fpc; i++) {
      type_t ft = callable_type_get_param_type(fct, i);
      type_t at = callable_type_get_param_type(act, i);
      if (!infer_walk_recurse(vm, (infer_ctx_t)ctx, ft, at))
        return false;
    }
  }

  type_t fret = callable_type_get_return_type(fct);
  type_t aret = callable_type_get_return_type(act);
  return infer_walk_recurse(vm, (infer_ctx_t)ctx, fret, aret);
}

/* ---- struct ---- */

bool _struct_infer_walk(vm_t vm, type_t formal, type_t actual,
                        void *ctx) {
  if (type_get_kind(actual) != TYPE_KIND_STRUCT) return false;
  struct_type_t fst = (struct_type_t)formal;
  struct_type_t ast_st = (struct_type_t)actual;
  uint64_t fc = vec_get_size(fst->fields);
  uint64_t ac = vec_get_size(ast_st->fields);
  if (fc != ac) return false;
  for (uint64_t i = 0; i < fc; i++) {
    field_info_t ffi = (field_info_t)vec_get(fst->fields, i);
    field_info_t afi = (field_info_t)vec_get(ast_st->fields, i);
    if (strcmp(field_info_get_name(ffi), field_info_get_name(afi)) != 0)
      return false;
    if (!infer_walk_recurse(vm, (infer_ctx_t)ctx,
                            field_info_get_type(ffi), field_info_get_type(afi)))
      return false;
  }
  return true;
}

/* ---- union ---- */

bool _union_infer_walk(vm_t vm, type_t formal, type_t actual,
                       void *ctx) {
  if (type_get_kind(actual) != TYPE_KIND_UNION) return false;
  union_type_t fut = (union_type_t)formal;
  union_type_t aut = (union_type_t)actual;
  uint64_t fc = vec_get_size(fut->fields);
  uint64_t ac = vec_get_size(aut->fields);
  if (fc != ac) return false;
  for (uint64_t i = 0; i < fc; i++) {
    field_info_t ffi = (field_info_t)vec_get(fut->fields, i);
    field_info_t afi = (field_info_t)vec_get(aut->fields, i);
    if (strcmp(field_info_get_name(ffi), field_info_get_name(afi)) != 0)
      return false;
    if (!infer_walk_recurse(vm, (infer_ctx_t)ctx,
                            field_info_get_type(ffi), field_info_get_type(afi)))
      return false;
  }
  return true;
}

/* ---- cunion ---- */

bool _cunion_infer_walk(vm_t vm, type_t formal, type_t actual,
                        void *ctx) {
  if (type_get_kind(actual) != TYPE_KIND_CUNION) return false;
  cunion_type_t fct = (cunion_type_t)formal;
  cunion_type_t act = (cunion_type_t)actual;
  uint64_t fc = vec_get_size(fct->fields);
  uint64_t ac = vec_get_size(act->fields);
  if (fc != ac) return false;
  for (uint64_t i = 0; i < fc; i++) {
    field_info_t ffi = (field_info_t)vec_get(fct->fields, i);
    field_info_t afi = (field_info_t)vec_get(act->fields, i);
    if (strcmp(field_info_get_name(ffi), field_info_get_name(afi)) != 0)
      return false;
    if (!infer_walk_recurse(vm, (infer_ctx_t)ctx,
                            field_info_get_type(ffi), field_info_get_type(afi)))
      return false;
  }
  return true;
}

/* ---- enum ---- */

bool _enum_infer_walk(vm_t vm, type_t formal, type_t actual,
                      void *ctx) {
  if (type_get_kind(actual) != TYPE_KIND_ENUM) return false;
  enum_type_t fet = (enum_type_t)formal;
  enum_type_t aet = (enum_type_t)actual;
  /* Recursively match the underlying type */
  type_t f_under = enum_type_get_underlying(fet);
  type_t a_under = enum_type_get_underlying(aet);
  if (!infer_walk_recurse(vm, (infer_ctx_t)ctx, f_under, a_under))
    return false;
  /* Items must have the same count and names */
  strmap_t f_items = fet->items;
  strmap_t a_items = aet->items;
  if (strmap_get_size(f_items) != strmap_get_size(a_items))
    return false;
  strmap_iter_t it = strmap_iter_first(f_items);
  const char *key;
  while ((key = strmap_iter_next(&it)) != NULL) {
    if (!strmap_find(a_items, key))
      return false;
  }
  return true;
}

/* ===================================================================
 *  generic_fn_call_with_inference — main entry point
 * =================================================================== */

value_t generic_fn_call_with_inference(vm_t vm, value_t generic_val,
                                       size_t type_argc, value_t *type_argv,
                                       size_t call_argc, value_t *call_argv) {
  type_t self_type = value_get_type(generic_val);
  generic_fn_type_t gt_fn = (generic_fn_type_t)self_type;
  allocator_t allocator = vm_get_allocator(vm);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));

  vec_t param_defs = generic_fn_type_get_params(gt_fn);
  size_t param_count = vec_get_size(param_defs);

  /* ---- 1. Initialize inference entries ---- */
  infer_entry_t *entries = (infer_entry_t *)allocator_alloc(
      allocator, param_count * sizeof(infer_entry_t));

  for (size_t i = 0; i < param_count; i++) {
    generic_param_t gp = (generic_param_t)vec_get(param_defs, i);
    const char *pname = generic_param_get_name(gp);
    type_t param_type = generic_param_get_type(gp);
    bool is_pack = generic_param_is_rest(gp);
    bool is_value_param = type_get_kind(param_type) != TYPE_KIND_TYPE &&
                          type_get_kind(param_type) != TYPE_KIND_PACK;

    entries[i].name = pname;
    entries[i].inferred_value = NULL;
    entries[i].is_pack = is_pack;
    entries[i].is_value_param = is_value_param;
    entries[i].param_type = is_value_param ? param_type : NULL;

    if (is_pack) {
      entries[i].placeholder = generic_pack_placeholder_create(allocator, pname);
      vec_init_t pvi = {.auto_dispose = false};
      entries[i].inferred_pack_values =
          (vec_t)allocator_create(allocator, &g_vec_class, &pvi);
    } else if (is_value_param) {
      entries[i].placeholder = NULL;
      entries[i].inferred_pack_values = NULL;
    } else {
      entries[i].placeholder = generic_param_placeholder_create(allocator, pname);
      entries[i].inferred_pack_values = NULL;
    }

    /* Register placeholder in vm->types so it gets cleaned up */
    if (entries[i].placeholder)
      vec_push(vm_get_types(vm), entries[i].placeholder);
  }

  /* ---- 2. Assign explicit type arguments ---- */
  size_t argv_idx = 0;
  for (size_t i = 0; i < param_count && argv_idx < type_argc; i++) {
    if (entries[i].is_pack) {
      while (argv_idx < type_argc) {
        vec_push(entries[i].inferred_pack_values, type_argv[argv_idx]);
        argv_idx++;
      }
    } else {
      entries[i].inferred_value = type_argv[argv_idx];
      argv_idx++;
    }
  }

  /* ---- 3. Create temp scope with placeholder bindings ---- */
  scope_t scope_before = vm_get_current_scope(vm);
  scope_t temp_scope = scope_create(allocator, SCOPE_TYPE, scope_before, NULL);
  vm_push_scope(vm, temp_scope);

  for (size_t i = 0; i < param_count; i++) {
    if (entries[i].inferred_value) {
      type_t concrete_type = value_get_type(entries[i].inferred_value);
      void *concrete_data = value_get_data(entries[i].inferred_value);
      vm_create_value_ref(vm, concrete_type, concrete_data, entries[i].name);
    } else if (entries[i].is_value_param) {
      /* Value param: bind as a value of declared type, initialized to UINT64_MAX
       * to indicate "pending inference". This avoids NULL data pointers that
       * would crash memcpy in array_type_create and other consumers.
       * vm_create_value copies the data, so we use a stack buffer. */
      type_t pt = entries[i].param_type;
      uint64_t sentinel = UINT64_MAX;
      vm_create_value(vm, pt, &sentinel, entries[i].name);
    } else if (entries[i].is_pack) {
      vm_create_value_ref(vm, type_type, entries[i].placeholder, entries[i].name);
    } else {
      vm_create_value_ref(vm, type_type, entries[i].placeholder, entries[i].name);
    }
  }

  /* ---- 4. Evaluate formal parameter types with placeholders in scope ---- */
  cubec_declaration_function_t decl =
      (cubec_declaration_function_t)generic_fn_type_get_node(gt_fn);
  vec_t ast_args = decl->arguments;
  size_t formal_argc = ast_args ? vec_get_size(ast_args) : 0;

  type_t *formal_types = NULL;
  if (formal_argc > 0)
    formal_types = (type_t *)allocator_alloc(
        allocator, formal_argc * sizeof(type_t));
  for (size_t i = 0; i < formal_argc; i++) {
    cubec_function_argument_t farg =
        (cubec_function_argument_t)vec_get(ast_args, i);
    if (!farg->type) {
      const char *pname = farg->identifier
          ? string_get(((cubec_literal_identifier_t)farg->identifier)->value)
          : "<anonymous>";
      vm_pop_scope(vm);
      for (size_t j = 0; j < param_count; j++) {
        if (entries[j].inferred_pack_values)
          allocator_free(allocator, &entries[j].inferred_pack_values);
      }
      allocator_free(allocator, &formal_types);
      allocator_free(allocator, &entries);
      return create_exception_value(vm,
          "generic inference: parameter '%s' requires type annotation",
          pname);
    }
    value_t type_val = run_expression(vm, farg->type, false);
    if (value_is_abnormal(type_val)) {
      vm_pop_scope(vm);
      for (size_t j = 0; j < param_count; j++) {
        if (entries[j].inferred_pack_values)
          allocator_free(allocator, &entries[j].inferred_pack_values);
      }
      allocator_free(allocator, &formal_types);
      allocator_free(allocator, &entries);
      return type_val;
    }
    if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE) {
      const char *got_name = type_get_name(value_get_type(type_val));
      vm_pop_scope(vm);
      for (size_t j = 0; j < param_count; j++) {
        if (entries[j].inferred_pack_values)
          allocator_free(allocator, &entries[j].inferred_pack_values);
      }
      allocator_free(allocator, &formal_types);
      allocator_free(allocator, &entries);
      return create_exception_value(vm,
          "generic inference: parameter type expression must produce a type, got '%s'",
          got_name);
    }
    formal_types[i] = (type_t)value_get_data(type_val);
  }

  /* ---- 5. Match formal types against actual arg types ---- */
  infer_ctx_t infer_ctx = _make_infer_ctx(allocator, entries, param_count);

  int pack_entry_idx = -1;
  for (size_t i = 0; i < param_count; i++) {
    if (entries[i].is_pack) {
      pack_entry_idx = (int)i;
      break;
    }
  }

  for (size_t i = 0; i < call_argc && i < formal_argc; i++) {
    type_t actual_type = value_get_type(call_argv[i]);

    if (pack_entry_idx >= 0 &&
        type_get_kind(formal_types[i]) == TYPE_KIND_GENERIC_PACK) {
      for (size_t j = i; j < call_argc; j++) {
        type_t arg_type = value_get_type(call_argv[j]);
        value_t arg_type_val = vm_create_value_ref(vm, type_type, arg_type, NULL);
        vec_push(entries[pack_entry_idx].inferred_pack_values, arg_type_val);
      }
      break;
    }

    if (!infer_walk_recurse(vm, infer_ctx, formal_types[i], actual_type)) {
      const char *formal_name = type_get_name(formal_types[i]);
      const char *actual_name = type_get_name(actual_type);
      vm_pop_scope(vm);
      for (size_t j = 0; j < param_count; j++) {
        if (entries[j].inferred_pack_values)
          allocator_free(allocator, &entries[j].inferred_pack_values);
      }
      allocator_free(allocator, &formal_types);
      allocator_free(allocator, &infer_ctx);
      allocator_free(allocator, &entries);
      return create_exception_value(vm,
          "generic inference: type mismatch at argument %zu: "
          "expected formal type '%s', got actual type '%s'",
          i, formal_name, actual_name);
    }
  }

  /* ---- 6. Save inferred data before popping temp scope ---- */
  typedef struct _saved_infer_t {
    type_t  value_type;
    void   *data;
    size_t  data_size;
  } saved_infer_t;

  saved_infer_t *saved = NULL;
  if (param_count > 0) {
    saved = (saved_infer_t *)allocator_alloc(
        allocator, param_count * sizeof(saved_infer_t));
    for (size_t i = 0; i < param_count; i++) {
      saved[i].value_type = NULL;
      saved[i].data = NULL;
      saved[i].data_size = 0;

      if (entries[i].is_pack) {
        /* Pack: handled separately after scope pop */
      } else if (entries[i].inferred_value) {
        type_t iv_type = value_get_type(entries[i].inferred_value);
        saved[i].value_type = iv_type;
        if (type_get_kind(iv_type) == TYPE_KIND_TYPE) {
          /* Type param: data is the inferred type pointer */
          saved[i].data = value_get_data(entries[i].inferred_value);
          saved[i].data_size = 0;
        } else {
          /* Value param: data is the concrete value bytes in value.data */
          size_t ds = (size_t)type_get_size(iv_type);
          if (ds > 0) {
            saved[i].data = allocator_alloc(allocator, ds);
            memcpy(saved[i].data, value_get_data(entries[i].inferred_value), ds);
            saved[i].data_size = ds;
          }
        }
      }
    }
  }

  vm_pop_scope(vm);

  /* Re-create inferred values in the parent scope */
  for (size_t i = 0; i < param_count; i++) {
    if (entries[i].is_pack) {
      vec_init_t pvi = {.auto_dispose = false};
      vec_t new_pack = (vec_t)allocator_create(allocator, &g_vec_class, &pvi);
      allocator_free(allocator, &entries[i].inferred_pack_values);
      entries[i].inferred_pack_values = new_pack;
    } else if (saved && saved[i].value_type) {
      if (type_get_kind(saved[i].value_type) == TYPE_KIND_TYPE) {
        entries[i].inferred_value =
            vm_create_value_ref(vm, type_type, saved[i].data, NULL);
      } else {
        /* Value param: saved data is the concrete value bytes.
         * vm_create_value copies the data, so we pass it directly. */
        entries[i].inferred_value =
            vm_create_value(vm, saved[i].value_type, saved[i].data, NULL);
      }
    }
  }

  if (saved) {
    for (size_t i = 0; i < param_count; i++) {
      if (saved[i].data_size > 0 && saved[i].data)
        allocator_free(allocator, &saved[i].data);
    }
    allocator_free(allocator, &saved);
  }

  /* ---- 7. Check all non-pack params have been inferred ---- */
  for (size_t i = 0; i < param_count; i++) {
    if (entries[i].is_pack) continue;
    if (!entries[i].inferred_value) {
      const char *pname = entries[i].name;
      for (size_t j = 0; j < param_count; j++) {
        if (entries[j].inferred_pack_values)
          allocator_free(allocator, &entries[j].inferred_pack_values);
      }
      allocator_free(allocator, &formal_types);
      allocator_free(allocator, &infer_ctx);
      allocator_free(allocator, &entries);
      return create_exception_value(vm,
          "generic inference: could not infer type parameter '%s'",
          pname);
    }
  }

  /* ---- 8. Build argv for value_instantiate ---- */
  vec_init_t inst_vi = {.auto_dispose = false};
  vec_t inst_argv = (vec_t)allocator_create(allocator, &g_vec_class, &inst_vi);

  for (size_t i = 0; i < param_count; i++) {
    if (entries[i].is_pack) {
      size_t pack_count = vec_get_size(entries[i].inferred_pack_values);
      for (size_t j = 0; j < pack_count; j++)
        vec_push(inst_argv, vec_get(entries[i].inferred_pack_values, j));
    } else {
      vec_push(inst_argv, entries[i].inferred_value);
    }
  }

  size_t inst_argc = vec_get_size(inst_argv);
  value_t *inst_argv_arr = NULL;
  if (inst_argc > 0) {
    inst_argv_arr = (value_t *)allocator_alloc(
        allocator, inst_argc * sizeof(value_t));
    for (size_t i = 0; i < inst_argc; i++)
      inst_argv_arr[i] = (value_t)vec_get(inst_argv, i);
  }
  allocator_free(allocator, &inst_argv);

  /* ---- 9. Instantiate ---- */
  value_t instance = value_instantiate(vm, generic_val, inst_argc, inst_argv_arr);

  if (inst_argv_arr)
    allocator_free(allocator, &inst_argv_arr);

  if (value_is_abnormal(instance)) {
    for (size_t i = 0; i < param_count; i++) {
      if (entries[i].inferred_pack_values)
        allocator_free(allocator, &entries[i].inferred_pack_values);
    }
    allocator_free(allocator, &formal_types);
    allocator_free(allocator, &infer_ctx);
    allocator_free(allocator, &entries);
    return instance;
  }

  /* ---- 10. Call ---- */
  value_t result = value_call(vm, instance, call_argc, call_argv);

  /* Clean up pack value vectors (created in step 1, replaced in step 6) */
  for (size_t i = 0; i < param_count; i++) {
    if (entries[i].inferred_pack_values)
      allocator_free(allocator, &entries[i].inferred_pack_values);
  }
  allocator_free(allocator, &formal_types);
  allocator_free(allocator, &infer_ctx);
  allocator_free(allocator, &entries);
  return result;
}
