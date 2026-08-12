#include "engine/array_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/error_type.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/integer_type.h"
#include "engine/slice_type.h"
#include "engine/type.h"
#include "core/string.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* ---- Forward declarations for vtable functions ---- */

static value_t _array_clone(vm_t vm, value_t self);
static value_t _array_equal(vm_t vm, value_t a, value_t b);
static value_t _array_type_equal(vm_t vm, type_t a, type_t b);
static value_t _array_type_extends(vm_t vm, type_t sub, type_t super);
static value_t _array_safe_cast(vm_t vm, value_t self, type_t to);
static value_t _array_assignment(vm_t vm, value_t lvalue, value_t rvalue);
static value_t _array_to_string(vm_t vm, value_t self);
static value_t _array_get_item(vm_t vm, value_t self, value_t index);
static value_t _array_set_item(vm_t vm, value_t self, value_t index, value_t val);
static value_t _array_slice(vm_t vm, value_t self, uint64_t start, uint64_t count);

/* ---- Shared vtable for all array types ---- */

static vtable_t _make_array_vtable(void) {
  return (vtable_t){
      .clone        = _array_clone,
      .equal        = _array_equal,
      .extends      = NULL,
      .type_equal   = _array_type_equal,
      .type_extends = _array_type_extends,
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
      .safe_cast    = _array_safe_cast,
      .assignment   = _array_assignment,
      .to_string    = _array_to_string,
      .get_field    = NULL,
      .set_field    = NULL,
      .get_item     = _array_get_item,
      .set_item     = _array_set_item,
      .slice        = _array_slice,
      .call         = NULL,
      .member_call  = NULL,
      .get_prop     = NULL,
      .set_prop     = NULL,
  };
}

/* ---- g_array_type_class lifecycle ---- */

static void _array_type_init(void *self, allocator_t allocator, void *arg) {
  array_type_t at = (array_type_t)self;
  array_type_init_t *init = (array_type_init_t *)arg;
  at->base.kind   = init->kind;
  at->base.name   = cstring_clone(allocator, init->name);
  at->base.size   = init->size;
  at->base.align  = init->align;
  at->base.mut    = init->mut;
  at->base.vtable = init->vtable;
  at->element_type = (type_t)alloc_clone(allocator, init->element_type);
  at->count        = init->count;
}

static void _array_type_dispose(void *self, allocator_t allocator) {
  array_type_t at = (array_type_t)self;
  allocator_free(allocator, &at->base.name);
  at->base.name = NULL;
  allocator_free(allocator, &at->element_type);
  at->count = 0;
}

static void _array_type_clone(void *self, allocator_t allocator, void *another) {
  array_type_t dst = (array_type_t)self;
  array_type_t src = (array_type_t)another;
  dst->base.kind   = src->base.kind;
  dst->base.name   = cstring_clone(allocator, src->base.name);
  dst->base.size   = src->base.size;
  dst->base.align  = src->base.align;
  dst->base.mut    = src->base.mut;
  dst->base.vtable = src->base.vtable;
  dst->element_type = (type_t)alloc_clone(allocator, src->element_type);
  dst->count        = src->count;
}

static void _array_type_move(void *self, allocator_t allocator, void *another) {
  (void)allocator;
  array_type_t dst = (array_type_t)self;
  array_type_t src = (array_type_t)another;
  *dst = *src;
  memset(src, 0, sizeof(*src));
}

class_t g_array_type_class = {
    .size    = sizeof(struct _array_type_t),
    .name    = "cubec.engine.array_type",
    .init    = (class_init_fn_t)_array_type_init,
    .dispose = (class_dispose_fn_t)_array_type_dispose,
    .clone   = (class_clone_fn_t)_array_type_clone,
    .move    = (class_move_fn_t)_array_type_move,
};

/* ---- Type creation ---- */

array_type_t array_type_create(allocator_t allocator, type_t element_type,
                                uint64_t count, bool mut) {
  const char *elem_name = type_get_name(element_type);
  size_t name_len;
  char *name;
  if (count == WILDCARD_COUNT) {
    name_len = snprintf(NULL, 0, "[?]%s", elem_name);
    name = (char *)allocator_alloc(allocator, name_len + 1);
    snprintf(name, name_len + 1, "[?]%s", elem_name);
  } else {
    name_len = snprintf(NULL, 0, "[%llu]%s",
                               (unsigned long long)count, elem_name);
    name = (char *)allocator_alloc(allocator, name_len + 1);
    snprintf(name, name_len + 1, "[%llu]%s",
             (unsigned long long)count, elem_name);
  }

  uint64_t arr_size = (count == WILDCARD_COUNT) ? 0 : count * type_get_size(element_type);
  array_type_init_t init = {
      .kind         = TYPE_KIND_ARRAY,
      .name         = name,
      .size         = arr_size,
      .align        = type_get_align(element_type),
      .mut          = mut,
      .vtable       = _make_array_vtable(),
      .element_type = element_type,
      .count        = count,
  };

  array_type_t at = (array_type_t)allocator_create(
      allocator, &g_array_type_class, &init);
  allocator_free(allocator, (void **)&name);
  return at;
}

