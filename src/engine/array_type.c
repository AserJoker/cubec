#include "engine/array_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/exception_type.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/integer_type.h"
#include "engine/wildcard_type.h"
#include "engine/slice_type.h"
#include "engine/type.h"
#include "engine/generic_inference.h"
#include "engine/name.h"
#include "core/string.h"
#include "core/strmap.h"
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
static vec_t   _array_spread(vm_t vm, value_t self);
static value_t _array_set_item(vm_t vm, value_t self, value_t index, value_t val);
static value_t _array_slice(vm_t vm, value_t self, uint64_t start, uint64_t count);

static type_t _array_type_clone(vm_t vm, type_t self) {
  array_type_t src = (array_type_t)self;
  allocator_t allocator = vm_get_allocator(vm);

  /* create new array type with same element_type and count value */
  array_type_t dst = (array_type_t)allocator_create(allocator, &g_array_type_class, NULL);
  dst->base.kind    = src->base.kind;
  dst->base.name    = cstring_clone(allocator, src->base.name);
  dst->base.size    = src->base.size;
  dst->base.align   = src->base.align;
  dst->base.mut     = src->base.mut;
  dst->base.vtable  = src->base.vtable;
  dst->element_type = src->element_type; /* borrowed from vm->types */

  /* isolated scope for count value lifecycle */
  dst->scope = scope_create(allocator, SCOPE_TYPE, NULL, NULL);

  /* clone count value into dst->scope */
  scope_t prev = vm_set_scope(vm, dst->scope);
  dst->count = value_clone(vm, src->count);
  vm_set_scope(vm, prev);

  vec_push(vm_get_types(vm), dst);
  return (type_t)dst;
}

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
      .type_clone   = _array_type_clone,
      .spread       = _array_spread,
      .infer_walk   = _array_infer_walk,
  };
}

/* ---- g_array_type_class lifecycle ---- */

static void _array_type_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  array_type_t at = (array_type_t)self;
  array_type_init_t *init = (array_type_init_t *)arg;
  at->base.kind   = init->kind;
  at->base.name   = init->name ? cstring_clone(allocator, init->name) : NULL;
  at->base.size   = init->size;
  at->base.align  = init->align;
  at->base.mut    = init->mut;
  at->base.vtable = init->vtable;
  at->element_type = init->element_type; /* borrowed: types managed by vm->types */
  at->count        = init->count;        /* set by array_type_create after scope setup */
  at->scope        = NULL;               /* set by array_type_create */
}

static void _array_type_dispose(void *self, allocator_t allocator) {
  array_type_t at = (array_type_t)self;
  allocator_free(allocator, &at->base.name);
  at->base.name = NULL;
  /* element_type is borrowed from vm->types — do not free */
  at->element_type = NULL;
  at->count = NULL;
  /* dispose isolated scope (owns count value) */
  if (at->scope) {
    scope_t s = at->scope;
    at->scope = NULL;
    scope_dispose(s);
  }
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
    .clone   = NULL, /* types are global singletons — use vtable.type_clone */
    .move    = (class_move_fn_t)_array_type_move,
};

/* ---- Type creation ---- */

array_type_t array_type_create(vm_t vm, type_t element_type,
                                value_t count, bool mut) {
  allocator_t allocator = vm_get_allocator(vm);
  bool is_wildcard = (type_get_kind(value_get_type(count)) == TYPE_KIND_WILDCARD);

  /* generate name */
  const char *elem_name = type_get_name(element_type);
  size_t name_len;
  char *name;
  if (is_wildcard) {
    name_len = snprintf(NULL, 0, "[?]%s", elem_name);
    name = (char *)allocator_alloc(allocator, name_len + 1);
    snprintf(name, name_len + 1, "[?]%s", elem_name);
  } else {
    uint64_t count_val = 0;
    type_kind_t ck = type_get_kind(value_get_type(count));
    if (ck >= TYPE_KIND_I8 && ck <= TYPE_KIND_U64) {
      memcpy(&count_val, value_get_data(count), (size_t)type_get_size(value_get_type(count)));
    }
    name_len = snprintf(NULL, 0, "[%llu]%s",
                               (unsigned long long)count_val, elem_name);
    name = (char *)allocator_alloc(allocator, name_len + 1);
    snprintf(name, name_len + 1, "[%llu]%s",
             (unsigned long long)count_val, elem_name);
  }

  uint64_t arr_size = 0;
  if (!is_wildcard) {
    uint64_t count_val = 0;
    type_kind_t ck = type_get_kind(value_get_type(count));
    if (ck >= TYPE_KIND_I8 && ck <= TYPE_KIND_U64) {
      memcpy(&count_val, value_get_data(count), (size_t)type_get_size(value_get_type(count)));
    }
    arr_size = count_val * type_get_size(element_type);
  }
  array_type_init_t init = {
      .kind         = TYPE_KIND_ARRAY,
      .name         = name,
      .size         = arr_size,
      .align        = type_get_align(element_type),
      .mut          = mut,
      .vtable       = _make_array_vtable(),
      .element_type = element_type,
      .count        = NULL, /* set below after scope creation */
  };

  array_type_t at = (array_type_t)allocator_create(
      allocator, &g_array_type_class, &init);
  allocator_free(allocator, (void **)&name);

  /* create isolated scope and register count value */
  at->scope = scope_create(allocator, SCOPE_TYPE, NULL, NULL);
  scope_t prev = vm_set_scope(vm, at->scope);
  at->count = value_clone(vm, count);
  vm_set_scope(vm, prev);

  return at;
}

