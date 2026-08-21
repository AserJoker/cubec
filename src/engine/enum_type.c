#include "engine/enum_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/exception_type.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/type.h"
#include "engine/generic_inference.h"
#include "core/string.h"
#include "core/strmap.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* ---- Forward declarations for vtable functions ---- */

static value_t _enum_clone(vm_t vm, value_t self);
static value_t _enum_equal(vm_t vm, value_t a, value_t b);
static value_t _enum_type_equal(vm_t vm, type_t a, type_t b);
static value_t _enum_type_extends(vm_t vm, type_t sub, type_t super);
static value_t _enum_assignment(vm_t vm, value_t lvalue, value_t rvalue);
static value_t _enum_to_string(vm_t vm, value_t self);
static value_t _enum_type_get_prop(vm_t vm, type_t self, const char *name);

static type_t _enum_type_type_clone(vm_t vm, type_t self) {
  enum_type_t src = (enum_type_t)self;
  allocator_t allocator = vm_get_allocator(vm);
  enum_type_t dst = (enum_type_t)allocator_create(allocator, &g_enum_type_class, NULL);

  dst->base.kind    = src->base.kind;
  dst->base.name    = src->base.name ? cstring_clone(allocator, src->base.name) : NULL;
  dst->base.size    = src->base.size;
  dst->base.align   = src->base.align;
  dst->base.mut     = src->base.mut;
  dst->base.vtable  = src->base.vtable;

  dst->underlying   = src->underlying; /* borrowed: types managed by vm->types */

  /* clone isolated scope */
  dst->scope = scope_create(allocator, SCOPE_TYPE, NULL, NULL);

  /* rebuild items: for each src item, clone its value into dst->scope */
  strmap_init_t smi = {.value_auto_dispose = false};
  dst->items = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  strmap_iter_t it = strmap_iter_first(src->items);
  const char *key;
  while ((key = strmap_iter_next(&it)) != NULL) {
    value_t sv = (value_t)strmap_find(src->items, key);
    type_t  src_type = value_get_type(sv);
    void   *src_data = value_get_data(sv);
    uint64_t sz = type_get_size(src_type);
    void *new_data = NULL;
    if (sz > 0 && src_data) {
      new_data = allocator_alloc(allocator, sz);
      memcpy(new_data, src_data, (size_t)sz);
    }
    value_t cv = value_create(allocator, src_type, new_data, true);
    value_set_initialized(cv, value_is_initialized(sv));
    vec_push(dst->scope->values, cv);
    strmap_insert(dst->items, key, cv);
  }

  dst->module_id = src->module_id;

  vec_push(vm_get_types(vm), dst);
  return (type_t)dst;
}

/* ---- Shared vtable for all enum types ---- */

static vtable_t _make_enum_vtable(void) {
  return (vtable_t){
      .clone        = _enum_clone,
      .equal       = _enum_equal,
      .extends      = NULL,
      .type_equal   = _enum_type_equal,
      .type_extends = _enum_type_extends,
      .band         = NULL,
      .bor          = NULL,
      .bxor         = NULL,
      .add          = NULL,
      .sub          = NULL,
      .mul          = NULL,
      .div          = NULL,
      .mod          = NULL,
      .shl          = NULL,
      .shr          = NULL,
      .gt           = NULL,
      .lt           = NULL,
      .bnot         = NULL,
      .lnot         = NULL,
      .pos          = NULL,
      .neg          = NULL,
      .safe_cast    = NULL,  /* enum does not safe_cast to underlying type */
      .assignment   = _enum_assignment,
      .to_string    = _enum_to_string,
      .get_field    = NULL,
      .set_field    = NULL,
      .get_item     = NULL,
      .set_item     = NULL,
      .deref_get    = NULL,
      .deref_set    = NULL,
      .slice        = NULL,
      .call         = NULL,
      .member_call  = NULL,
      .get_prop     = NULL,
      .set_prop     = NULL,
      .type_get_prop= _enum_type_get_prop,
      .type_set_prop= NULL,
      .is_instance  = NULL,
      .get_field_raw= NULL,
      .type_clone   = _enum_type_type_clone,
      .infer_walk   = _enum_infer_walk,
  };
}

