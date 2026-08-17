#include "engine/tuple_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/exception_type.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/integer_type.h"
#include "engine/type.h"
#include "core/string.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* ---- Forward declarations for vtable functions ---- */

static value_t _tuple_clone(vm_t vm, value_t self);
static value_t _tuple_equal(vm_t vm, value_t a, value_t b);
static value_t _tuple_type_equal(vm_t vm, type_t a, type_t b);
static value_t _tuple_type_extends(vm_t vm, type_t sub, type_t super);
static value_t _tuple_safe_cast(vm_t vm, value_t self, type_t to);
static value_t _tuple_assignment(vm_t vm, value_t lvalue, value_t rvalue);
static value_t _tuple_to_string(vm_t vm, value_t self);
static value_t _tuple_get_item(vm_t vm, value_t self, value_t index);
static value_t _tuple_set_item(vm_t vm, value_t self, value_t index, value_t val);

/* ---- Shared vtable for all tuple types ---- */

static vtable_t _make_tuple_vtable(void) {
  return (vtable_t){
      .clone        = _tuple_clone,
      .equal        = _tuple_equal,
      .extends      = NULL,
      .type_equal   = _tuple_type_equal,
      .type_extends = _tuple_type_extends,
      .band         = NULL,
      .bor          = NULL,
      .bxor         = NULL,
      .bnot         = NULL,
      .lnot         = NULL,
      .add          = NULL,
      .sub          = NULL,
      .mul          = NULL,
      .div          = NULL,
      .mod          = NULL,
      .shl          = NULL,
      .shr          = NULL,
      .pos          = NULL,
      .neg          = NULL,
      .gt           = NULL,
      .lt           = NULL,
      .safe_cast    = _tuple_safe_cast,
      .assignment   = _tuple_assignment,
      .to_string    = _tuple_to_string,
      .get_field    = NULL,
      .set_field    = NULL,
      .get_item     = _tuple_get_item,
      .set_item     = _tuple_set_item,
      .deref_get    = NULL,
      .deref_set    = NULL,
      .slice        = NULL,
      .call         = NULL,
      .member_call  = NULL,
      .get_prop     = NULL,
      .set_prop     = NULL,
  };
}

/* ---- g_tuple_type_class lifecycle ---- */

static void _tuple_type_init(void *self, allocator_t allocator, void *arg) {
  tuple_type_t tt = (tuple_type_t)self;
  tuple_type_init_t *init = (tuple_type_init_t *)arg;
  tt->base.kind    = init->kind;
  tt->base.name    = cstring_clone(allocator, init->name);
  tt->base.size    = init->size;
  tt->base.align   = init->align;
  tt->base.mut     = init->mut;
  tt->base.vtable  = init->vtable;
  tt->field_count  = init->field_count;
  /* own element_types via alloc_clone (auto_dispose=true to free each type_t) */
  vec_init_t vi = {.auto_dispose = true};
  tt->element_types = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  for (uint64_t i = 0; i < init->field_count; i++) {
    type_t et = (type_t)vec_get(init->element_types, (size_t)i);
    type_t owned = (type_t)alloc_clone(allocator, et);
    vec_push(tt->element_types, owned);
  }
  /* clone offsets array */
  if (init->offsets && init->field_count > 0) {
    tt->offsets = (uint64_t *)allocator_alloc(allocator,
        init->field_count * sizeof(uint64_t));
    memcpy(tt->offsets, init->offsets,
           init->field_count * sizeof(uint64_t));
  } else {
    tt->offsets = NULL;
  }
}

static void _tuple_type_dispose(void *self, allocator_t allocator) {
  tuple_type_t tt = (tuple_type_t)self;
  allocator_free(allocator, &tt->base.name);
  tt->base.name = NULL;
  allocator_free(allocator, (void **)&tt->offsets);
  allocator_free(allocator, &tt->element_types); /* auto_dispose frees each type_t */
  tt->field_count = 0;
}

