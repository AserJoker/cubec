#include "engine/cunion_type.h"
#include "engine/struct_type.h" /* field_info_t, g_field_info_class */
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/exception_type.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/pointer_type.h"
#include "engine/str_type.h"
#include "engine/type.h"
#include "core/string.h"
#include "core/vec.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* ---- Forward declarations for static helpers ---- */

static cunion_type_t _ct_create(allocator_t allocator, const char *name, bool mut,
                                const char *module_id);
static void _ct_add_field(allocator_t allocator, cunion_type_t ct,
                          const char *name, type_t field_type, bool pub);
static bool _ct_seal(cunion_type_t ct);
static vec_t _ct_get_fields(cunion_type_t self);
static scope_t _ct_get_scope(cunion_type_t self);
static bool _ct_is_sealed(cunion_type_t self);
static const char *_ct_get_module_id(cunion_type_t self);
static bool _ct_is_field_pub(cunion_type_t self, const char *name);
static field_info_t _ct_find_field(cunion_type_t self, const char *name);

/* ---- Forward declarations for vtable functions ---- */

static value_t _cunion_clone(vm_t vm, value_t self);
static value_t _cunion_equal(vm_t vm, value_t a, value_t b);
static value_t _cunion_type_equal(vm_t vm, type_t a, type_t b);
static value_t _cunion_type_extends(vm_t vm, type_t sub, type_t super);
static value_t _cunion_safe_cast(vm_t vm, value_t self, type_t to);
static value_t _cunion_assignment(vm_t vm, value_t lvalue, value_t rvalue);
static value_t _cunion_to_string(vm_t vm, value_t self);
static value_t _cunion_get_field(vm_t vm, value_t self, const char *name);
static value_t _cunion_set_field(vm_t vm, value_t self, const char *name, value_t val);

/* ---- Shared vtable for all cunion types ---- */

static vtable_t _make_cunion_vtable(void) {
  return (vtable_t){
      .clone        = _cunion_clone,
      .equal        = _cunion_equal,
      .extends      = NULL,
      .type_equal   = _cunion_type_equal,
      .type_extends = _cunion_type_extends,
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
      .safe_cast    = _cunion_safe_cast,
      .assignment   = _cunion_assignment,
      .to_string    = _cunion_to_string,
      .get_field    = _cunion_get_field,
      .set_field    = _cunion_set_field,
      .get_item     = NULL,
      .set_item     = NULL,
      .deref_get    = NULL,
      .deref_set    = NULL,
      .slice       = NULL,
      .call         = NULL,
      .member_call  = NULL,
      .get_prop     = NULL,
      .set_prop     = NULL,
      .type_get_prop= NULL,
      .type_set_prop= NULL,
      .is_instance  = NULL,
      .get_field_raw= NULL,
  };
}

/* ================================================================== */
/* cunion_type_t class                                                 */
/* ================================================================== */

/* field_info_init_t — matches struct_type.c internal layout */
typedef struct {
  const char *name;
  type_t      type;
  uint64_t    offset;
  bool        pub;
} _field_info_init_t;

static void _cunion_type_init(void *self, allocator_t allocator, void *arg) {
  cunion_type_t ct = (cunion_type_t)self;
  cunion_type_init_t *init = (cunion_type_init_t *)arg;

  ct->base.kind    = init->kind;
  ct->base.name    = init->name ? cstring_clone(allocator, init->name) : NULL;
  ct->base.size    = init->size;
  ct->base.align   = init->align;
  ct->base.mut     = init->mut;
  ct->base.vtable  = init->vtable;

  vec_init_t vi = {.auto_dispose = true};
  ct->fields  = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  ct->scope   = scope_create(allocator, SCOPE_TYPE, NULL, NULL);

  ct->sealed    = false;
  ct->module_id = init->module_id;
}

static void _cunion_type_dispose(void *self, allocator_t allocator) {
  cunion_type_t ct = (cunion_type_t)self;

  if (ct->scope) {
    scope_dispose(ct->scope);
    ct->scope = NULL;
  }

  allocator_free(allocator, &ct->fields);

  if (ct->base.name) {
    void *p = ct->base.name;
    allocator_free(allocator, &p);
    ct->base.name = NULL;
  }
}

