#include "engine/union_type.h"
#include "engine/struct_type.h" /* field_info_t, g_field_info_class */
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/exception_type.h"
#include "engine/error.h"
#include "engine/error_code.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/pointer_type.h"
#include "engine/callable_type.h"
#include "engine/interface_type.h"
#include "engine/result_type.h"
#include "engine/type.h"
#include "core/string.h"
#include "core/strmap.h"
#include "core/vec.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* ---- Access control helper ---- */

/** @brief Returns true if access should be denied: field/prop is private
 *  and caller's module differs from the type's owning module. */
static bool _access_denied(const char *type_module_id, bool is_pub,
                           const char *current_module_id) {
  if (is_pub) return false;
  if (type_module_id == current_module_id) return false;
  return true;
}
#include <stdint.h>

/* ---- Forward declarations for static helpers ---- */

static union_type_t _ut_create(allocator_t allocator, const char *name, bool mut,
                               const char *module_id);
static void _ut_add_field(allocator_t allocator, union_type_t ut,
                           const char *name, type_t field_type, bool pub);
static bool _ut_seal(union_type_t ut);
static void _ut_add_prop(vm_t vm, union_type_t ut,
                          const char *name, value_t val, bool is_method, bool pub);
static vec_t _ut_get_fields(union_type_t self);
static scope_t _ut_get_scope(union_type_t self);
static strmap_t _ut_get_props(union_type_t self);
static strmap_t _ut_get_methods(union_type_t self);
static bool _ut_is_sealed(union_type_t self);
static const char *_ut_get_module_id(union_type_t self);
static bool _ut_is_field_pub(union_type_t self, const char *name);
static bool _ut_is_prop_pub(union_type_t self, const char *name);
static field_info_t _ut_find_field(union_type_t self, const char *name);

/* ---- Forward declarations for vtable functions ---- */

static value_t _union_clone(vm_t vm, value_t self);
static value_t _union_equal(vm_t vm, value_t a, value_t b);
static value_t _union_type_equal(vm_t vm, type_t a, type_t b);
static value_t _union_type_extends(vm_t vm, type_t sub, type_t super);
static value_t _union_safe_cast(vm_t vm, value_t self, type_t to);
static value_t _union_assignment(vm_t vm, value_t lvalue, value_t rvalue);
static value_t _union_to_string(vm_t vm, value_t self);
static value_t _union_get_field(vm_t vm, value_t self, const char *name);
static value_t _union_get_field_raw(vm_t vm, value_t self, const char *name);
static value_t _union_set_field(vm_t vm, value_t self, const char *name, value_t val);
static value_t _union_member_call(vm_t vm, value_t self, const char *name,
                                   size_t argc, value_t *argv);
static value_t _union_type_get_prop(vm_t vm, type_t self, const char *name);
static value_t _union_type_set_prop(vm_t vm, type_t self, const char *name, value_t val);
static value_t _union_is_instance(vm_t vm, value_t self, type_t type);

/* ---- Shared vtable for all union types ---- */

static vtable_t _make_union_vtable(void) {
  return (vtable_t){
      .clone        = _union_clone,
      .equal        = _union_equal,
      .extends      = NULL,
      .type_equal   = _union_type_equal,
      .type_extends = _union_type_extends,
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
      .safe_cast    = _union_safe_cast,
      .assignment   = _union_assignment,
      .to_string    = _union_to_string,
      .get_field    = _union_get_field,
      .set_field    = _union_set_field,
      .get_item     = NULL,
      .set_item     = NULL,
      .deref_get    = NULL,
      .deref_set    = NULL,
      .slice        = NULL,
      .call         = NULL,
      .member_call  = _union_member_call,
      .get_prop     = NULL,
      .set_prop     = NULL,
      .type_get_prop= _union_type_get_prop,
      .type_set_prop= _union_type_set_prop,
      .is_instance  = _union_is_instance,
      .get_field_raw= _union_get_field_raw,
  };
}

/* ================================================================== */
/* union_type_t class                                                  */
/* ================================================================== */

/* field_info_init_t — matches struct_type.c internal layout */
typedef struct {
  const char *name;
  type_t      type;
  uint64_t    offset;
  bool        pub;
} _field_info_init_t;

static void _union_type_init(void *self, allocator_t allocator, void *arg) {
  union_type_t ut = (union_type_t)self;
  union_type_init_t *init = (union_type_init_t *)arg;

  ut->base.kind    = init->kind;
  ut->base.name    = init->name ? cstring_clone(allocator, init->name) : NULL;
  ut->base.size    = init->size;
  ut->base.align   = init->align;
  ut->base.mut     = init->mut;
  ut->base.vtable  = init->vtable;

  vec_init_t vi = {.auto_dispose = true};
  ut->fields  = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  ut->scope   = scope_create(allocator, SCOPE_TYPE, NULL, NULL);

  strmap_init_t smi = {.value_auto_dispose = false};
  ut->props        = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  ut->methods      = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  ut->pub_names    = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  ut->sealed       = false;
  ut->payload_size   = 0;
  ut->payload_offset = 0;
  ut->module_id      = init->module_id;
}