static void _tuple_type_clone(void *self, allocator_t allocator, void *another) {
  tuple_type_t dst = (tuple_type_t)self;
  tuple_type_t src = (tuple_type_t)another;
  dst->base.kind    = src->base.kind;
  dst->base.name    = cstring_clone(allocator, src->base.name);
  dst->base.size    = src->base.size;
  dst->base.align   = src->base.align;
  dst->base.mut     = src->base.mut;
  dst->base.vtable  = src->base.vtable;
  dst->field_count  = src->field_count;
  /* deep-clone element_types (auto_dispose=true, each alloc_clone'd) */
  vec_init_t vi = {.auto_dispose = true};
  dst->element_types = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  for (uint64_t i = 0; i < src->field_count; i++) {
    type_t et = (type_t)vec_get(src->element_types, (size_t)i);
    type_t owned = (type_t)alloc_clone(allocator, et);
    vec_push(dst->element_types, owned);
  }
  if (src->offsets && src->field_count > 0) {
    dst->offsets = (uint64_t *)allocator_alloc(allocator,
        src->field_count * sizeof(uint64_t));
    memcpy(dst->offsets, src->offsets,
           src->field_count * sizeof(uint64_t));
  } else {
    dst->offsets = NULL;
  }
}

static void _tuple_type_move(void *self, allocator_t allocator, void *another) {
  (void)allocator;
  tuple_type_t dst = (tuple_type_t)self;
  tuple_type_t src = (tuple_type_t)another;
  *dst = *src;
  memset(src, 0, sizeof(*src));
}

class_t g_tuple_type_class = {
    .size    = sizeof(struct _tuple_type_t),
    .name    = "cubec.engine.tuple_type",
    .init    = (class_init_fn_t)_tuple_type_init,
    .dispose = (class_dispose_fn_t)_tuple_type_dispose,
    .clone   = (class_clone_fn_t)_tuple_type_clone,
    .move    = (class_move_fn_t)_tuple_type_move,
};

/* ---- Alignment helper ---- */

static uint64_t _align_up(uint64_t value, uint64_t align) {
  return (value + align - 1) & ~(align - 1);
}

/* ---- Type creation ---- */

tuple_type_t tuple_type_create(allocator_t allocator, vec_t element_types,
                                bool mut) {
  uint64_t count = (uint64_t)vec_get_size(element_types);
  if (count == 0)
    return NULL; /* zero-field tuples are not semantically valid */

  /* compute offsets and total size/align */
  uint64_t *offsets = NULL;
  uint64_t total_size = 0;
  uint64_t total_align = 1;

  if (count > 0) {
    offsets = (uint64_t *)allocator_alloc(allocator, count * sizeof(uint64_t));
    for (uint64_t i = 0; i < count; i++) {
      type_t et = (type_t)vec_get(element_types, (size_t)i);
      uint64_t esize = type_get_size(et);
      uint64_t ealign = type_get_align(et);
      if (ealign > total_align)
        total_align = ealign;
      offsets[i] = _align_up(total_size, ealign);
      total_size = offsets[i] + esize;
    }
    /* pad total size to alignment */
    total_size = _align_up(total_size, total_align);
  }

  /* generate name: <T1, T2, ...> */
  string_t name_buf = (string_t)allocator_create(allocator, &g_string_class, NULL);
  string_concat(name_buf, "<");
  for (uint64_t i = 0; i < count; i++) {
    if (i > 0) string_concat(name_buf, ", ");
    type_t et = (type_t)vec_get(element_types, (size_t)i);
    string_concat(name_buf, type_get_name(et));
  }
  string_concat(name_buf, ">");
  const char *name = string_get(name_buf);

  tuple_type_init_t init = {
      .kind          = TYPE_KIND_TUPLE,
      .name          = name,
      .size          = total_size,
      .align         = total_align,
      .mut           = mut,
      .vtable        = _make_tuple_vtable(),
      .element_types = element_types,
      .offsets       = offsets,
      .field_count   = count,
  };

  tuple_type_t tt = (tuple_type_t)allocator_create(
      allocator, &g_tuple_type_class, &init);
  allocator_free(allocator, (void **)&offsets);
  allocator_free(allocator, &name_buf);
  return tt;
}