/* ---- g_enum_type_class lifecycle ---- */

static void _enum_type_init(void *self, allocator_t allocator, void *arg) {
  enum_type_t et = (enum_type_t)self;
  enum_type_init_t *init = (enum_type_init_t *)arg;
  et->base.kind    = init->kind;
  et->base.name    = init->name ? cstring_clone(allocator, init->name) : NULL;
  et->base.size    = init->size;
  et->base.align   = init->align;
  et->base.mut     = init->mut;
  et->base.vtable  = init->vtable;
  /* underlying is borrowed: types managed by vm->types */
  et->underlying   = init->underlying;
  /* isolated scope — enum_type_t fully owns and disposes it (no parent) */
  et->scope        = scope_create(allocator, SCOPE_TYPE, NULL, NULL);
  strmap_init_t smi = {.value_auto_dispose = false};
  et->items        = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  et->module_id    = init->module_id;
}

static void _enum_type_dispose(void *self, allocator_t allocator) {
  enum_type_t et = (enum_type_t)self;
  /* dispose items strmap (borrowed values, just free map) */
  allocator_free(allocator, &et->items);
  /* dispose owned scope (owns the item values) */
  if (et->scope) {
    scope_dispose(et->scope);
    et->scope = NULL;
  }
  /* underlying type is borrowed from vm->types — do not free */
  if (et->base.name) {
    void *p = et->base.name;
    allocator_free(allocator, &p);
    et->base.name = NULL;
  }
  et->module_id = NULL;
}

class_t g_enum_type_class = {
    .size    = sizeof(struct _enum_type_t),
    .name    = "cubec.engine.enum_type",
    .init    = (class_init_fn_t)_enum_type_init,
    .dispose = (class_dispose_fn_t)_enum_type_dispose,
    .clone   = NULL, /* types are global singletons — use vtable.type_clone instead */
    .move    = NULL,
};

/* ---- Type creation ---- */

enum_type_t enum_type_create(allocator_t allocator, const char *name,
                             type_t underlying, bool mut,
                             const char *module_id) {
  enum_type_init_t init = {
      .kind       = TYPE_KIND_ENUM,
      .name       = name,
      .size       = type_get_size(underlying),
      .align      = type_get_align(underlying),
      .mut        = mut,
      .vtable     = _make_enum_vtable(),
      .underlying = underlying,
      .module_id  = module_id,
  };
  return (enum_type_t)allocator_create(allocator, &g_enum_type_class, &init);
}

/* ---- Accessors ---- */

type_t enum_type_get_underlying(enum_type_t self) { return self->underlying; }
scope_t enum_type_get_scope(enum_type_t self)      { return self->scope; }
const char *enum_type_get_module_id(enum_type_t self) { return self->module_id; }

/* ---- Item management ---- */

value_t enum_type_add_item(vm_t vm, enum_type_t self,
                           const char *name, const void *item_data) {
  allocator_t alloc = vm_get_allocator(vm);
  if (strmap_find(self->items, name))
    return create_exception_value(vm, "duplicate enum item '%s' in enum '%s'",
                                  name, self->base.name ? self->base.name : "<anonymous>");

  uint64_t sz = type_get_size(self->underlying);
  void *buf = NULL;
  if (sz > 0) {
    buf = allocator_alloc(alloc, sz);
    if (item_data)
      memcpy(buf, item_data, (size_t)sz);
    else
      memset(buf, 0, (size_t)sz);
  }
  /* item value owns its own real underlying buffer (own=true) */
  value_t v = value_create(alloc, (type_t)self, buf, true);
  value_set_initialized(v, true);
  vec_push(self->scope->values, v);
  strmap_insert(self->items, name, v);
  return create_void_value(vm);
}