/* ---- Accessors ---- */

type_t   array_type_get_element_type(array_type_t self) { return self->element_type; }
uint64_t array_type_get_count(array_type_t self) { return self->count; }

/* ---- Helper: create a temporary element value from array buffer ---- */

static value_t _make_elem_value(vm_t vm, array_type_t at, value_t array,
                                 uint64_t index) {
  type_t elem_type = at->element_type;
  uint64_t elem_size = type_get_size(elem_type);
  allocator_t alloc = vm_get_allocator(vm);
  void *data = allocator_alloc(alloc, elem_size);
  memcpy(data, (char *)value_get_data(array) + index * elem_size, elem_size);
  value_t v = value_create(alloc, elem_type, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ---- VTable: clone ---- */

static value_t _array_clone(vm_t vm, value_t self) {
  array_type_t src_at = (array_type_t)value_get_type(self);

  if (value_is_shadow(self))
    return create_array_shadow(vm, src_at, value_is_initialized(self));

  /* clone array type into current scope (recursive) */
  type_t cloned_type = value_type_clone(vm, (type_t)src_at);
  array_type_t dst_at = (array_type_t)cloned_type;

  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = dst_at->base.size;
  void *dst = NULL;
  if (total_size > 0) {
    dst = allocator_alloc(alloc, total_size);
    memcpy(dst, value_get_data(self), total_size);
  }

  value_t v = value_create(alloc, (type_t)dst_at, dst, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ---- VTable: equal ---- */

static value_t _array_equal(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (tb->kind != TYPE_KIND_ARRAY)
    return create_error_value(vm, "cannot compare array with different kind");
  array_type_t at = (array_type_t)value_get_type(a);
  array_type_t bt = (array_type_t)tb;
  if (at->count != bt->count || type_get_kind(at->element_type) != type_get_kind(bt->element_type))
    return create_bool_value(vm, false);
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  for (uint64_t i = 0; i < at->count; i++) {
    value_t ea = _make_elem_value(vm, at, a, i);
    value_t eb = _make_elem_value(vm, bt, b, i);
    value_t eq = value_equal(vm, ea, eb);
    if (type_get_kind(value_get_type(eq)) == TYPE_KIND_ERROR)
      return eq;
    if (value_is_shadow(eq))
      return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
    if (!(*(bool *)value_get_data(eq)))
      return create_bool_value(vm, false);
  }
  return create_bool_value(vm, true);
}

/* ---- VTable: type_equal ---- */

static value_t _array_type_equal(vm_t vm, type_t a, type_t b) {
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (b->kind != TYPE_KIND_ARRAY)
    return create_bool_value(vm, false);
  array_type_t aa = (array_type_t)a;
  array_type_t ab = (array_type_t)b;
  if (aa->count != ab->count && ab->count != WILDCARD_COUNT)
    return create_bool_value(vm, false);
  /* delegate to element type's type_equal */
  vtable_t elem_vt = type_get_vtable(aa->element_type);
  if (elem_vt.type_equal)
    return elem_vt.type_equal(vm, aa->element_type, ab->element_type);
  /* fallback: same kind = equal */
  return create_bool_value(vm, type_get_kind(aa->element_type) == type_get_kind(ab->element_type));
}

/* ---- VTable: type_extends ---- */

static value_t _array_type_extends(vm_t vm, type_t sub, type_t super) {
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (super->kind != TYPE_KIND_ARRAY)
    return create_bool_value(vm, false);
  array_type_t sub_at = (array_type_t)sub;
  array_type_t super_at = (array_type_t)super;
  if (sub_at->count != super_at->count && super_at->count != WILDCARD_COUNT)
    return create_bool_value(vm, false);
  vtable_t elem_vt = type_get_vtable(sub_at->element_type);
  if (elem_vt.type_extends)
    return elem_vt.type_extends(vm, sub_at->element_type, super_at->element_type);
  return create_bool_value(vm, type_get_kind(sub_at->element_type) == type_get_kind(super_at->element_type));
}

/* ---- VTable: safe_cast ---- */

static value_t _array_safe_cast(vm_t vm, value_t self, type_t to) {
  type_t from = value_get_type(self);
  if (to->kind != TYPE_KIND_ARRAY)
    return create_error_value(vm, "cannot safe_cast array to '%s'", to->name);
  if (to == from)
    return self;
  array_type_t from_at = (array_type_t)from;
  array_type_t to_at = (array_type_t)to;
  if (from_at->count != to_at->count)
    return create_error_value(vm, "cannot safe_cast array with different count");
  if (type_get_kind(from_at->element_type) != type_get_kind(to_at->element_type))
    return create_error_value(vm, "cannot safe_cast array with different element type");
  if (!from->mut && to->mut)
    return create_error_value(vm, "cannot safe_cast const array to mut array");
  if (value_is_shadow(self))
    return create_array_shadow(vm, to_at, value_is_initialized(self));
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

static value_t _array_assignment(vm_t vm, value_t lvalue, value_t rvalue) {
  type_t rt = value_get_type(rvalue);
  if (rt->kind != TYPE_KIND_ARRAY)
    return create_error_value(vm, "cannot assign non-array to array");
  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {
    value_set_initialized(lvalue, true);
    return create_void_value(vm);
  }
  array_type_t lat = (array_type_t)value_get_type(lvalue);
  array_type_t rat = (array_type_t)rt;
  if (lat->count != rat->count || type_get_kind(lat->element_type) != type_get_kind(rat->element_type))
    return create_error_value(vm, "cannot assign array with different shape");
  memcpy(value_get_data(lvalue), value_get_data(rvalue), lat->base.size);
  value_set_initialized(lvalue, true);
  return create_void_value(vm);
}

/* ---- VTable: get_item ---- */

static value_t _array_get_item(vm_t vm, value_t self, value_t index) {
  array_type_t at = (array_type_t)value_get_type(self);
  uint64_t i = (uint64_t)(*(int32_t *)value_get_data(index));
  if (i >= at->count)
    return create_error_value(vm, "array index %llu out of bounds (size %llu)",
                              (unsigned long long)i, (unsigned long long)at->count);
  return _make_elem_value(vm, at, self, i);
}

/* ---- VTable: set_item ---- */

static value_t _array_set_item(vm_t vm, value_t self, value_t index, value_t val) {
  array_type_t at = (array_type_t)value_get_type(self);
  uint64_t i = (uint64_t)(*(int32_t *)value_get_data(index));
  if (i >= at->count)
    return create_error_value(vm, "array index %llu out of bounds (size %llu)",
                              (unsigned long long)i, (unsigned long long)at->count);
  type_t elem_type = at->element_type;
  uint64_t elem_size = type_get_size(elem_type);
  memcpy((char *)value_get_data(self) + i * elem_size,
         value_get_data(val), elem_size);
  return create_void_value(vm);
}

/* ---- VTable: to_string ---- */

static value_t _array_to_string(vm_t vm, value_t self) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, value_get_type(self), NULL, true);
  array_type_t at = (array_type_t)value_get_type(self);
  allocator_t alloc = vm_get_allocator(vm);
  string_t result = (string_t)allocator_create(alloc, &g_string_class, NULL);
  string_concat(result, "[");
  for (uint64_t i = 0; i < at->count; i++) {
    if (i > 0) string_concat(result, ", ");
    value_t idx = create_i32_value(vm, (int32_t)i);
    value_t elem = _array_get_item(vm, self, idx);
    if (type_get_kind(value_get_type(elem)) == TYPE_KIND_ERROR) {
      string_concat(result, "<error>");
    } else {
      value_t s = value_to_string(vm, elem);
      if (type_get_kind(value_get_type(s)) == TYPE_KIND_STR)
        string_concat(result, string_get(*(string_t *)value_get_data(s)));
      else
        string_concat(result, "...");
    }
  }
  string_concat(result, "]");
  const char *cstr = string_get(result);
  value_t sv = create_str_value(vm, cstr);
  allocator_free(alloc, &result);
  return sv;
}

/* ---- VTable: slice ---- */

static value_t _array_slice(vm_t vm, value_t self, uint64_t start,
                             uint64_t count) {
  array_type_t at = (array_type_t)value_get_type(self);
  if (start + count > at->count)
    return create_error_value(vm,
        "array slice [%llu..%llu) out of bounds (size %llu)",
        (unsigned long long)start, (unsigned long long)(start + count),
        (unsigned long long)at->count);
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, value_get_type(self), NULL, true);
  /* create slice type and value */
  type_t elem_type = at->element_type;
  value_t slice_type_val = vm_create_slice_type_value(vm, elem_type, at->base.mut);
  slice_type_t st = (slice_type_t)value_get_data(slice_type_val);
  return create_slice_value(vm, st, self, start, count);
}

/* ---- Value constructors ---- */

value_t create_array_value(vm_t vm, array_type_t at, value_t *elements) {
  allocator_t alloc = vm_get_allocator(vm);
  type_t elem_type = at->element_type;
  uint64_t elem_size = type_get_size(elem_type);
  uint64_t total_size = at->base.size;

  void *data = NULL;
  if (total_size > 0) {
    data = allocator_alloc(alloc, total_size);
    memset(data, 0, total_size);
    for (uint64_t i = 0; i < at->count; i++) {
      if (elements[i] && value_get_data(elements[i])) {
        memcpy((char *)data + i * elem_size,
               value_get_data(elements[i]), elem_size);
      }
    }
  }

  value_t v = value_create(alloc, (type_t)at, data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

value_t create_array_shadow(vm_t vm, array_type_t at, bool initialized) {
  value_t v = value_create(vm_get_allocator(vm), (type_t)at, NULL, false);
  value_set_initialized(v, initialized);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}