static void _cunion_type_clone(void *self, allocator_t allocator, void *another) {
  cunion_type_t dst = (cunion_type_t)self;
  cunion_type_t src = (cunion_type_t)another;

  dst->base.kind    = src->base.kind;
  dst->base.name    = src->base.name ? cstring_clone(allocator, src->base.name) : NULL;
  dst->base.size    = src->base.size;
  dst->base.align   = src->base.align;
  dst->base.mut     = src->base.mut;
  dst->base.vtable  = src->base.vtable;

  /* clone scope — isolated, no parent (must be created before fields,
   * because _ct_add_field pushes cloned field types into scope->types) */
  dst->scope = scope_create(allocator, SCOPE_TYPE, NULL, NULL);

  /* clone fields — clone each field type into dst->scope->types */
  vec_init_t vi = {.auto_dispose = true};
  dst->fields = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  size_t fc = vec_get_size(src->fields);
  for (size_t i = 0; i < fc; i++) {
    field_info_t fi = (field_info_t)vec_get(src->fields, i);
    type_t cloned_type = (type_t)alloc_clone(allocator, field_info_get_type(fi));
    vec_push(dst->scope->types, cloned_type);
    _field_info_init_t fiinit = {
        .name   = field_info_get_name(fi),
        .type   = cloned_type,
        .offset = field_info_get_offset(fi),
        .pub    = field_info_is_pub(fi),
    };
    field_info_t cloned = (field_info_t)allocator_create(allocator,
                                                          &g_field_info_class, &fiinit);
    vec_push(dst->fields, cloned);
  }

  dst->sealed    = src->sealed;
  dst->module_id = src->module_id;
}

class_t g_cunion_type_class = {
    .size    = sizeof(struct _cunion_type_t),
    .name    = "cubec.engine.cunion_type",
    .init    = (class_init_fn_t)_cunion_type_init,
    .dispose = (class_dispose_fn_t)_cunion_type_dispose,
    .clone   = (class_clone_fn_t)_cunion_type_clone,
    .move    = NULL,
};

/* ================================================================== */
/* Type creation                                                       */
/* ================================================================== */

static uint64_t _align_up(uint64_t offset, uint64_t align) {
  if (align == 0) return offset;
  return (offset + align - 1) & ~(align - 1);
}

static cunion_type_t _ct_create(allocator_t allocator, const char *name, bool mut,
                                const char *module_id) {
  cunion_type_init_t init = {
      .kind      = TYPE_KIND_CUNION,
      .name      = name,
      .size      = 0,
      .align     = 1,
      .mut       = mut,
      .vtable    = _make_cunion_vtable(),
      .module_id = module_id,
  };

  cunion_type_t ct = (cunion_type_t)allocator_create(
      allocator, &g_cunion_type_class, &init);
  return ct;
}

static void _ct_add_field(allocator_t allocator, cunion_type_t ct,
                          const char *name, type_t field_type, bool pub) {
  if (ct->sealed) {
    fprintf(stderr, "error: cannot add field '%s' to sealed cunion type '%s'\n",
            name, ct->base.name ? ct->base.name : "<anonymous>");
    return;
  }
  if (_ct_find_field(ct, name)) {
    fprintf(stderr, "error: duplicate field '%s' in cunion type '%s'\n",
            name, ct->base.name ? ct->base.name : "<anonymous>");
    return;
  }

  /* Clone the field type into the cunion's own scope.
   * field_info_t.type borrows from scope->types, so the type's lifecycle
   * is tied to the cunion, not the caller's value. */
  type_t cloned = (type_t)alloc_clone(allocator, field_type);
  vec_push(ct->scope->types, cloned);

  uint64_t field_align = type_get_align(cloned);
  uint64_t field_size  = type_get_size(cloned);

  /* C-compatible union: all fields overlap at offset 0.
   * align = max(field aligns), size = max(field sizes) aligned up. */
  if (field_align > ct->base.align)
    ct->base.align = field_align;

  if (field_size > ct->base.size)
    ct->base.size = field_size;

  _field_info_init_t fiinit = {
      .name   = name,
      .type   = cloned,
      .offset = 0, /* C union: every field starts at offset 0 */
      .pub    = pub,
  };
  field_info_t fi = (field_info_t)allocator_create(allocator,
                                                     &g_field_info_class, &fiinit);
  vec_push(ct->fields, fi);
}

