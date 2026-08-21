#include "engine/slice_type.h"
#include "engine/array_type.h"
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

static value_t _slice_clone(vm_t vm, value_t self);
static value_t _slice_equal(vm_t vm, value_t a, value_t b);
static value_t _slice_type_equal(vm_t vm, type_t a, type_t b);
static value_t _slice_type_extends(vm_t vm, type_t sub, type_t super);
static value_t _slice_safe_cast(vm_t vm, value_t self, type_t to);
static value_t _slice_assignment(vm_t vm, value_t lvalue, value_t rvalue);
static value_t _slice_to_string(vm_t vm, value_t self);
static value_t _slice_get_item(vm_t vm, value_t self, value_t index);
static value_t _slice_set_item(vm_t vm, value_t self, value_t index, value_t val);
static value_t _slice_deref_get(vm_t vm, value_t self);
static value_t _slice_slice(vm_t vm, value_t self, uint64_t start, uint64_t count);

static type_t _slice_type_clone(vm_t vm, type_t self) {
  slice_type_t src = (slice_type_t)self;
  allocator_t allocator = vm_get_allocator(vm);
  slice_type_t dst = (slice_type_t)allocator_create(allocator, &g_slice_type_class, NULL);
  dst->base.kind    = src->base.kind;
  dst->base.name    = cstring_clone(allocator, src->base.name);
  dst->base.size    = src->base.size;
  dst->base.align   = src->base.align;
  dst->base.mut     = src->base.mut;
  dst->base.vtable  = src->base.vtable;
  dst->element_type = src->element_type; /* borrowed from vm->types */
  vec_push(vm_get_types(vm), dst);
  return (type_t)dst;
}

/* ---- Shared vtable for all slice types ---- */

static vtable_t _make_slice_vtable(void) {
  return (vtable_t){
      .clone        = _slice_clone,
      .equal        = _slice_equal,
      .extends      = NULL,
      .type_equal   = _slice_type_equal,
      .type_extends = _slice_type_extends,
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
      .safe_cast    = _slice_safe_cast,
      .assignment   = _slice_assignment,
      .to_string    = _slice_to_string,
      .get_field    = NULL,
      .set_field    = NULL,
      .get_item     = _slice_get_item,
      .set_item     = _slice_set_item,
      .deref_get    = _slice_deref_get,
      .deref_set    = NULL,
      .slice        = _slice_slice,
      .call         = NULL,
      .member_call  = NULL,
      .get_prop     = NULL,
      .set_prop     = NULL,
      .type_clone   = _slice_type_clone,
  };
}

/* ---- g_slice_type_class lifecycle ---- */

static void _slice_type_init(void *self, allocator_t allocator, void *arg) {
  slice_type_t st = (slice_type_t)self;
  slice_type_init_t *init = (slice_type_init_t *)arg;
  st->base.kind   = init->kind;
  st->base.name   = cstring_clone(allocator, init->name);
  st->base.size   = init->size;
  st->base.align  = init->align;
  st->base.mut    = init->mut;
  st->base.vtable = init->vtable;
  st->element_type = init->element_type; /* borrowed: types managed by vm->types */
}

static void _slice_type_dispose(void *self, allocator_t allocator) {
  slice_type_t st = (slice_type_t)self;
  allocator_free(allocator, &st->base.name);
  st->base.name = NULL;
  /* element_type is borrowed from vm->types — do not free */
  st->element_type = NULL;
}

static void _slice_type_move(void *self, allocator_t allocator, void *another) {
  (void)allocator;
  slice_type_t dst = (slice_type_t)self;
  slice_type_t src = (slice_type_t)another;
  *dst = *src;
  memset(src, 0, sizeof(*src));
}

class_t g_slice_type_class = {
    .size    = sizeof(struct _slice_type_t),
    .name    = "cubec.engine.slice_type",
    .init    = (class_init_fn_t)_slice_type_init,
    .dispose = (class_dispose_fn_t)_slice_type_dispose,
    .clone   = NULL, /* types are global singletons — alloc_clone aborts */
    .move    = (class_move_fn_t)_slice_type_move,
};