/* ---- Accessors ---- */

type_t   array_type_get_element_type(array_type_t self) { return self->element_type; }
value_t  array_type_get_count(array_type_t self) { return self->count; }

uint64_t array_type_get_count_value(array_type_t self) {
  if (!self->count)
    return 0;
  type_t ct = value_get_type(self->count);
  type_kind_t kind = type_get_kind(ct);
  if (kind == TYPE_KIND_WILDCARD)
    return 0;
  /* integer types: extract value based on size */
  if (kind >= TYPE_KIND_I8 && kind <= TYPE_KIND_U64) {
    uint64_t val = 0;
    memcpy(&val, value_get_data(self->count), (size_t)type_get_size(ct));
    return val;
  }
  return 0; /* generic_param or other — not yet resolved */
}

bool array_type_is_count_wildcard(array_type_t self) {
  return self->count && type_get_kind(value_get_type(self->count)) == TYPE_KIND_WILDCARD;
}

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

/* ---- VTable: spread ---- */

static vec_t _array_spread(vm_t vm, value_t self) {
  array_type_t at = (array_type_t)value_get_type(self);
  if (value_is_shadow(self))
    return NULL; /* shadow arrays cannot spread runtime elements */
  allocator_t allocator = vm_get_allocator(vm);
  vec_init_t vi = {.auto_dispose = false};
  vec_t result = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  uint64_t count = array_type_get_count_value(at);
  for (uint64_t i = 0; i < count; i++) {
    value_t elem = _make_elem_value(vm, at, self, i);
    vec_push(result, elem);
  }
  return result;
}

/* ---- VTable: clone ---- */