static bool _ct_seal(cunion_type_t ct) {
  if (ct->sealed) return true;
  if (vec_get_size(ct->fields) == 0) return false; /* empty cunion is invalid */

  /* C guarantee: size is at least the max field size, aligned to overall align.
   * C guarantees even an empty union has size >= 1. */
  ct->base.size = _align_up(ct->base.size, ct->base.align);
  if (ct->base.size == 0)
    ct->base.size = 1;
  ct->sealed = true;
  return true;
}

/* ================================================================== */
/* Accessors                                                           */
/* ================================================================== */

static vec_t    _ct_get_fields(cunion_type_t self)   { return self->fields; }
static scope_t  _ct_get_scope(cunion_type_t self)    { return self->scope; }
static bool     _ct_is_sealed(cunion_type_t self)    { return self->sealed; }
static const char *_ct_get_module_id(cunion_type_t self) { return self->module_id; }

static bool _ct_is_field_pub(cunion_type_t self, const char *name) {
  field_info_t fi = _ct_find_field(self, name);
  return fi ? field_info_is_pub(fi) : false;
}

static field_info_t _ct_find_field(cunion_type_t self, const char *name) {
  size_t fc = vec_get_size(self->fields);
  for (size_t i = 0; i < fc; i++) {
    field_info_t fi = (field_info_t)vec_get(self->fields, i);
    if (strcmp(field_info_get_name(fi), name) == 0)
      return fi;
  }
  return NULL;
}

/* ================================================================== */
/* Value constructors                                                  */
/* ================================================================== */

value_t vm_create_cunion_type_value(vm_t vm, const char *name,
                                    bool mut, const char *module_id) {
  cunion_type_t ct = _ct_create(vm_get_allocator(vm), name, mut, module_id);
  if (vm_get_current_scope(vm))
    vec_push(vm_get_current_scope(vm)->types, ct);
  return create_type_value(vm, (type_t)ct, NULL, false);
}

/* ================================================================== */