/* ---- Type creation ---- */

slice_type_t slice_type_create(allocator_t allocator, type_t element_type, bool mut) {
  const char *elem_name = type_get_name(element_type);
  size_t name_len = snprintf(NULL, 0, "[]%s", elem_name);
  char *name = (char *)allocator_alloc(allocator, name_len + 1);
  snprintf(name, name_len + 1, "[]%s", elem_name);

  slice_type_init_t init = {
      .kind         = TYPE_KIND_SLICE,
      .name         = name,
      .size         = sizeof(struct slice_data_t),
      .align        = _Alignof(struct slice_data_t),
      .mut          = mut,
      .vtable       = _make_slice_vtable(),
      .element_type = element_type,
  };

  slice_type_t st = (slice_type_t)allocator_create(
      allocator, &g_slice_type_class, &init);
  allocator_free(allocator, (void **)&name);
  return st;
}

/* ---- Accessors ---- */

type_t slice_type_get_element_type(slice_type_t self) { return self->element_type; }

/* ---- Helper: read slice_data_t from value ---- */

static struct slice_data_t *_slice_read(value_t v) {
  return (struct slice_data_t *)value_get_data(v);
}

/* ---- Helper: create element value from slice at index ---- */

static value_t _make_elem_from_slice(vm_t vm, slice_type_t st, value_t slice,
                                      uint64_t index) {
  struct slice_data_t *sd = _slice_read(slice);
  type_t elem_type = st->element_type;
  uint64_t elem_size = type_get_size(elem_type);
  allocator_t alloc = vm_get_allocator(vm);
  void *data = allocator_alloc(alloc, elem_size);
  memcpy(data, (char *)sd->ptr + sd->start + index * elem_size, elem_size);
  value_t v = value_create(alloc, elem_type, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ---- VTable: clone ---- */

static value_t _slice_clone(vm_t vm, value_t self) {
  slice_type_t src_st = (slice_type_t)value_get_type(self);

  if (value_is_shadow(self))
    return create_slice_shadow(vm, src_st, value_is_initialized(self));

  /* clone slice type into current scope */
  type_t cloned_type = (type_t)src_st;
  slice_type_t dst_st = (slice_type_t)cloned_type;

  /* clone slice data struct (shallow copy of ptr/start/len) */
  allocator_t alloc = vm_get_allocator(vm);
  struct slice_data_t *src_sd = _slice_read(self);
  struct slice_data_t *dst_sd = (struct slice_data_t *)allocator_alloc(
      alloc, sizeof(struct slice_data_t));
  *dst_sd = *src_sd;

  value_t v = value_create(alloc, (type_t)dst_st, dst_sd, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ---- VTable: equal ---- */

static value_t _slice_equal(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (tb->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (tb->kind != TYPE_KIND_SLICE)
    return create_exception_value(vm, "cannot compare slice with different kind");
  slice_type_t sa = (slice_type_t)value_get_type(a);
  slice_type_t sb = (slice_type_t)tb;
  if (type_get_kind(sa->element_type) != type_get_kind(sb->element_type))
    return create_exception_value(vm, "cannot compare slice of '%s' with slice of '%s'",
                                  type_get_name(sa->element_type),
                                  type_get_name(sb->element_type));
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  struct slice_data_t *da = _slice_read(a);
  struct slice_data_t *db = _slice_read(b);
  if (da->len != db->len)
    return create_bool_value(vm, false);
  for (uint64_t i = 0; i < da->len; i++) {
    value_t ea = _make_elem_from_slice(vm, sa, a, i);
    value_t eb = _make_elem_from_slice(vm, sb, b, i);
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

static value_t _slice_type_equal(vm_t vm, type_t a, type_t b) {
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (b->kind != TYPE_KIND_SLICE)
    return create_bool_value(vm, false);
  slice_type_t sa = (slice_type_t)a;
  slice_type_t sb = (slice_type_t)b;
  vtable_t elem_vt = type_get_vtable(sa->element_type);
  if (elem_vt.type_equal)
    return elem_vt.type_equal(vm, sa->element_type, sb->element_type);
  return create_bool_value(vm, type_get_kind(sa->element_type) == type_get_kind(sb->element_type));
}

/* ---- VTable: type_extends ---- */

static value_t _slice_type_extends(vm_t vm, type_t sub, type_t super) {
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (super->kind != TYPE_KIND_SLICE)
    return create_bool_value(vm, false);
  slice_type_t sub_st = (slice_type_t)sub;
  slice_type_t super_st = (slice_type_t)super;
  vtable_t elem_vt = type_get_vtable(sub_st->element_type);
  if (elem_vt.type_extends)
    return elem_vt.type_extends(vm, sub_st->element_type, super_st->element_type);
  return create_bool_value(vm, type_get_kind(sub_st->element_type) == type_get_kind(super_st->element_type));
}

/* ---- VTable: safe_cast ---- */

static value_t _slice_safe_cast(vm_t vm, value_t self, type_t to) {
  type_t from = value_get_type(self);
  if (to->kind != TYPE_KIND_SLICE)
    return create_exception_value(vm, "cannot safe_cast slice to '%s'", to->name);
  if (to == from)
    return self;
  slice_type_t from_st = (slice_type_t)from;
  slice_type_t to_st = (slice_type_t)to;
  if (type_get_kind(from_st->element_type) != type_get_kind(to_st->element_type))
    return create_exception_value(vm, "cannot safe_cast slice with different element type");
  if (!from->mut && to->mut)
    return create_exception_value(vm, "cannot safe_cast const slice to mut slice");
  if (value_is_shadow(self))
    return create_slice_shadow(vm, to_st, value_is_initialized(self));
  /* clone data struct with new type */
  allocator_t alloc = vm_get_allocator(vm);
  struct slice_data_t *src_sd = _slice_read(self);
  struct slice_data_t *dst_sd = (struct slice_data_t *)allocator_alloc(
      alloc, sizeof(struct slice_data_t));
  *dst_sd = *src_sd;
  value_t v = value_create(alloc, to, dst_sd, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ---- VTable: assignment ---- */

static value_t _slice_assignment(vm_t vm, value_t lvalue, value_t rvalue) {
  type_t rt = value_get_type(rvalue);
  if (rt->kind != TYPE_KIND_SLICE)
    return create_exception_value(vm, "cannot assign non-slice to slice");
  slice_type_t lst = (slice_type_t)value_get_type(lvalue);
  slice_type_t rst = (slice_type_t)rt;
  if (type_get_kind(lst->element_type) != type_get_kind(rst->element_type))
    return create_exception_value(vm, "cannot assign slice with different element type");
  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {
    value_set_initialized(lvalue, true);
    return create_void_value(vm);
  }
  /* copy slice_data_t (ptr/start/len) */
  struct slice_data_t *dst_sd = _slice_read(lvalue);
  struct slice_data_t *src_sd = _slice_read(rvalue);
  *dst_sd = *src_sd;
  value_set_initialized(lvalue, true);
  return create_void_value(vm);
}

/* ---- VTable: get_item ---- */

static value_t _slice_get_item(vm_t vm, value_t self, value_t index) {
  slice_type_t st = (slice_type_t)value_get_type(self);
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, st->element_type, NULL, true);
  struct slice_data_t *sd = _slice_read(self);
  uint64_t i = (uint64_t)(*(int32_t *)value_get_data(index));
  if (i >= sd->len)
    return create_exception_value(vm, "slice index %llu out of bounds (len %llu)",
                              (unsigned long long)i, (unsigned long long)sd->len);
  return _make_elem_from_slice(vm, st, self, i);
}

/* ---- VTable: set_item ---- */

static value_t _slice_set_item(vm_t vm, value_t self, value_t index, value_t val) {
  slice_type_t st = (slice_type_t)value_get_type(self);
  if (!st->base.mut)
    return create_exception_value(vm, "cannot set_item on const slice");
  if (value_is_shadow(self) || value_is_shadow(val)) {
    value_set_initialized(self, true);
    return create_void_value(vm);
  }
  struct slice_data_t *sd = _slice_read(self);
  uint64_t i = (uint64_t)(*(int32_t *)value_get_data(index));
  if (i >= sd->len)
    return create_exception_value(vm, "slice index %llu out of bounds (len %llu)",
                              (unsigned long long)i, (unsigned long long)sd->len);
  type_t elem_type = st->element_type;
  uint64_t elem_size = type_get_size(elem_type);
  memcpy((char *)sd->ptr + sd->start + i * elem_size,
         value_get_data(val), elem_size);
  return create_void_value(vm);
}

/* ---- VTable: deref_get — first element ---- */

static value_t _slice_deref_get(vm_t vm, value_t self) {
  slice_type_t st = (slice_type_t)value_get_type(self);
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, st->element_type, NULL, true);
  struct slice_data_t *sd = _slice_read(self);
  if (sd->len == 0)
    return create_exception_value(vm, "cannot dereference empty slice");
  return _make_elem_from_slice(vm, st, self, 0);
}

/* ---- VTable: slice ---- */

static value_t _slice_slice(vm_t vm, value_t self, uint64_t start,
                             uint64_t count) {
  slice_type_t st = (slice_type_t)value_get_type(self);
  struct slice_data_t *sd = _slice_read(self);
  if (start + count > sd->len)
    return create_exception_value(vm,
        "slice slice [%llu..%llu) out of bounds (len %llu)",
        (unsigned long long)start, (unsigned long long)(start + count),
        (unsigned long long)sd->len);
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, value_get_type(self), NULL, true);
  /* create new slice with adjusted start offset */
  allocator_t alloc = vm_get_allocator(vm);
  struct slice_data_t *new_sd = (struct slice_data_t *)allocator_alloc(
      alloc, sizeof(struct slice_data_t));
  uint64_t elem_size = type_get_size(st->element_type);
  new_sd->ptr   = sd->ptr;
  new_sd->start = sd->start + start * elem_size;
  new_sd->len   = count;

  /* clone slice type into current scope */
  type_t cloned_type = (type_t)st;
  value_t v = value_create(alloc, cloned_type, new_sd, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ---- VTable: to_string ---- */

static value_t _slice_to_string(vm_t vm, value_t self) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, value_get_type(self), NULL, true);
  slice_type_t st = (slice_type_t)value_get_type(self);
  (void)st;
  struct slice_data_t *sd = _slice_read(self);
  allocator_t alloc = vm_get_allocator(vm);
  string_t result = (string_t)allocator_create(alloc, &g_string_class, NULL);
  string_concat(result, "[");
  for (uint64_t i = 0; i < sd->len; i++) {
    if (i > 0) string_concat(result, ", ");
    value_t idx = create_i32_value(vm, (int32_t)i);
    value_t elem = _slice_get_item(vm, self, idx);
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

/* ---- Value constructors ---- */

value_t create_slice_value(vm_t vm, slice_type_t st,
                           value_t array_value, uint64_t start_elem, uint64_t count) {
  allocator_t alloc = vm_get_allocator(vm);
  type_t arr_type = value_get_type(array_value);
  /* must be an array with compatible element type */
  if (type_get_kind(arr_type) != TYPE_KIND_ARRAY)
    return create_exception_value(vm, "cannot create slice from non-array");
  array_type_t at = (array_type_t)arr_type;
  if (type_get_kind(at->element_type) != type_get_kind(st->element_type))
    return create_exception_value(vm, "slice element type does not match array");
  if (start_elem + count > array_type_get_count_value(at))
    return create_exception_value(vm, "slice range out of bounds");

  struct slice_data_t *sd = (struct slice_data_t *)allocator_alloc(
      alloc, sizeof(struct slice_data_t));
  sd->ptr   = value_get_data(array_value);
  sd->start = start_elem * type_get_size(st->element_type);
  sd->len   = count;

  value_t v = value_create(alloc, (type_t)st, sd, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

value_t create_slice_shadow(vm_t vm, slice_type_t st, bool initialized) {
  value_t v = value_create(vm_get_allocator(vm), (type_t)st, NULL, false);
  value_set_initialized(v, initialized);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}