/* ---- Accessors ---- */

type_t   tuple_type_get_element_type(tuple_type_t self, uint64_t index) {
  return (type_t)vec_get(self->element_types, (size_t)index);
}
uint64_t tuple_type_get_field_count(tuple_type_t self) { return self->field_count; }
uint64_t tuple_type_get_offset(tuple_type_t self, uint64_t index) {
  return self->offsets[index];
}

/* ---- Helper: create element value from tuple buffer at index ---- */

static value_t _make_elem_from_tuple(vm_t vm, tuple_type_t tt, value_t tuple,
                                      uint64_t index) {
  type_t elem_type = (type_t)vec_get(tt->element_types, (size_t)index);
  uint64_t elem_size = type_get_size(elem_type);
  uint64_t offset = tt->offsets[index];
  allocator_t alloc = vm_get_allocator(vm);
  void *data = allocator_alloc(alloc, elem_size);
  memcpy(data, (char *)value_get_data(tuple) + offset, elem_size);
  value_t v = value_create(alloc, elem_type, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ---- VTable: clone ---- */

static value_t _tuple_clone(vm_t vm, value_t self) {
  tuple_type_t src_tt = (tuple_type_t)value_get_type(self);

  if (value_is_shadow(self))
    return create_tuple_shadow(vm, src_tt, value_is_initialized(self));

  /* clone tuple type into current scope (recursive) */
  type_t cloned_type = value_type_clone(vm, (type_t)src_tt);
  tuple_type_t dst_tt = (tuple_type_t)cloned_type;

  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = dst_tt->base.size;
  void *dst = NULL;
  if (total_size > 0) {
    dst = allocator_alloc(alloc, total_size);
    memcpy(dst, value_get_data(self), total_size);
  }

  value_t v = value_create(alloc, (type_t)dst_tt, dst, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ---- VTable: equal ---- */

static value_t _tuple_equal(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (tb->kind != TYPE_KIND_TUPLE)
    return create_exception_value(vm, "cannot compare tuple with different kind");
  tuple_type_t ta = (tuple_type_t)value_get_type(a);
  tuple_type_t tbb = (tuple_type_t)tb;
  if (ta->field_count != tbb->field_count)
    return create_bool_value(vm, false);
  for (uint64_t i = 0; i < ta->field_count; i++) {
    if (type_get_kind(tuple_type_get_element_type(ta, i)) != type_get_kind(tuple_type_get_element_type(tbb, i)))
      return create_bool_value(vm, false);
  }
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  for (uint64_t i = 0; i < ta->field_count; i++) {
    value_t ea = _make_elem_from_tuple(vm, ta, a, i);
    value_t eb = _make_elem_from_tuple(vm, tbb, b, i);
    value_t eq = value_equal(vm, ea, eb);
    if (value_is_error(eq))
      return eq;
    if (value_is_shadow(eq))
      return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
    if (!(*(bool *)value_get_data(eq)))
      return create_bool_value(vm, false);
  }
  return create_bool_value(vm, true);
}

/* ---- VTable: type_equal ---- */

static value_t _tuple_type_equal(vm_t vm, type_t a, type_t b) {
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  /* wildcard tuple: any tuple equal to <?> */
  if (b->kind == TYPE_KIND_TUPLE && b == (type_t)value_get_data(vm_get_wildcard_tuple_type(vm)))
    return create_bool_value(vm, true);
  if (b->kind != TYPE_KIND_TUPLE)
    return create_bool_value(vm, false);
  tuple_type_t ta = (tuple_type_t)a;
  tuple_type_t tbb = (tuple_type_t)b;
  if (ta->field_count != tbb->field_count)
    return create_bool_value(vm, false);
  for (uint64_t i = 0; i < ta->field_count; i++) {
    type_t ea = (type_t)vec_get(ta->element_types, (size_t)i);
    type_t eb = (type_t)vec_get(tbb->element_types, (size_t)i);
    /* wildcard element: skip comparison for this position */
    if (type_get_kind(eb) == TYPE_KIND_WILDCARD)
      continue;
    vtable_t elem_vt = type_get_vtable(ea);
    if (elem_vt.type_equal) {
      value_t eq = elem_vt.type_equal(vm, ea, eb);
      if (!(*(bool *)value_get_data(eq)))
        return eq;
    } else if (ea != eb) {
      return create_bool_value(vm, false);
    }
  }
  return create_bool_value(vm, true);
}

/* ---- VTable: type_extends ---- */

static value_t _tuple_type_extends(vm_t vm, type_t sub, type_t super) {
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (super->kind == TYPE_KIND_TUPLE && super == (type_t)value_get_data(vm_get_wildcard_tuple_type(vm)))
    return create_bool_value(vm, true);
  if (super->kind != TYPE_KIND_TUPLE)
    return create_bool_value(vm, false);
  tuple_type_t sub_tt = (tuple_type_t)sub;
  tuple_type_t super_tt = (tuple_type_t)super;
  if (sub_tt->field_count != super_tt->field_count)
    return create_bool_value(vm, false);
  for (uint64_t i = 0; i < sub_tt->field_count; i++) {
    type_t ea = (type_t)vec_get(sub_tt->element_types, (size_t)i);
    type_t eb = (type_t)vec_get(super_tt->element_types, (size_t)i);
    /* wildcard element: skip comparison for this position */
    if (type_get_kind(eb) == TYPE_KIND_WILDCARD)
      continue;
    vtable_t elem_vt = type_get_vtable(ea);
    if (elem_vt.type_extends) {
      value_t ext = elem_vt.type_extends(vm, ea, eb);
      if (!(*(bool *)value_get_data(ext)))
        return ext;
    } else if (ea != eb) {
      return create_bool_value(vm, false);
    }
  }
  return create_bool_value(vm, true);
}

/* ---- VTable: safe_cast ---- */

static value_t _tuple_safe_cast(vm_t vm, value_t self, type_t to) {
  type_t from = value_get_type(self);
  if (to->kind != TYPE_KIND_TUPLE)
    return create_exception_value(vm, "cannot safe_cast tuple to '%s'", to->name);
  if (to == from)
    return self;
  tuple_type_t from_tt = (tuple_type_t)from;
  tuple_type_t to_tt = (tuple_type_t)to;
  if (from_tt->field_count != to_tt->field_count)
    return create_exception_value(vm, "cannot safe_cast tuple with different field count");
  for (uint64_t i = 0; i < from_tt->field_count; i++) {
    if (type_get_kind(tuple_type_get_element_type(from_tt, i)) != type_get_kind(tuple_type_get_element_type(to_tt, i)))
      return create_exception_value(vm, "cannot safe_cast tuple with different element types");
  }
  if (!from->mut && to->mut)
    return create_exception_value(vm, "cannot safe_cast const tuple to mut tuple");
  if (value_is_shadow(self))
    return create_tuple_shadow(vm, to_tt, value_is_initialized(self));
  /* clone data buffer with new type */
  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = to->size;
  void *dst = NULL;
  if (total_size > 0) {
    dst = allocator_alloc(alloc, total_size);
    memcpy(dst, value_get_data(self), total_size);
  }
  value_t v = value_create(alloc, to, dst, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ---- VTable: assignment ---- */

static value_t _tuple_assignment(vm_t vm, value_t lvalue, value_t rvalue) {
  type_t rt = value_get_type(rvalue);
  if (rt->kind != TYPE_KIND_TUPLE)
    return create_exception_value(vm, "cannot assign non-tuple to tuple");
  tuple_type_t ltt = (tuple_type_t)value_get_type(lvalue);
  tuple_type_t rtt = (tuple_type_t)rt;
  if (ltt->field_count != rtt->field_count)
    return create_exception_value(vm, "cannot assign tuple with different field count");
  for (uint64_t i = 0; i < ltt->field_count; i++) {
    if (type_get_kind(tuple_type_get_element_type(ltt, i)) != type_get_kind(tuple_type_get_element_type(rtt, i)))
      return create_exception_value(vm, "cannot assign tuple with different element types");
  }
  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {
    value_set_initialized(lvalue, true);
    return create_void_value(vm);
  }
  /* copy entire data buffer */
  memcpy(value_get_data(lvalue), value_get_data(rvalue), ltt->base.size);
  value_set_initialized(lvalue, true);
  return create_void_value(vm);
}

/* ---- VTable: get_item ---- */

static value_t _tuple_get_item(vm_t vm, value_t self, value_t index) {
  tuple_type_t tt = (tuple_type_t)value_get_type(self);
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, (type_t)vec_get(tt->element_types, 0), NULL, true);
  uint64_t i = (uint64_t)(*(int32_t *)value_get_data(index));
  if (i >= tt->field_count)
    return create_exception_value(vm, "tuple index %llu out of bounds (fields %llu)",
                              (unsigned long long)i,
                              (unsigned long long)tt->field_count);
  return _make_elem_from_tuple(vm, tt, self, i);
}

/* ---- VTable: set_item ---- */

static value_t _tuple_set_item(vm_t vm, value_t self, value_t index, value_t val) {
  tuple_type_t tt = (tuple_type_t)value_get_type(self);
  if (!tt->base.mut)
    return create_exception_value(vm, "cannot set_item on const tuple");
  if (value_is_shadow(self) || value_is_shadow(val)) {
    value_set_initialized(self, true);
    return create_void_value(vm);
  }
  uint64_t i = (uint64_t)(*(int32_t *)value_get_data(index));
  if (i >= tt->field_count)
    return create_exception_value(vm, "tuple index %llu out of bounds (fields %llu)",
                              (unsigned long long)i,
                              (unsigned long long)tt->field_count);
  type_t elem_type = (type_t)vec_get(tt->element_types, (size_t)i);
  uint64_t elem_size = type_get_size(elem_type);
  uint64_t offset = tt->offsets[i];
  memcpy((char *)value_get_data(self) + offset, value_get_data(val), elem_size);
  return create_void_value(vm);
}

/* ---- VTable: to_string ---- */

static value_t _tuple_to_string(vm_t vm, value_t self) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, value_get_type(self), NULL, true);
  tuple_type_t tt = (tuple_type_t)value_get_type(self);
  allocator_t alloc = vm_get_allocator(vm);
  string_t result = (string_t)allocator_create(alloc, &g_string_class, NULL);
  string_concat(result, "<");
  for (uint64_t i = 0; i < tt->field_count; i++) {
    if (i > 0) string_concat(result, ", ");
    value_t idx = create_i32_value(vm, (int32_t)i);
    value_t elem = _tuple_get_item(vm, self, idx);
    if (value_is_error(elem)) {
      string_concat(result, "<error>");
    } else {
      value_t s = value_to_string(vm, elem);
      if (type_get_kind(value_get_type(s)) == TYPE_KIND_STR)
        string_concat(result, string_get(*(string_t *)value_get_data(s)));
      else
        string_concat(result, "...");
    }
  }
  string_concat(result, ">");
  const char *cstr = string_get(result);
  value_t sv = create_str_value(vm, cstr);
  allocator_free(alloc, &result);
  return sv;
}

/* ---- Value constructors ---- */

value_t create_tuple_value(vm_t vm, tuple_type_t tt, value_t *elements) {
  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = tt->base.size;

  void *data = NULL;
  if (total_size > 0) {
    data = allocator_alloc(alloc, total_size);
    memset(data, 0, total_size);
    for (uint64_t i = 0; i < tt->field_count; i++) {
      type_t elem_type = (type_t)vec_get(tt->element_types, (size_t)i);
      uint64_t elem_size = type_get_size(elem_type);
      uint64_t offset = tt->offsets[i];
      if (elements[i] && value_get_data(elements[i])) {
        memcpy((char *)data + offset, value_get_data(elements[i]), elem_size);
      }
    }
  }

  value_t v = value_create(alloc, (type_t)tt, data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

value_t create_tuple_shadow(vm_t vm, tuple_type_t tt, bool initialized) {
  value_t v = value_create(vm_get_allocator(vm), (type_t)tt, NULL, false);
  value_set_initialized(v, initialized);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}