static void _union_type_dispose(void *self, allocator_t allocator) {
  union_type_t ut = (union_type_t)self;

  allocator_free(allocator, &ut->props);
  allocator_free(allocator, &ut->methods);
  allocator_free(allocator, &ut->pub_names);

  if (ut->scope) {
    scope_dispose(ut->scope);
    ut->scope = NULL;
  }

  allocator_free(allocator, &ut->fields);

  if (ut->base.name) {
    void *p = ut->base.name;
    allocator_free(allocator, &p);
    ut->base.name = NULL;
  }
}

static value_t _shallow_clone_value(allocator_t allocator, value_t src);

static value_t _shallow_clone_value(allocator_t allocator, value_t src) {
  extern class_t g_value_class;

  type_t  src_type = value_get_type(src);
  void   *src_data = value_get_data(src);
  bool    src_own   = value_is_own(src);
  bool    src_init  = value_is_initialized(src);

  void *new_data = src_data;
  if (src_own && src_data) {
    uint64_t sz = type_get_size(src_type);
    if (sz > 0) {
      new_data = allocator_alloc(allocator, sz);
      memcpy(new_data, src_data, (size_t)sz);
    }
  }

  typedef struct { type_t type; void *data; void *meta; bool own; bool initialized; } value_init_repr;
  value_init_repr init = { .type = src_type, .data = new_data, .meta = NULL, .own = src_own, .initialized = src_init };
  value_t dst = (value_t)allocator_create(allocator, &g_value_class, &init);
  return dst;
}

static void _union_type_clone(void *self, allocator_t allocator, void *another) {
  union_type_t dst = (union_type_t)self;
  union_type_t src = (union_type_t)another;

  dst->base.kind    = src->base.kind;
  dst->base.name    = src->base.name ? cstring_clone(allocator, src->base.name) : NULL;
  dst->base.size    = src->base.size;
  dst->base.align   = src->base.align;
  dst->base.mut     = src->base.mut;
  dst->base.vtable  = src->base.vtable;

  /* clone scope — isolated, no parent (must be created before fields,
   * because _ut_add_field pushes cloned field types into scope->types) */
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

  strmap_init_t smi = {.value_auto_dispose = false};
  dst->props = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  strmap_iter_t it = strmap_iter_first(src->props);
  const char *key;
  while ((key = strmap_iter_next(&it)) != NULL) {
    value_t sv = (value_t)strmap_find(src->props, key);
    value_t cv = _shallow_clone_value(allocator, sv);
    vec_push(dst->scope->values, cv);
    strmap_insert(dst->props, key, cv);
  }

  dst->methods = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  it = strmap_iter_first(src->methods);
  while ((key = strmap_iter_next(&it)) != NULL) {
    value_t sv = (value_t)strmap_find(src->methods, key);
    value_t cv = _shallow_clone_value(allocator, sv);
    vec_push(dst->scope->values, cv);
    strmap_insert(dst->methods, key, cv);
  }

  dst->sealed         = src->sealed;
  dst->payload_size   = src->payload_size;
  dst->payload_offset = src->payload_offset;
  dst->module_id      = src->module_id;

  /* clone pub_names */
  dst->pub_names = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  it = strmap_iter_first(src->pub_names);
  while ((key = strmap_iter_next(&it)) != NULL) {
    strmap_insert(dst->pub_names, key, (void *)1);
  }
}

class_t g_union_type_class = {
    .size    = sizeof(struct _union_type_t),
    .name    = "cubec.engine.union_type",
    .init    = (class_init_fn_t)_union_type_init,
    .dispose = (class_dispose_fn_t)_union_type_dispose,
    .clone   = (class_clone_fn_t)_union_type_clone,
    .move    = NULL,
};

/* ================================================================== */
/* Type creation                                                       */
/* ================================================================== */

static uint64_t _align_up(uint64_t offset, uint64_t align) {
  return (offset + align - 1) & ~(align - 1);
}

static union_type_t _ut_create(allocator_t allocator, const char *name, bool mut,
                                const char *module_id) {
  union_type_init_t init = {
      .kind      = TYPE_KIND_UNION,
      .name      = name,
      .size      = 0,
      .align     = _Alignof(uint32_t),
      .mut       = mut,
      .vtable    = _make_union_vtable(),
      .module_id = module_id,
  };

  union_type_t ut = (union_type_t)allocator_create(
      allocator, &g_union_type_class, &init);
  return ut;
}

static void _ut_add_field(allocator_t allocator, union_type_t ut,
                           const char *name, type_t field_type, bool pub) {
  if (ut->sealed) {
    fprintf(stderr, "error: cannot add field '%s' to sealed union type '%s'\n",
            name, ut->base.name ? ut->base.name : "<anonymous>");
    return;
  }
  if (_ut_find_field(ut, name)) {
    fprintf(stderr, "error: duplicate field '%s' in union type '%s'\n",
            name, ut->base.name ? ut->base.name : "<anonymous>");
    return;
  }

  /* Clone the field type into the union's own scope.
   * field_info_t.type borrows from scope->types, so the type's lifecycle
   * is tied to the union, not the caller's value. */
  type_t cloned = (type_t)alloc_clone(allocator, field_type);
  vec_push(ut->scope->types, cloned);

  uint64_t field_align = type_get_align(cloned);
  uint64_t field_size  = type_get_size(cloned);

  if (field_align > ut->base.align)
    ut->base.align = field_align;

  if (field_size > ut->payload_size)
    ut->payload_size = field_size;

  _field_info_init_t fiinit = {
      .name   = name,
      .type   = cloned,
      .offset = 0, /* set in seal */
      .pub    = pub,
  };
  field_info_t fi = (field_info_t)allocator_create(allocator,
                                                     &g_field_info_class, &fiinit);
  vec_push(ut->fields, fi);

  /* if pub, insert name into pub_names */
  if (pub)
    strmap_insert(ut->pub_names, name, (void *)1);
}