static value_t _array_clone(vm_t vm, value_t self) {
  array_type_t src_at = (array_type_t)value_get_type(self);

  if (value_is_shadow(self))
    return create_array_shadow(vm, src_at, value_is_initialized(self));

  /* clone array type into current scope (recursive) */
  type_t cloned_type = (type_t)src_at;
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
  if (tb->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (tb->kind != TYPE_KIND_ARRAY)
    return create_exception_value(vm, "cannot compare array with different kind");
  array_type_t at = (array_type_t)value_get_type(a);
  array_type_t bt = (array_type_t)tb;
  uint64_t ac = array_type_get_count_value(at);
  uint64_t bc = array_type_get_count_value(bt);
  if (ac != bc || type_get_kind(at->element_type) != type_get_kind(bt->element_type))
    return create_exception_value(vm, "cannot compare array [%llu]%s with array [%llu]%s",
                                  (unsigned long long)ac,
                                  type_get_name(at->element_type),
                                  (unsigned long long)bc,
                                  type_get_name(bt->element_type));
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  for (uint64_t i = 0; i < ac; i++) {
    value_t ea = _make_elem_value(vm, at, a, i);
    value_t eb = _make_elem_value(vm, bt, b, i);
    value_t eq = value_equal(vm, ea, eb);
    if (value_is_abnormal(eq))
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
  /* count comparison via value_equal — wildcard count on right matches anything */
  value_t count_eq = value_equal(vm, aa->count, ab->count);
  if (value_is_abnormal(count_eq))
    return create_bool_value(vm, false);
  if (value_is_shadow(count_eq))
    return create_bool_value(vm, true); /* shadow = type-level match */
  if (!(*(bool *)value_get_data(count_eq)))
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
  /* count comparison via value_equal — wildcard count in super matches anything */
  if (!array_type_is_count_wildcard(super_at)) {
    value_t count_eq = value_equal(vm, sub_at->count, super_at->count);
    if (value_is_abnormal(count_eq) || value_is_shadow(count_eq))
      return create_bool_value(vm, false);
    if (!(*(bool *)value_get_data(count_eq)))
      return create_bool_value(vm, false);
  }
  vtable_t elem_vt = type_get_vtable(sub_at->element_type);
  if (elem_vt.type_extends)
    return elem_vt.type_extends(vm, sub_at->element_type, super_at->element_type);
  return create_bool_value(vm, type_get_kind(sub_at->element_type) == type_get_kind(super_at->element_type));
}

/* ---- VTable: safe_cast ---- */

static value_t _array_safe_cast(vm_t vm, value_t self, type_t to) {
  type_t from = value_get_type(self);
  if (to->kind != TYPE_KIND_ARRAY)
    return create_exception_value(vm, "cannot safe_cast array to '%s'", to->name);
  if (to == from)
    return self;
  array_type_t from_at = (array_type_t)from;
  array_type_t to_at = (array_type_t)to;
  uint64_t from_count = array_type_get_count_value(from_at);
  uint64_t to_count = array_type_get_count_value(to_at);
  if (from_count != to_count)
    return create_exception_value(vm, "cannot safe_cast array with different count");
  if (type_get_kind(from_at->element_type) != type_get_kind(to_at->element_type))
    return create_exception_value(vm, "cannot safe_cast array with different element type");
  if (!from->mut && to->mut)
    return create_exception_value(vm, "cannot safe_cast const array to mut array");
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
    return create_exception_value(vm, "cannot assign non-array to array");
  array_type_t lat = (array_type_t)value_get_type(lvalue);
  array_type_t rat = (array_type_t)rt;
  uint64_t lc = array_type_get_count_value(lat);
  uint64_t rc = array_type_get_count_value(rat);
  if (lc != rc || type_get_kind(lat->element_type) != type_get_kind(rat->element_type))
    return create_exception_value(vm, "cannot assign array with different shape");
  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {
    value_set_initialized(lvalue, true);
    return create_void_value(vm);
  }
  memcpy(value_get_data(lvalue), value_get_data(rvalue), lat->base.size);
  value_set_initialized(lvalue, true);
  return create_void_value(vm);
}

/* ---- VTable: get_item ---- */

static value_t _array_get_item(vm_t vm, value_t self, value_t index) {
  array_type_t at = (array_type_t)value_get_type(self);
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, at->element_type, NULL, true);
  uint64_t count = array_type_get_count_value(at);
  uint64_t i = (uint64_t)(*(int32_t *)value_get_data(index));
  if (i >= count)
    return create_exception_value(vm, "array index %llu out of bounds (size %llu)",
                              (unsigned long long)i, (unsigned long long)count);
  return _make_elem_value(vm, at, self, i);
}

/* ---- VTable: set_item ---- */

static value_t _array_set_item(vm_t vm, value_t self, value_t index, value_t val) {
  array_type_t at = (array_type_t)value_get_type(self);
  if (value_is_shadow(self) || value_is_shadow(val)) {
    value_set_initialized(self, true);
    return create_void_value(vm);
  }
  uint64_t count = array_type_get_count_value(at);
  uint64_t i = (uint64_t)(*(int32_t *)value_get_data(index));
  if (i >= count)
    return create_exception_value(vm, "array index %llu out of bounds (size %llu)",
                              (unsigned long long)i, (unsigned long long)count);
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
  uint64_t count = array_type_get_count_value(at);
  for (uint64_t i = 0; i < count; i++) {
    if (i > 0) string_concat(result, ", ");
    value_t idx = create_i32_value(vm, (int32_t)i);
    value_t elem = _array_get_item(vm, self, idx);
    if (value_is_abnormal(elem)) {
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
  uint64_t at_count = array_type_get_count_value(at);
  if (start + count > at_count)
    return create_exception_value(vm,
        "array slice [%llu..%llu) out of bounds (size %llu)",
        (unsigned long long)start, (unsigned long long)(start + count),
        (unsigned long long)at_count);
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
  uint64_t count = array_type_get_count_value(at);

  void *data = NULL;
  if (total_size > 0) {
    data = allocator_alloc(alloc, total_size);
    memset(data, 0, total_size);
    for (uint64_t i = 0; i < count; i++) {
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