/** Internal: create cunion value from cunion_type_t directly. */
static value_t _create_cunion_value(vm_t vm, cunion_type_t ct,
                                    const char *field_name, value_t field_value) {
  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = ct->base.size;

  void *data = allocator_alloc(alloc, total_size);
  memset(data, 0, (size_t)total_size);

  if (field_value && value_get_data(field_value)) {
    field_info_t fi = _ct_find_field(ct, field_name);
    if (fi) {
      uint64_t fsize = type_get_size(field_info_get_type(fi));
      if (fsize > 0)
        memcpy(data, value_get_data(field_value), (size_t)fsize);
    }
  }

  value_t v = value_create(alloc, (type_t)ct, data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/** Internal: create cunion shadow from cunion_type_t directly. */
static value_t _create_cunion_shadow(vm_t vm, cunion_type_t ct, bool initialized) {
  value_t v = value_create(vm_get_allocator(vm), (type_t)ct, NULL, false);
  value_set_initialized(v, initialized);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ---- Public value-based API ---- */

value_t vm_create_cunion_value(vm_t vm, value_t type_val,
                               const char *field_name, value_t field_value) {
  cunion_type_t ct = (cunion_type_t)value_get_data(type_val);
  field_info_t fi = _ct_find_field(ct, field_name);
  if (!fi)
    return create_exception_value(vm, "cunion '%s' has no field '%s'",
                          type_get_name((type_t)ct), field_name);
  return _create_cunion_value(vm, ct, field_name, field_value);
}

value_t vm_create_cunion_shadow(vm_t vm, value_t type_val, bool initialized) {
  cunion_type_t ct = (cunion_type_t)value_get_data(type_val);
  return _create_cunion_shadow(vm, ct, initialized);
}

value_t _cunion_value_member_addr(vm_t vm, value_t self, const char *name) {
  cunion_type_t ct = (cunion_type_t)value_get_type(self);
  field_info_t fi = _ct_find_field(ct, name);
  if (!fi)
    return create_exception_value(vm, "cunion '%s' has no field '%s'",
                              type_get_name((type_t)ct), name);

  if (value_is_shadow(self) || !value_get_data(self))
    return create_exception_value(vm, "cannot take address of field in uninitialized cunion");

  /* C-compatible: no active-variant check, field is at offset 0. */
  allocator_t alloc = vm_get_allocator(vm);
  pointer_type_t pt = pointer_type_create(alloc, field_info_get_type(fi), true, false);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->types, pt);

  void *field_addr = value_get_data(self); /* offset 0 */
  return create_pointer_value_from_addr(vm, pt, field_addr);
}

/* ================================================================== */
/* VTable: clone                                                       */
/* ================================================================== */

static value_t _cunion_clone(vm_t vm, value_t self) {
  cunion_type_t ct = (cunion_type_t)value_get_type(self);
  if (value_is_shadow(self))
    return _create_cunion_shadow(vm, ct, value_is_initialized(self));

  type_t cloned_type = value_type_clone(vm, (type_t)ct);

  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = ct->base.size;
  void *data = NULL;
  if (total_size > 0) {
    data = allocator_alloc(alloc, total_size);
    memcpy(data, value_get_data(self), (size_t)total_size);
  }

  value_t v = value_create(alloc, cloned_type, data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ================================================================== */
/* VTable: equal                                                       */
/* ================================================================== */

static value_t _cunion_equal(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (type_get_kind(tb) != TYPE_KIND_CUNION)
    return create_bool_value(vm, false);
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);

  cunion_type_t cta = (cunion_type_t)value_get_type(a);
  cunion_type_t ctb = (cunion_type_t)value_get_type(b);

  /* C-compatible: compare the whole overlapping region byte-for-byte. */
  allocator_t alloc = vm_get_allocator(vm);
  value_t va = value_create(alloc, (type_t)cta, value_get_data(a), false);
  value_t vb = value_create(alloc, (type_t)ctb, value_get_data(b), false);

  uint64_t sz = cta->base.size < ctb->base.size ? cta->base.size : ctb->base.size;
  int cmp = sz > 0 ? memcmp(value_get_data(va), value_get_data(vb), (size_t)sz) : 0;
  allocator_free(alloc, &va);
  allocator_free(alloc, &vb);

  return create_bool_value(vm, cmp == 0);
}

/* ================================================================== */
/* VTable: type_equal (duck typing)                                    */
/* ================================================================== */

static value_t _cunion_type_equal(vm_t vm, type_t a, type_t b) {
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (b->kind != TYPE_KIND_CUNION)
    return create_bool_value(vm, false);

  cunion_type_t ca = (cunion_type_t)a;
  cunion_type_t cb = (cunion_type_t)b;

  if (a->mut != b->mut)
    return create_bool_value(vm, false);

  size_t fa_count = vec_get_size(ca->fields);
  size_t fb_count = vec_get_size(cb->fields);
  if (fa_count != fb_count)
    return create_bool_value(vm, false);

  for (size_t i = 0; i < fa_count; i++) {
    field_info_t fia = (field_info_t)vec_get(ca->fields, i);
    field_info_t fib = (field_info_t)vec_get(cb->fields, i);
    type_t ta = field_info_get_type(fia);
    type_t tb = fib ? field_info_get_type(fib) : NULL;
    vtable_t evt = type_get_vtable(ta);
    value_t eq;
    if (evt.type_equal)
      eq = evt.type_equal(vm, ta, tb);
    else
      eq = create_bool_value(vm, type_get_kind(ta) == type_get_kind(tb));
    if (value_is_error(eq))
      return eq;
    if (value_is_shadow(eq))
      return vm_create_value_shadow(vm, a, NULL, true);
    if (!(*(bool *)value_get_data(eq)))
      return create_bool_value(vm, false);
  }
  return create_bool_value(vm, true);
}

/* ================================================================== */
/* VTable: type_extends (cunion only, no interface)                    */
/* ================================================================== */

static value_t _cunion_type_extends(vm_t vm, type_t sub, type_t super) {
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (super->kind == TYPE_KIND_CUNION) {
    /* cunion extends cunion: structural equality (no subtyping) */
    return _cunion_type_equal(vm, sub, super);
  }
  /* cunion has no subtyping with other kinds */
  return create_bool_value(vm, false);
}

/* ================================================================== */
/* VTable: safe_cast (alias of assignment, no tag remap)               */
/* ================================================================== */

static value_t _cunion_safe_cast(vm_t vm, value_t self, type_t to) {
  type_t from = value_get_type(self);
  value_t eq = _cunion_type_equal(vm, from, to);
  if (value_is_error(eq))
    return eq;
  if (value_is_shadow(eq) || !(*(bool *)value_get_data(eq)))
    return create_exception_value(vm, "cannot safe_cast '%s' to '%s'",
                              type_get_name(from), type_get_name(to));

  if (value_is_shadow(self))
    return _create_cunion_shadow(vm, (cunion_type_t)to, value_is_initialized(self));

  cunion_type_t to_ct = (cunion_type_t)to;

  type_t cloned_type = value_type_clone(vm, to);

  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = to_ct->base.size;
  void *data = allocator_alloc(alloc, total_size);
  memset(data, 0, (size_t)total_size);

  /* copy min size; overlapping region reinterpreted (C semantics) */
  uint64_t copy_size = from->size < to_ct->base.size ? from->size : to_ct->base.size;
  if (copy_size > 0 && value_get_data(self))
    memcpy(data, value_get_data(self), (size_t)copy_size);

  value_t v = value_create(alloc, cloned_type, data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ================================================================== */
/* VTable: assignment                                                  */
/* ================================================================== */

static value_t _cunion_assignment(vm_t vm, value_t lvalue, value_t rvalue) {
  type_t lt = value_get_type(lvalue);
  if (value_is_initialized(lvalue) && !lt->mut)
    return create_exception_value(vm, "cannot assign to const '%s'", lt->name);

  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {
    value_set_initialized(lvalue, true);
    return create_void_value(vm);
  }

  value_t eq = _cunion_type_equal(vm, lt, value_get_type(rvalue));
  if (value_is_error(eq))
    return eq;
  if (!(*(bool *)value_get_data(eq)))
    return create_exception_value(vm, "cannot assign '%s' to '%s'",
                              type_get_name(value_get_type(rvalue)),
                              type_get_name(lt));

  uint64_t size = type_get_size(lt);
  if (size > 0 && value_get_data(rvalue))
    memcpy(value_get_data(lvalue), value_get_data(rvalue), (size_t)size);

  value_set_initialized(lvalue, true);
  return create_void_value(vm);
}

/* ================================================================== */
/* VTable: to_string                                                   */
/* ================================================================== */

static value_t _cunion_to_string(vm_t vm, value_t self) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, (type_t)value_get_data(vm_get_str_type(vm)), NULL, true);
  cunion_type_t ct = (cunion_type_t)value_get_type(self);
  allocator_t alloc = vm_get_allocator(vm);

  string_t result = (string_t)allocator_create(alloc, &g_string_class, NULL);

  if (ct->base.name)
    string_concat(result, ct->base.name);

  string_concat(result, "{ ");

  /* C-compatible: no active variant, list all fields sharing the region */
  size_t fc = vec_get_size(ct->fields);
  for (size_t i = 0; i < fc; i++) {
    field_info_t fi = (field_info_t)vec_get(ct->fields, i);
    if (i > 0) string_concat(result, ", ");
    string_concat(result, field_info_get_name(fi));
  }
  string_concat(result, " }");

  const char *cstr = string_get(result);
  value_t sv = create_str_value(vm, cstr);
  allocator_free(alloc, &result);
  return sv;
}

/* ================================================================== */
/* VTable: get_field / set_field (raw, no tag, no result wrapping)      */
/* ================================================================== */

static value_t _cunion_get_field(vm_t vm, value_t self, const char *name) {
  cunion_type_t ct = (cunion_type_t)value_get_type(self);
  field_info_t fi = _ct_find_field(ct, name);
  if (!fi)
    return create_exception_value(vm, "cunion '%s' has no field '%s'",
                              type_get_name((type_t)ct), name);

  if (value_is_shadow(self) || !value_get_data(self))
    return create_exception_value(vm, "cannot access field '%s' of uninitialized cunion", name);

  /* raw read from offset 0 — C-compatible, no active-variant check, no narrowing */
  uint64_t fsize = type_get_size(field_info_get_type(fi));
  allocator_t alloc = vm_get_allocator(vm);
  void *data = NULL;
  if (fsize > 0) {
    data = allocator_alloc(alloc, fsize);
    memcpy(data, value_get_data(self), (size_t)fsize);
  }
  value_t v = value_create(alloc, field_info_get_type(fi), data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

static value_t _cunion_set_field(vm_t vm, value_t self, const char *name, value_t val) {
  cunion_type_t ct = (cunion_type_t)value_get_type(self);
  field_info_t fi = _ct_find_field(ct, name);
  if (!fi)
    return create_exception_value(vm, "cunion '%s' has no field '%s'",
                              type_get_name((type_t)ct), name);

  if (!type_is_mut((type_t)ct))
    return create_exception_value(vm, "cannot assign to field of const cunion");

  value_t casted = value_safe_cast(vm, val, field_info_get_type(fi));
  if (value_is_error(casted))
    return casted;

  if (!value_is_shadow(self) && value_get_data(self)) {
    /* write at offset 0 — overwrites the whole shared region (C semantics) */
    if (value_get_data(casted)) {
      uint64_t fsize = type_get_size(field_info_get_type(fi));
      if (fsize > 0)
        memcpy(value_get_data(self),
               value_get_data(casted), (size_t)fsize);
    }
  } else {
    value_set_initialized(self, true);
  }

  return create_void_value(vm);
}

/* ================================================================== */
/* Value-based public API wrappers                                     */
/* ================================================================== */

static cunion_type_t _unwrap_cunion_type(vm_t vm, value_t type_val) {
  if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE)
    return NULL;
  type_t inner = (type_t)value_get_data(type_val);
  if (type_get_kind(inner) != TYPE_KIND_CUNION)
    return NULL;
  return (cunion_type_t)inner;
}

value_t vm_cunion_add_field(vm_t vm, value_t type_val,
                            const char *name, value_t field_type_val, bool pub) {
  cunion_type_t ct = _unwrap_cunion_type(vm, type_val);
  if (!ct)
    return create_exception_value(vm, "vm_cunion_add_field: expected cunion type value");
  type_t inner = (type_t)value_get_data(type_val);
  if (ct->sealed)
    return create_exception_value(vm, "cannot add field '%s' to sealed cunion type '%s'",
                                  name, type_get_name(inner));
  if (_ct_find_field(ct, name))
    return create_exception_value(vm, "duplicate field '%s' in cunion type '%s'",
                                  name, type_get_name(inner));
  type_t field_type = (type_t)value_get_data(field_type_val);
  _ct_add_field(vm_get_allocator(vm), ct, name, field_type, pub);
  return create_void_value(vm);
}

value_t vm_cunion_seal(vm_t vm, value_t type_val) {
  cunion_type_t ct = _unwrap_cunion_type(vm, type_val);
  if (!ct)
    return create_exception_value(vm, "vm_cunion_seal: expected cunion type value");
  if (!_ct_seal(ct))
    return create_exception_value(vm, "cannot seal empty cunion type '%s'",
                                  type_get_name((type_t)ct));
  return create_void_value(vm);
}

field_info_t vm_cunion_find_field(vm_t vm, value_t type_val, const char *name) {
  cunion_type_t ct = _unwrap_cunion_type(vm, type_val);
  (void)vm;
  if (!ct) return NULL;
  return _ct_find_field(ct, name);
}

vec_t vm_cunion_get_fields(vm_t vm, value_t type_val) {
  cunion_type_t ct = _unwrap_cunion_type(vm, type_val);
  (void)vm;
  if (!ct) return NULL;
  return _ct_get_fields(ct);
}

scope_t vm_cunion_get_scope(vm_t vm, value_t type_val) {
  cunion_type_t ct = _unwrap_cunion_type(vm, type_val);
  (void)vm;
  if (!ct) return NULL;
  return _ct_get_scope(ct);
}

bool vm_cunion_is_sealed(vm_t vm, value_t type_val) {
  cunion_type_t ct = _unwrap_cunion_type(vm, type_val);
  (void)vm;
  if (!ct) return false;
  return _ct_is_sealed(ct);
}

const char *vm_cunion_get_module_id(vm_t vm, value_t type_val) {
  cunion_type_t ct = _unwrap_cunion_type(vm, type_val);
  (void)vm;
  if (!ct) return NULL;
  return _ct_get_module_id(ct);
}

bool vm_cunion_is_field_pub(vm_t vm, value_t type_val, const char *name) {
  cunion_type_t ct = _unwrap_cunion_type(vm, type_val);
  (void)vm;
  if (!ct) return false;
  return _ct_is_field_pub(ct, name);
}

/* ---- Internal helpers (operate on cunion_type_t directly) ---- */

field_info_t _cunion_type_find_field(cunion_type_t ct, const char *name) {
  return _ct_find_field(ct, name);
}

value_t _cunion_type_create_value(vm_t vm, cunion_type_t ct,
                                  const char *field_name, value_t field_value) {
  return _create_cunion_value(vm, ct, field_name, field_value);
}