static bool _ut_seal(union_type_t ut) {
  if (ut->sealed) return true;
  if (vec_get_size(ut->fields) == 0) return false; /* empty union is invalid */

  ut->payload_offset = _align_up(sizeof(uint32_t), ut->base.align);

  /* set all field offsets to payload_offset */
  size_t fc = vec_get_size(ut->fields);
  for (size_t i = 0; i < fc; i++) {
    field_info_t fi = (field_info_t)vec_get(ut->fields, i);
    /* patch offset via the internal struct — field_info_t doesn't have a setter.
     * Since _field_info_init_t matches the layout, we can access directly
     * because struct_type.c defines the struct. We include struct_type.h for
     * field_info_t forward decl, and the actual definition is in struct_type.c.
     * Use the accessor pattern: create a new field_info with correct offset. */
    (void)fi;
    /* We need to set fi->offset but field_info_t is opaque.
     * Workaround: since g_field_info_class has no move, we dispose+re-create.
     * But that's complex. Instead, let's directly access via cast.
     * The struct _field_info_t layout is: { char* name; type_t type; uint64_t offset; }
     * This is guaranteed by the class init. We can safely cast and set. */
  }

  /* Since we can't set offset through accessors, we use a different approach:
   * Store payload_offset in union_type_t and compute field address on the fly. */
  (void)fc;

  ut->base.size = _align_up(ut->payload_offset + ut->payload_size, ut->base.align);
  /* C guarantee: even empty union has size >= 1 */
  if (ut->base.size == 0)
    ut->base.size = 1;
  ut->sealed = true;
  return true;
}

static void _ut_add_prop(vm_t vm, union_type_t ut,
                          const char *name, value_t val, bool is_method, bool pub) {
  if (strmap_find(ut->props, name)) {
    fprintf(stderr, "error: duplicate prop '%s' in union type '%s'\n",
            name, ut->base.name ? ut->base.name : "<anonymous>");
    return;
  }
  /* clone value into the union's own scope (owned lifecycle) */
  scope_t prev = vm_set_scope(vm, ut->scope);
  value_t cloned = value_clone(vm, val);
  vm_set_scope(vm, prev);
  strmap_insert(ut->props, name, cloned);
  if (is_method)
    strmap_insert(ut->methods, name, cloned);

  /* if pub, insert name into pub_names */
  if (pub)
    strmap_insert(ut->pub_names, name, (void *)1);
}

/* ================================================================== */
/* Accessors                                                           */
/* ================================================================== */

static vec_t    _ut_get_fields(union_type_t self)   { return self->fields; }
static scope_t  _ut_get_scope(union_type_t self)    { return self->scope; }
static strmap_t _ut_get_props(union_type_t self)     { return self->props; }
static strmap_t _ut_get_methods(union_type_t self)   { return self->methods; }
static bool     _ut_is_sealed(union_type_t self)     { return self->sealed; }
static const char *_ut_get_module_id(union_type_t self) { return self->module_id; }

static bool _ut_is_field_pub(union_type_t self, const char *name) {
  field_info_t fi = _ut_find_field(self, name);
  return fi ? field_info_is_pub(fi) : false;
}

static bool _ut_is_prop_pub(union_type_t self, const char *name) {
  return strmap_find(self->pub_names, name) != NULL;
}

static field_info_t _ut_find_field(union_type_t self, const char *name) {
  size_t fc = vec_get_size(self->fields);
  for (size_t i = 0; i < fc; i++) {
    field_info_t fi = (field_info_t)vec_get(self->fields, i);
    if (strcmp(field_info_get_name(fi), name) == 0)
      return fi;
  }
  return NULL;
}

/* ---- Helper: find field index ---- */

static uint32_t _union_find_field_index(union_type_t ut, field_info_t fi) {
  size_t fc = vec_get_size(ut->fields);
  for (size_t i = 0; i < fc; i++) {
    if (vec_get(ut->fields, i) == fi)
      return (uint32_t)i;
  }
  return UINT32_MAX;
}

/* ---- Helper: read/write tag from value data ---- */

static uint32_t _union_read_tag(value_t v) {
  struct union_data_t *ud = (struct union_data_t *)value_get_data(v);
  return ud->tag;
}

static void _union_write_tag(value_t v, uint32_t tag) {
  struct union_data_t *ud = (struct union_data_t *)value_get_data(v);
  ud->tag = tag;
}

/* ================================================================== */
/* Value constructors                                                  */
/* ================================================================== */
/* VM convenience                                                      */
/* ================================================================== */