value_t enum_type_find_item(enum_type_t self, const char *name) {
  return (value_t)strmap_find(self->items, name);
}

/* ---- Value constructors ---- */

value_t create_enum_value(vm_t vm, enum_type_t et, const void *data) {
  allocator_t alloc = vm_get_allocator(vm);
  uint64_t sz = type_get_size((type_t)et);
  void *buf = NULL;
  if (sz > 0) {
    buf = allocator_alloc(alloc, sz);
    if (data)
      memcpy(buf, data, (size_t)sz);
    else
      memset(buf, 0, (size_t)sz);
  }
  value_t v = value_create(alloc, (type_t)et, buf, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

value_t create_enum_shadow(vm_t vm, enum_type_t et, bool initialized) {
  value_t v = value_create(vm_get_allocator(vm), (type_t)et, NULL, false);
  value_set_initialized(v, initialized);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ---- VM convenience ---- */

value_t vm_create_enum_type_value(vm_t vm, const char *name,
                                 value_t underlying_type_val, bool mut,
                                 const char *module_id) {
  type_t underlying = (type_t)value_get_data(underlying_type_val);
  enum_type_t et = enum_type_create(vm_get_allocator(vm), name,
                                    underlying, mut, module_id);
  vec_push(vm_get_types(vm), et);
  return create_type_value(vm, (type_t)et, NULL, false);
}

/* ================================================================== */
/* VTable: clone                                                       */
/* ================================================================== */

static value_t _enum_clone(vm_t vm, value_t self) {
  enum_type_t et = (enum_type_t)value_get_type(self);
  if (value_is_shadow(self))
    return create_enum_shadow(vm, et, value_is_initialized(self));

  type_t cloned_type = (type_t)et;

  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = et->base.size;
  void *dst = NULL;
  if (total_size > 0) {
    dst = allocator_alloc(alloc, total_size);
    memcpy(dst, value_get_data(self), total_size);
  }
  return create_enum_value(vm, (enum_type_t)cloned_type, dst);
}

/* ================================================================== */
/* VTable: equal (strict isolation — only same enum kind, compare underlying) */
/* ================================================================== */

static value_t _enum_equal(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (type_get_kind(tb) != TYPE_KIND_ENUM)
    return create_exception_value(vm,
        "cannot compare enum '%s' with non-enum type '%s'",
        type_get_name(value_get_type(a)), type_get_name(tb));
  if (value_get_type(a) != tb)
    return create_bool_value(vm, false); /* distinct enum types, strict isolation */
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);

  /* compare underlying values via the underlying type's equal */
  enum_type_t et = (enum_type_t)value_get_type(a);
  type_t ut = et->underlying;
  vtable_t uv = type_get_vtable(ut);
  if (!uv.equal)
    return create_exception_value(vm, "underlying type '%s' does not support equal",
                                  type_get_name(ut));

  allocator_t alloc = vm_get_allocator(vm);
  /* build temporary borrowed underlying values wrapping the raw buffers */
  value_t av = value_create(alloc, ut, value_get_data(a), false);
  value_t bv = value_create(alloc, ut, value_get_data(b), false);
  vec_t scope_values = NULL;
  scope_t scope = vm_get_current_scope(vm);
  if (scope) scope_values = scope->values;
  if (scope_values) { vec_push(scope_values, av); vec_push(scope_values, bv); }

  value_t eq = uv.equal(vm, av, bv);
  if (value_is_abnormal(eq))
    return eq;
  if (value_is_shadow(eq))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return eq;
}

/* ================================================================== */
/* VTable: type_equal                                                   */
/* ================================================================== */

static value_t _enum_type_equal(vm_t vm, type_t a, type_t b) {
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (b->kind != TYPE_KIND_ENUM)
    return create_bool_value(vm, false);
  enum_type_t ea = (enum_type_t)a;
  enum_type_t eb = (enum_type_t)b;
  /* distinct enum types are never equal (strict isolation) */
  if (a == b)
    return create_bool_value(vm, true);
  /* delegate to underlying type_equal for structural compatibility */
  vtable_t uv = type_get_vtable(ea->underlying);
  if (uv.type_equal)
    return uv.type_equal(vm, ea->underlying, eb->underlying);
  return create_bool_value(vm, type_get_kind(ea->underlying) == type_get_kind(eb->underlying));
}

/* ================================================================== */
/* VTable: type_extends (only self + wildcard)                          */
/* ================================================================== */

static value_t _enum_type_extends(vm_t vm, type_t sub, type_t super) {
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (super->kind != TYPE_KIND_ENUM)
    return create_bool_value(vm, false);
  return create_bool_value(vm, sub == super);
}

/* ================================================================== */
/* VTable: assignment                                                   */
/* ================================================================== */

static value_t _enum_assignment(vm_t vm, value_t lvalue, value_t rvalue) {
  type_t lt = value_get_type(lvalue);
  type_t rt = value_get_type(rvalue);
  if (type_get_kind(rt) != TYPE_KIND_ENUM)
    return create_exception_value(vm, "cannot assign non-enum to enum '%s'",
                                  type_get_name(lt));
  if (lt != rt)
    return create_exception_value(vm,
        "cannot assign enum '%s' to enum '%s' (strict isolation)",
        type_get_name(rt), type_get_name(lt));

  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {
    value_set_initialized(lvalue, true);
    return create_void_value(vm);
  }

  uint64_t sz = lt->size;
  if (sz > 0 && value_get_data(rvalue))
    memcpy(value_get_data(lvalue), value_get_data(rvalue), (size_t)sz);
  value_set_initialized(lvalue, true);
  return create_void_value(vm);
}

/* ================================================================== */
/* VTable: to_string                                                    */
/* ================================================================== */

static value_t _enum_to_string(vm_t vm, value_t self) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, value_get_type(self), NULL, true);
  enum_type_t et = (enum_type_t)value_get_type(self);
  /* try to find the item name whose underlying value matches */
  strmap_iter_t it = strmap_iter_first(et->items);
  const char *key;
  while ((key = strmap_iter_next(&it)) != NULL) {
    value_t item = (value_t)strmap_find(et->items, key);
    /* compare underlying buffers */
    uint64_t sz = et->base.size;
    if (sz > 0 && value_get_data(item) && value_get_data(self) &&
        memcmp(value_get_data(item), value_get_data(self), (size_t)sz) == 0) {
      size_t len = strlen(type_get_name((type_t)et)) + 2 + strlen(key);
      char *buf = (char *)allocator_alloc(vm_get_allocator(vm), len + 1);
      snprintf(buf, len + 1, "%s::%s", type_get_name((type_t)et), key);
      value_t result = create_str_value(vm, buf);
      allocator_free(vm_get_allocator(vm), &buf);
      return result;
    }
  }
  /* unknown underlying value */
  size_t len = strlen(type_get_name((type_t)et)) + strlen("::?");
  char *buf = (char *)allocator_alloc(vm_get_allocator(vm), len + 1);
  snprintf(buf, len + 1, "%s::?", type_get_name((type_t)et));
  value_t result = create_str_value(vm, buf);
  allocator_free(vm_get_allocator(vm), &buf);
  return result;
}

/* ================================================================== */
/* VTable: type_get_prop (Color::Red)                                   */
/* ================================================================== */

static value_t _enum_type_get_prop(vm_t vm, type_t self, const char *name) {
  enum_type_t et = (enum_type_t)self;
  value_t item = enum_type_find_item(et, name);
  if (!item)
    return create_exception_value(vm, "enum '%s' has no item '%s'",
                                  et->base.name ? et->base.name : "<anonymous>", name);
  /* shadow self → shadow item (compile-time type-only) */
  if (value_is_shadow(item))
    return create_enum_shadow(vm, et, value_is_initialized(item));
  return item;
}