value_t vm_create_union_type_value(vm_t vm, const char *name,
                                    bool mut, const char *module_id) {
  union_type_t ut = _ut_create(vm_get_allocator(vm), name, mut, module_id);
  if (vm_get_current_scope(vm))
    vec_push(vm_get_current_scope(vm)->types, ut);
  return create_type_value(vm, (type_t)ut, NULL, false);
}

/* ================================================================== */

/** Internal: create union value from union_type_t directly (for vtable/result_type use). */
static value_t _create_union_value(vm_t vm, union_type_t ut,
                                    uint32_t tag, value_t field_value) {
  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = ut->base.size;

  void *data = allocator_alloc(alloc, total_size);
  memset(data, 0, (size_t)total_size);

  struct union_data_t *ud = (struct union_data_t *)data;
  ud->tag = tag;

  if (field_value && value_get_data(field_value)) {
    field_info_t fi = (field_info_t)vec_get(ut->fields, tag);
    uint64_t fsize = type_get_size(field_info_get_type(fi));
    if (fsize > 0)
      memcpy((char *)data + ut->payload_offset,
             value_get_data(field_value), (size_t)fsize);
  }

  value_t v = value_create(alloc, (type_t)ut, data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/** Internal: create union shadow from union_type_t directly. */
static value_t _create_union_shadow(vm_t vm, union_type_t ut, bool initialized) {
  value_t v = value_create(vm_get_allocator(vm), (type_t)ut, NULL, false);
  value_set_initialized(v, initialized);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ---- Public value-based API ---- */

value_t vm_create_union_value(vm_t vm, value_t type_val,
                               const char *field_name, value_t field_value) {
  union_type_t ut = (union_type_t)value_get_data(type_val);
  field_info_t fi = _ut_find_field(ut, field_name);
  if (!fi)
    return create_exception_value(vm, "union '%s' has no field '%s'",
                          type_get_name((type_t)ut), field_name);
  uint32_t tag = _union_find_field_index(ut, fi);
  return _create_union_value(vm, ut, tag, field_value);
}

value_t vm_create_union_shadow(vm_t vm, value_t type_val, bool initialized) {
  union_type_t ut = (union_type_t)value_get_data(type_val);
  return _create_union_shadow(vm, ut, initialized);
}

value_t _union_value_member_addr(vm_t vm, value_t self, const char *name) {
  union_type_t ut = (union_type_t)value_get_type(self);
  field_info_t fi = _ut_find_field(ut, name);
  if (!fi)
    return create_exception_value(vm, "union '%s' has no field '%s'",
                              type_get_name((type_t)ut), name);

  /* access control: private field only accessible from same module */
  if (_access_denied(_ut_get_module_id(ut), field_info_is_pub(fi),
                     vm_get_current_module_id(vm)))
    return create_exception_value(vm, "cannot access private field '%s' of union '%s' from outside its module",
                              name, type_get_name((type_t)ut));

  if (value_is_shadow(self) || !value_get_data(self))
    return create_exception_value(vm, "cannot take address of field in uninitialized union");

  uint32_t tag = _union_read_tag(self);
  uint32_t field_idx = _union_find_field_index(ut, fi);
  if (tag != field_idx)
    return create_error_value(vm, ERROR_CODE_UNION_ADDR_INACTIVE,
                        "cannot take address of inactive field '%s' in union '%s'",
                        name, type_get_name((type_t)ut));

  allocator_t alloc = vm_get_allocator(vm);
  pointer_type_t pt = pointer_type_create(alloc, field_info_get_type(fi), true, false);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->types, pt);

  void *field_addr = (char *)value_get_data(self) + ut->payload_offset;
  return create_pointer_value_from_addr(vm, pt, field_addr);
}

/* ================================================================== */
/* VTable: clone                                                       */
/* ================================================================== */

static value_t _union_clone(vm_t vm, value_t self) {
  union_type_t ut = (union_type_t)value_get_type(self);
  if (value_is_shadow(self))
    return _create_union_shadow(vm, ut, value_is_initialized(self));

  type_t cloned_type = value_type_clone(vm, (type_t)ut);

  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = ut->base.size;
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

static value_t _union_equal(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (type_get_kind(tb) != TYPE_KIND_UNION)
    return create_bool_value(vm, false);
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);

  union_type_t uta = (union_type_t)value_get_type(a);
  union_type_t utb = (union_type_t)value_get_type(b);

  uint32_t tag_a = _union_read_tag(a);
  uint32_t tag_b = _union_read_tag(b);

  field_info_t fi_a = (field_info_t)vec_get(uta->fields, tag_a);
  field_info_t fi_b = (field_info_t)vec_get(utb->fields, tag_b);

  /* active fields must have the same name */
  if (strcmp(field_info_get_name(fi_a), field_info_get_name(fi_b)) != 0)
    return create_bool_value(vm, false);

  /* compare active field types */
  type_t type_a = field_info_get_type(fi_a);
  type_t type_b = field_info_get_type(fi_b);
  vtable_t evt = type_get_vtable(type_a);
  value_t teq;
  if (evt.type_equal)
    teq = evt.type_equal(vm, type_a, type_b);
  else
    teq = create_bool_value(vm, type_get_kind(type_a) == type_get_kind(type_b));

  if (type_get_kind(value_get_type(teq)) == TYPE_KIND_EXCEPTION)
    return teq;
  if (value_is_shadow(teq) || !(*(bool *)value_get_data(teq)))
    return create_bool_value(vm, false);

  /* compare active field values */
  allocator_t alloc = vm_get_allocator(vm);
  value_t va = value_create(alloc, type_a,
                             (char *)value_get_data(a) + uta->payload_offset, false);
  value_t vb = value_create(alloc, type_b,
                             (char *)value_get_data(b) + utb->payload_offset, false);

  value_t eq = value_equal(vm, va, vb);
  allocator_free(alloc, &va);
  allocator_free(alloc, &vb);

  if (type_get_kind(value_get_type(eq)) == TYPE_KIND_EXCEPTION)
    return eq;
  if (value_is_shadow(eq))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
  return eq;
}

/* ================================================================== */
/* VTable: type_equal (duck typing)                                    */
/* ================================================================== */

static value_t _union_type_equal(vm_t vm, type_t a, type_t b) {
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (b->kind != TYPE_KIND_UNION)
    return create_bool_value(vm, false);

  union_type_t ua = (union_type_t)a;
  union_type_t ub = (union_type_t)b;

  if (a->mut != b->mut)
    return create_bool_value(vm, false);

  size_t fa_count = vec_get_size(ua->fields);
  size_t fb_count = vec_get_size(ub->fields);
  if (fa_count != fb_count)
    return create_bool_value(vm, false);

  for (size_t i = 0; i < fa_count; i++) {
    field_info_t fia = (field_info_t)vec_get(ua->fields, i);
    field_info_t fib = (field_info_t)vec_get(ub->fields, i);
    type_t ta = field_info_get_type(fia);
    type_t tb = fib ? field_info_get_type(fib) : NULL;
    vtable_t evt = type_get_vtable(ta);
    value_t eq;
    if (evt.type_equal)
      eq = evt.type_equal(vm, ta, tb);
    else
      eq = create_bool_value(vm, type_get_kind(ta) == type_get_kind(tb));
    if (type_get_kind(value_get_type(eq)) == TYPE_KIND_EXCEPTION)
      return eq;
    if (value_is_shadow(eq))
      return vm_create_value_shadow(vm, a, NULL, true);
    if (!(*(bool *)value_get_data(eq)))
      return create_bool_value(vm, false);
  }
  return create_bool_value(vm, true);
}

/* ================================================================== */
/* VTable: type_extends (interface only)                               */
/* ================================================================== */

static value_t _union_type_extends(vm_t vm, type_t sub, type_t super) {
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (super->kind == TYPE_KIND_INTERFACE) {
    /* union implements interface: check all interface methods against union's methods */
    return _interface_type_check_extends(vm, (interface_type_t)super,
                                        _ut_get_methods((union_type_t)sub));
  }
  if (super->kind == TYPE_KIND_UNION) {
    /* union extends union: structural equality (no subtyping) */
    return _union_type_equal(vm, sub, super);
  }
  /* union has no subtyping with other kinds */
  return create_bool_value(vm, false);
}

/* ================================================================== */
/* VTable: safe_cast (tag remapping)                                   */
/* ================================================================== */

static value_t _union_safe_cast(vm_t vm, value_t self, type_t to) {
  type_t from = value_get_type(self);
  value_t eq = _union_type_equal(vm, from, to);
  if (type_get_kind(value_get_type(eq)) == TYPE_KIND_EXCEPTION)
    return eq;
  if (value_is_shadow(eq) || !(*(bool *)value_get_data(eq)))
    return create_exception_value(vm, "cannot safe_cast '%s' to '%s'",
                              type_get_name(from), type_get_name(to));

  if (value_is_shadow(self))
    return _create_union_shadow(vm, (union_type_t)to, value_is_initialized(self));

  union_type_t from_ut = (union_type_t)from;
  union_type_t to_ut   = (union_type_t)to;

  /* remap tag: find active field name in source, look up index in target */
  uint32_t src_tag = _union_read_tag(self);
  field_info_t src_fi = (field_info_t)vec_get(from_ut->fields, src_tag);
  const char *active_name = field_info_get_name(src_fi);

  field_info_t dst_fi = _ut_find_field(to_ut, active_name);
  if (!dst_fi)
    return create_exception_value(vm, "cannot safe_cast union: no matching field '%s'",
                              active_name);

  uint32_t dst_tag = _union_find_field_index(to_ut, dst_fi);

  /* create new value with cloned type and remapped tag */
  type_t cloned_type = value_type_clone(vm, to);

  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = to_ut->base.size;
  void *data = allocator_alloc(alloc, total_size);
  memset(data, 0, (size_t)total_size);

  struct union_data_t *ud = (struct union_data_t *)data;
  ud->tag = dst_tag;

  /* copy payload */
  uint64_t copy_size = from_ut->payload_size < to_ut->payload_size ?
                        from_ut->payload_size : to_ut->payload_size;
  if (copy_size > 0 && value_get_data(self))
    memcpy((char *)data + to_ut->payload_offset,
           (char *)value_get_data(self) + from_ut->payload_offset,
           (size_t)copy_size);

  value_t v = value_create(alloc, cloned_type, data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

/* ================================================================== */
/* VTable: assignment                                                  */
/* ================================================================== */

static value_t _union_assignment(vm_t vm, value_t lvalue, value_t rvalue) {
  type_t lt = value_get_type(lvalue);
  if (value_is_initialized(lvalue) && !lt->mut)
    return create_exception_value(vm, "cannot assign to const '%s'", lt->name);

  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {
    value_set_initialized(lvalue, true);
    return create_void_value(vm);
  }

  value_t eq = _union_type_equal(vm, lt, value_get_type(rvalue));
  if (type_get_kind(value_get_type(eq)) == TYPE_KIND_EXCEPTION)
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

static value_t _union_to_string(vm_t vm, value_t self) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, (type_t)value_get_data(vm_get_str_type(vm)), NULL, true);
  union_type_t ut = (union_type_t)value_get_type(self);
  allocator_t alloc = vm_get_allocator(vm);

  string_t result = (string_t)allocator_create(alloc, &g_string_class, NULL);

  if (ut->base.name)
    string_concat(result, ut->base.name);

  string_concat(result, "{");

  if (!value_is_shadow(self) && value_get_data(self)) {
    uint32_t tag = _union_read_tag(self);
    field_info_t fi = (field_info_t)vec_get(ut->fields, tag);
    string_concat(result, field_info_get_name(fi));
    string_concat(result, ": ");

    value_t fv = value_create(alloc, field_info_get_type(fi),
                               (char *)value_get_data(self) + ut->payload_offset, false);
    value_t fvs = value_to_string(vm, fv);
    allocator_free(alloc, &fv);
    if (type_get_kind(value_get_type(fvs)) == TYPE_KIND_STR) {
      string_t sdata = *(string_t *)value_get_data(fvs);
      string_concat(result, string_get(sdata));
    }
  } else {
    string_concat(result, "<uninit>");
  }

  string_concat(result, "}");

  const char *cstr = string_get(result);
  value_t sv = create_str_value(vm, cstr);
  allocator_free(alloc, &result);
  return sv;
}

/* ================================================================== */
/* VTable: get_field / set_field                                       */
/* ================================================================== */

/** @brief Read field data directly from union payload (no tag check, no result wrapping).
 *  VTable entry for path-narrowed access: after `if u is T`, the compiler can
 *  call get_field_raw to bypass the result[T,error] wrapper.
 *  Also used internally by _union_get_field and result methods. */
static value_t _union_get_field_raw(vm_t vm, value_t self, const char *name) {
  union_type_t ut = (union_type_t)value_get_type(self);
  field_info_t fi = _ut_find_field(ut, name);
  if (!fi)
    return create_exception_value(vm, "union '%s' has no field '%s'",
                              type_get_name((type_t)ut), name);

  /* access control: same as get_field */
  if (_access_denied(_ut_get_module_id(ut), field_info_is_pub(fi),
                     vm_get_current_module_id(vm)))
    return create_exception_value(vm, "cannot access private field '%s' of union '%s' from outside its module",
                              name, type_get_name((type_t)ut));

  if (value_is_shadow(self) || !value_get_data(self))
    return create_exception_value(vm, "cannot access field '%s' of uninitialized union", name);

  /* read directly from payload — no tag check, no result wrapping */
  uint64_t fsize = type_get_size(field_info_get_type(fi));
  allocator_t alloc = vm_get_allocator(vm);
  void *data = NULL;
  if (fsize > 0) {
    data = allocator_alloc(alloc, fsize);
    memcpy(data, (char *)value_get_data(self) + ut->payload_offset, (size_t)fsize);
  }
  value_t v = value_create(alloc, field_info_get_type(fi), data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

static value_t _union_get_field(vm_t vm, value_t self, const char *name) {
  union_type_t ut = (union_type_t)value_get_type(self);
  field_info_t fi = _ut_find_field(ut, name);
  if (!fi)
    return create_exception_value(vm, "union '%s' has no field '%s'",
                              type_get_name((type_t)ut), name);

  /* access control: private field only accessible from same module */
  if (_access_denied(_ut_get_module_id(ut), field_info_is_pub(fi),
                     vm_get_current_module_id(vm)))
    return create_exception_value(vm, "cannot access private field '%s' of union '%s' from outside its module",
                              name, type_get_name((type_t)ut));

  if (value_is_shadow(self) || !value_get_data(self))
    return create_exception_value(vm, "cannot access field '%s' of uninitialized union", name);

  /* create result[field_type, u64] — error is just a code, not a heavy struct */
  type_t field_type = field_info_get_type(fi);
  type_t u64_type = (type_t)value_get_data(vm_get_u64_type(vm));
  value_t rv = vm_create_result_type_value(vm, field_type, u64_type);
  union_type_t result_ut = (union_type_t)value_get_data(rv);

  /* check tag to determine which variant of result to return */
  uint32_t tag = _union_read_tag(self);
  uint32_t field_idx = _union_find_field_index(ut, fi);

  if (tag == field_idx) {
    /* active field → result.of_value(field_data) */
    value_t field_val = _union_get_field_raw(vm, self, name);
    return _create_union_value(vm, result_ut, 0, field_val);
  } else {
    /* inactive field → result.of_error(ERROR_CODE_UNION_INACTIVE_FIELD) */
    uint64_t code = ERROR_CODE_UNION_INACTIVE_FIELD;
    value_t err_val = vm_create_value(vm, u64_type, &code, NULL);
    return _create_union_value(vm, result_ut, 1, err_val);
  }
}

static value_t _union_set_field(vm_t vm, value_t self, const char *name, value_t val) {
  union_type_t ut = (union_type_t)value_get_type(self);
  field_info_t fi = _ut_find_field(ut, name);
  if (!fi)
    return create_exception_value(vm, "union '%s' has no field '%s'",
                              type_get_name((type_t)ut), name);

  /* access control: private field only accessible from same module */
  if (_access_denied(_ut_get_module_id(ut), field_info_is_pub(fi),
                     vm_get_current_module_id(vm)))
    return create_exception_value(vm, "cannot access private field '%s' of union '%s' from outside its module",
                              name, type_get_name((type_t)ut));

  if (!type_is_mut((type_t)ut))
    return create_exception_value(vm, "cannot assign to field of const union");

  value_t casted = value_safe_cast(vm, val, field_info_get_type(fi));
  if (type_get_kind(value_get_type(casted)) == TYPE_KIND_EXCEPTION)
    return casted;

  if (!value_is_shadow(self) && value_get_data(self)) {
    uint32_t field_idx = _union_find_field_index(ut, fi);

    /* update tag to this field */
    _union_write_tag(self, field_idx);

    /* write payload */
    if (value_get_data(casted)) {
      uint64_t fsize = type_get_size(field_info_get_type(fi));
      if (fsize > 0)
        memcpy((char *)value_get_data(self) + ut->payload_offset,
               value_get_data(casted), (size_t)fsize);
    }
  } else {
    value_set_initialized(self, true);
  }

  return create_void_value(vm);
}

/* ================================================================== */
/* VTable: member_call                                                 */
/* ================================================================== */

static value_t _union_member_call(vm_t vm, value_t self, const char *name,
                                   size_t argc, value_t *argv) {
  union_type_t ut = (union_type_t)value_get_type(self);

  value_t method = (value_t)strmap_find(ut->methods, name);
  if (!method)
    return create_exception_value(vm, "union '%s' has no method '%s'",
                              type_get_name((type_t)ut), name);

  value_t addr = value_addrof(vm, self);

  allocator_t alloc = vm_get_allocator(vm);
  size_t new_argc = argc + 1;
  value_t *new_argv = (value_t *)allocator_alloc(alloc, sizeof(value_t) * new_argc);
  new_argv[0] = addr;
  for (size_t i = 0; i < argc; i++)
    new_argv[i + 1] = argv[i];

  value_t result = value_call(vm, method, new_argc, new_argv);

  allocator_free(alloc, &new_argv);
  return result;
}

/* ================================================================== */
/* VTable: type_get_prop / type_set_prop                               */
/* ================================================================== */

static value_t _union_type_get_prop(vm_t vm, type_t self, const char *name) {
  union_type_t ut = (union_type_t)self;
  value_t val = (value_t)strmap_find(ut->props, name);
  if (!val)
    return create_exception_value(vm, "union '%s' has no static property '%s'",
                              type_get_name(self), name);

  /* access control: private prop only accessible from same module */
  if (_access_denied(_ut_get_module_id(ut), _ut_is_prop_pub(ut, name),
                     vm_get_current_module_id(vm)))
    return create_exception_value(vm, "cannot access private property '%s' of union '%s' from outside its module",
                              name, type_get_name(self));

  return val;
}

static value_t _union_type_set_prop(vm_t vm, type_t self, const char *name, value_t val) {
  union_type_t ut = (union_type_t)self;
  value_t existing = (value_t)strmap_find(ut->props, name);
  if (!existing)
    return create_exception_value(vm, "union '%s' has no static property '%s'",
                              type_get_name(self), name);

  /* access control: private prop only accessible from same module */
  if (_access_denied(_ut_get_module_id(ut), _ut_is_prop_pub(ut, name),
                     vm_get_current_module_id(vm)))
    return create_exception_value(vm, "cannot access private property '%s' of union '%s' from outside its module",
                              name, type_get_name(self));

  if (value_is_initialized(existing) && !type_is_mut(value_get_type(existing)))
    return create_exception_value(vm, "cannot assign to const static property '%s'", name);

  value_t result = value_assignment(vm, existing, val);
  if (type_get_kind(value_get_type(result)) == TYPE_KIND_EXCEPTION)
    return result;

  return create_void_value(vm);
}

/* ================================================================== */
/* VTable: is_instance                                                 */
/* ================================================================== */

static value_t _union_is_instance(vm_t vm, value_t self, type_t type) {
  union_type_t ut = (union_type_t)value_get_type(self);

  /* shadow → shadow bool */
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, (type_t)value_get_data(vm_get_bool_type(vm)),
                                  NULL, true);

  /* read active tag */
  uint32_t tag = _union_read_tag(self);
  if (tag >= vec_get_size(ut->fields))
    return create_bool_value(vm, false);

  /* get active field's type */
  field_info_t fi = (field_info_t)vec_get(ut->fields, tag);
  type_t active_type = field_info_get_type(fi);

  /* compare with target type via type_equal */
  if (type == active_type)
    return create_bool_value(vm, true);

  vtable_t evt = type_get_vtable(active_type);
  if (evt.type_equal)
    return evt.type_equal(vm, active_type, type);

  return create_bool_value(vm, type_get_kind(active_type) == type_get_kind(type));
}

/* ================================================================== */
/* Value-based public API wrappers                                     */
/* ================================================================== */

static union_type_t _unwrap_union_type(vm_t vm, value_t type_val) {
  if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE)
    return NULL;
  type_t inner = (type_t)value_get_data(type_val);
  if (type_get_kind(inner) != TYPE_KIND_UNION)
    return NULL;
  return (union_type_t)inner;
}

value_t vm_union_add_field(vm_t vm, value_t type_val,
                            const char *name, value_t field_type_val, bool pub) {
  union_type_t ut = _unwrap_union_type(vm, type_val);
  if (!ut)
    return create_exception_value(vm, "vm_union_add_field: expected union type value");
  type_t inner = (type_t)value_get_data(type_val);
  if (ut->sealed)
    return create_exception_value(vm, "cannot add field '%s' to sealed union type '%s'",
                                  name, type_get_name(inner));
  if (_ut_find_field(ut, name))
    return create_exception_value(vm, "duplicate field '%s' in union type '%s'",
                                  name, type_get_name(inner));
  type_t field_type = (type_t)value_get_data(field_type_val);
  _ut_add_field(vm_get_allocator(vm), ut, name, field_type, pub);
  return create_void_value(vm);
}

value_t vm_union_seal(vm_t vm, value_t type_val) {
  union_type_t ut = _unwrap_union_type(vm, type_val);
  if (!ut)
    return create_exception_value(vm, "vm_union_seal: expected union type value");
  if (!_ut_seal(ut))
    return create_exception_value(vm, "cannot seal empty union type '%s'",
                                  type_get_name((type_t)ut));
  return create_void_value(vm);
}

value_t vm_union_add_prop(vm_t vm, value_t type_val,
                           const char *name, value_t val, bool is_method, bool pub) {
  union_type_t ut = _unwrap_union_type(vm, type_val);
  if (!ut)
    return create_exception_value(vm, "vm_union_add_prop: expected union type value");
  if (strmap_find(ut->props, name))
    return create_exception_value(vm, "duplicate prop '%s' in union type '%s'",
                                  name, type_get_name((type_t)ut));
  _ut_add_prop(vm, ut, name, val, is_method, pub);
  return create_void_value(vm);
}

field_info_t vm_union_find_field(vm_t vm, value_t type_val, const char *name) {
  union_type_t ut = _unwrap_union_type(vm, type_val);
  (void)vm;
  if (!ut) return NULL;
  return _ut_find_field(ut, name);
}

vec_t vm_union_get_fields(vm_t vm, value_t type_val) {
  union_type_t ut = _unwrap_union_type(vm, type_val);
  (void)vm;
  if (!ut) return NULL;
  return _ut_get_fields(ut);
}

scope_t vm_union_get_scope(vm_t vm, value_t type_val) {
  union_type_t ut = _unwrap_union_type(vm, type_val);
  (void)vm;
  if (!ut) return NULL;
  return _ut_get_scope(ut);
}

strmap_t vm_union_get_props(vm_t vm, value_t type_val) {
  union_type_t ut = _unwrap_union_type(vm, type_val);
  (void)vm;
  if (!ut) return NULL;
  return _ut_get_props(ut);
}

strmap_t vm_union_get_methods(vm_t vm, value_t type_val) {
  union_type_t ut = _unwrap_union_type(vm, type_val);
  (void)vm;
  if (!ut) return NULL;
  return _ut_get_methods(ut);
}

bool vm_union_is_sealed(vm_t vm, value_t type_val) {
  union_type_t ut = _unwrap_union_type(vm, type_val);
  (void)vm;
  if (!ut) return false;
  return _ut_is_sealed(ut);
}

const char *vm_union_get_module_id(vm_t vm, value_t type_val) {
  union_type_t ut = _unwrap_union_type(vm, type_val);
  (void)vm;
  if (!ut) return NULL;
  return _ut_get_module_id(ut);
}

bool vm_union_is_field_pub(vm_t vm, value_t type_val, const char *name) {
  union_type_t ut = _unwrap_union_type(vm, type_val);
  (void)vm;
  if (!ut) return false;
  return _ut_is_field_pub(ut, name);
}

bool vm_union_is_prop_pub(vm_t vm, value_t type_val, const char *name) {
  union_type_t ut = _unwrap_union_type(vm, type_val);
  (void)vm;
  if (!ut) return false;
  return _ut_is_prop_pub(ut, name);
}

/* ---- Internal helpers for result_type.c (operate on union_type_t directly) ---- */

field_info_t _union_type_find_field(union_type_t ut, const char *name) {
  return _ut_find_field(ut, name);
}

value_t _union_type_create_value(vm_t vm, union_type_t ut,
                                  uint32_t tag, value_t field_value) {
  return _create_union_value(vm, ut, tag, field_value);
}
