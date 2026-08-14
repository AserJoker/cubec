#include "engine/struct_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/exception_type.h"
#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/str_type.h"
#include "engine/pointer_type.h"
#include "engine/callable_type.h"
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

/* ---- Forward declarations for vtable functions ---- */

static value_t _struct_clone(vm_t vm, value_t self);
static value_t _struct_equal(vm_t vm, value_t a, value_t b);
static value_t _struct_type_equal(vm_t vm, type_t a, type_t b);
static value_t _struct_type_extends(vm_t vm, type_t sub, type_t super);
static value_t _struct_safe_cast(vm_t vm, value_t self, type_t to);
static value_t _struct_assignment(vm_t vm, value_t lvalue, value_t rvalue);
static value_t _struct_to_string(vm_t vm, value_t self);
static value_t _struct_get_field(vm_t vm, value_t self, const char *name);
static value_t _struct_set_field(vm_t vm, value_t self, const char *name, value_t val);
static value_t _struct_member_call(vm_t vm, value_t self, const char *name,
                                   size_t argc, value_t *argv);
static value_t _struct_type_get_prop(vm_t vm, type_t self, const char *name);
static value_t _struct_type_set_prop(vm_t vm, type_t self, const char *name, value_t val);

/* ---- Shared vtable for all struct types ---- */

static vtable_t _make_struct_vtable(void) {
  return (vtable_t){
      .clone        = _struct_clone,
      .equal        = _struct_equal,
      .extends      = NULL,
      .type_equal   = _struct_type_equal,
      .type_extends = _struct_type_extends,
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
      .safe_cast    = _struct_safe_cast,
      .assignment   = _struct_assignment,
      .to_string    = _struct_to_string,
      .get_field    = _struct_get_field,
      .set_field    = _struct_set_field,
      .get_item     = NULL,
      .set_item     = NULL,
      .deref_get    = NULL,
      .deref_set    = NULL,
      .slice        = NULL,
      .call         = NULL,
      .member_call  = _struct_member_call,
      .get_prop     = NULL,
      .set_prop     = NULL,
      .type_get_prop= _struct_type_get_prop,
      .type_set_prop= _struct_type_set_prop,
  };
}

/* ================================================================== */
/* field_info_t class                                                  */
/* ================================================================== */

struct _field_info_t {
  char    *name;     /* owned */
  type_t   type;     /* owned (alloc_clone) */
  uint64_t offset;   /* computed */
  bool     pub;      /* true = accessible across modules */
};

typedef struct field_info_init_t {
  const char *name;
  type_t      type;
  uint64_t    offset;
  bool        pub;
} field_info_init_t;

static void _field_info_init(void *self, allocator_t allocator, void *arg) {
  field_info_t fi = (field_info_t)self;
  field_info_init_t *init = (field_info_init_t *)arg;
  fi->name   = cstring_clone(allocator, init->name);
  fi->type   = (type_t)alloc_clone(allocator, init->type);
  fi->offset = init->offset;
  fi->pub    = init ? init->pub : false;
}

static void _field_info_dispose(void *self, allocator_t allocator) {
  field_info_t fi = (field_info_t)self;
  if (fi->name) {
    void *p = fi->name;
    allocator_free(allocator, &p);
    fi->name = NULL;
  }
  allocator_free(allocator, &fi->type);
  fi->type   = NULL;
  fi->offset = 0;
  fi->pub    = false;
}

static void _field_info_clone(void *self, allocator_t allocator, void *another) {
  field_info_t dst = (field_info_t)self;
  field_info_t src = (field_info_t)another;
  dst->name   = cstring_clone(allocator, src->name);
  dst->type   = (type_t)alloc_clone(allocator, src->type);
  dst->offset = src->offset;
  dst->pub    = src->pub;
}

class_t g_field_info_class = {
    .size    = sizeof(struct _field_info_t),
    .name    = "cubec.engine.field_info",
    .init    = (class_init_fn_t)_field_info_init,
    .dispose = (class_dispose_fn_t)_field_info_dispose,
    .clone   = (class_clone_fn_t)_field_info_clone,
    .move    = NULL,
};

const char *field_info_get_name(field_info_t self)   { return self->name; }
type_t      field_info_get_type(field_info_t self)   { return self->type; }
uint64_t    field_info_get_offset(field_info_t self)  { return self->offset; }
bool        field_info_is_pub(field_info_t self)      { return self->pub; }

/* ================================================================== */
/* struct_type_t class                                                 */
/* ================================================================== */

static void _struct_type_init(void *self, allocator_t allocator, void *arg) {
  struct_type_t st = (struct_type_t)self;
  struct_type_init_t *init = (struct_type_init_t *)arg;

  st->base.kind    = init->kind;
  st->base.name    = init->name ? cstring_clone(allocator, init->name) : NULL;
  st->base.size    = init->size;
  st->base.align   = init->align;
  st->base.mut     = init->mut;
  st->base.vtable  = init->vtable;

  vec_init_t vi = {.auto_dispose = true};
  st->fields  = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  /* Isolated scope — struct_type_t fully owns and disposes it.
   * No parent linkage: prevents double-free from vm_dispose → scope_dispose(parent). */
  st->scope   = scope_create(allocator, SCOPE_TYPE, NULL, NULL);

  strmap_init_t smi = {.value_auto_dispose = false};
  st->props   = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  st->methods = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  st->pub_names = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  st->sealed  = false;
  st->module_id = init->module_id;
}

static void _struct_type_dispose(void *self, allocator_t allocator) {
  struct_type_t st = (struct_type_t)self;

  /* dispose props/methods/pub_names strmaps (borrowed values, just free map) */
  allocator_free(allocator, &st->props);
  allocator_free(allocator, &st->methods);
  allocator_free(allocator, &st->pub_names);

  /* dispose owned scope (which owns the prop/method values) */
  if (st->scope) {
    scope_dispose(st->scope);
    st->scope = NULL;
  }

  /* dispose fields vec (auto_dispose=true, owns field_info_t*) */
  allocator_free(allocator, &st->fields);

  if (st->base.name) {
    void *p = st->base.name;
    allocator_free(allocator, &p);
    st->base.name = NULL;
  }

  st->module_id = NULL;
}

static value_t _shallow_clone_value(allocator_t allocator, value_t src);

static value_t _shallow_clone_value(allocator_t allocator, value_t src) {
  /* shallow-clone a value_t using public accessors (opaque pointer) */
  extern class_t g_value_class;

  type_t  src_type = value_get_type(src);
  void   *src_data = value_get_data(src);
  bool    src_own   = value_is_own(src);
  bool    src_init  = value_is_initialized(src);

  /* clone the data if owned */
  void *new_data = src_data;
  if (src_own && src_data) {
    uint64_t sz = type_get_size(src_type);
    if (sz > 0) {
      new_data = allocator_alloc(allocator, sz);
      memcpy(new_data, src_data, (size_t)sz);
    }
  }

  /* use value_init_t to match the actual value internal layout */
  typedef struct { type_t type; void *data; void *meta; bool own; bool initialized; } value_init_repr;
  value_init_repr init = { .type = src_type, .data = new_data, .meta = NULL, .own = src_own, .initialized = src_init };
  value_t dst = (value_t)allocator_create(allocator, &g_value_class, &init);
  return dst;
}

static void _struct_type_clone(void *self, allocator_t allocator, void *another) {
  struct_type_t dst = (struct_type_t)self;
  struct_type_t src = (struct_type_t)another;

  dst->base.kind    = src->base.kind;
  dst->base.name    = src->base.name ? cstring_clone(allocator, src->base.name) : NULL;
  dst->base.size    = src->base.size;
  dst->base.align   = src->base.align;
  dst->base.mut     = src->base.mut;
  dst->base.vtable  = src->base.vtable;

  /* clone fields */
  vec_init_t vi = {.auto_dispose = true};
  dst->fields = (vec_t)allocator_create(allocator, &g_vec_class, &vi);
  size_t fc = vec_get_size(src->fields);
  for (size_t i = 0; i < fc; i++) {
    field_info_t fi = (field_info_t)vec_get(src->fields, i);
    field_info_init_t fiinit = {
        .name   = fi->name,
        .type   = fi->type,
        .offset = fi->offset,
        .pub    = fi->pub,
    };
    field_info_t cloned = (field_info_t)allocator_create(allocator,
                                                          &g_field_info_class, &fiinit);
    vec_push(dst->fields, cloned);
  }

  /* clone scope — isolated, no parent */
  dst->scope = scope_create(allocator, SCOPE_TYPE, NULL, NULL);

  /* clone props: shallow-clone values, register in dst->scope, rebuild strmap */
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

  /* clone methods similarly */
  dst->methods = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  it = strmap_iter_first(src->methods);
  while ((key = strmap_iter_next(&it)) != NULL) {
    value_t sv = (value_t)strmap_find(src->methods, key);
    value_t cv = _shallow_clone_value(allocator, sv);
    vec_push(dst->scope->values, cv);
    strmap_insert(dst->methods, key, cv);
  }

  /* clone pub_names: copy entries with dummy value */
  dst->pub_names = (strmap_t)allocator_create(allocator, &g_strmap_class, &smi);
  it = strmap_iter_first(src->pub_names);
  while ((key = strmap_iter_next(&it)) != NULL) {
    strmap_insert(dst->pub_names, key, (void *)1);
  }

  dst->sealed = src->sealed;
  dst->module_id = src->module_id;
}

class_t g_struct_type_class = {
    .size    = sizeof(struct _struct_type_t),
    .name    = "cubec.engine.struct_type",
    .init    = (class_init_fn_t)_struct_type_init,
    .dispose = (class_dispose_fn_t)_struct_type_dispose,
    .clone   = (class_clone_fn_t)_struct_type_clone,
    .move    = NULL,
};

/* ================================================================== */
/* Type creation                                                       */
/* ================================================================== */

/** Align offset up to alignment boundary. */
static uint64_t _align_up(uint64_t offset, uint64_t align) {
  return (offset + align - 1) & ~(align - 1);
}

struct_type_t struct_type_create(allocator_t allocator, const char *name,
                                  bool mut, const char *module_id) {
  struct_type_init_t init = {
      .kind         = TYPE_KIND_STRUCT,
      .name         = name,
      .size         = 0,  /* computed during add_field / seal */
      .align        = 1,
      .mut          = mut,
      .vtable       = _make_struct_vtable(),
      .module_id    = module_id,
  };

  struct_type_t st = (struct_type_t)allocator_create(
      allocator, &g_struct_type_class, &init);
  return st;
}

void struct_type_add_field(allocator_t allocator, struct_type_t st,
                           const char *name, type_t field_type, bool pub) {
  if (st->sealed) {
    fprintf(stderr, "error: cannot add field '%s' to sealed struct type '%s'\n",
            name, st->base.name ? st->base.name : "<anonymous>");
    return;
  }

  /* compute offset using C alignment rules */
  uint64_t field_align = type_get_align(field_type);
  uint64_t field_size  = type_get_size(field_type);

  /* update struct alignment to max of all fields */
  if (field_align > st->base.align)
    st->base.align = field_align;

  /* current_offset = sum of previous fields (stored in base.size so far) */
  uint64_t offset = _align_up(st->base.size, field_align);

  field_info_init_t fiinit = {
      .name   = name,
      .type   = field_type,
      .offset = offset,
      .pub    = pub,
  };
  field_info_t fi = (field_info_t)allocator_create(allocator,
                                                     &g_field_info_class, &fiinit);
  vec_push(st->fields, fi);

  /* if pub, insert name into pub_names */
  if (pub)
    strmap_insert(st->pub_names, name, (void *)1);

  /* advance size past this field */
  st->base.size = offset + field_size;
}

bool struct_type_seal(struct_type_t st) {
  if (st->sealed) return true;

  /* final size = align_up(current_size, struct_align) for trailing padding */
  st->base.size = _align_up(st->base.size, st->base.align);
  /* C guarantee: even empty struct has size >= 1 */
  if (st->base.size == 0)
    st->base.size = 1;
  st->sealed = true;
  return true;
}

void struct_type_add_prop(vm_t vm, struct_type_t st,
                          const char *name, value_t val, bool is_method, bool pub) {
  /* register in props table (borrowed — value lifecycle managed by vm's scope) */
  strmap_insert(st->props, name, val);

  /* if method, also register in methods table */
  if (is_method)
    strmap_insert(st->methods, name, val);

  /* if pub, insert name into pub_names */
  if (pub)
    strmap_insert(st->pub_names, name, (void *)1);

  (void)vm;
}

/* ================================================================== */
/* Accessors                                                           */
/* ================================================================== */

vec_t    struct_type_get_fields(struct_type_t self)  { return self->fields; }
scope_t  struct_type_get_scope(struct_type_t self)   { return self->scope; }
strmap_t struct_type_get_props(struct_type_t self)    { return self->props; }
strmap_t struct_type_get_methods(struct_type_t self)  { return self->methods; }
bool     struct_type_is_sealed(struct_type_t self)    { return self->sealed; }

const char *struct_type_get_module_id(struct_type_t self) { return self->module_id; }

bool struct_type_is_field_pub(struct_type_t self, const char *name) {
  field_info_t fi = struct_type_find_field(self, name);
  return fi ? field_info_is_pub(fi) : false;
}

bool struct_type_is_prop_pub(struct_type_t self, const char *name) {
  return strmap_find(self->pub_names, name) != NULL;
}

field_info_t struct_type_find_field(struct_type_t self, const char *name) {
  size_t fc = vec_get_size(self->fields);
  for (size_t i = 0; i < fc; i++) {
    field_info_t fi = (field_info_t)vec_get(self->fields, i);
    if (strcmp(fi->name, name) == 0)
      return fi;
  }
  return NULL;
}

/* ================================================================== */
/* Value constructors                                                  */
/* ================================================================== */

value_t create_struct_value(vm_t vm, struct_type_t st, value_t *field_values) {
  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = st->base.size;

  void *data = NULL;
  if (total_size > 0) {
    data = allocator_alloc(alloc, total_size);
    memset(data, 0, (size_t)total_size);

    /* memcpy each field value into its offset */
    size_t fc = vec_get_size(st->fields);
    for (size_t i = 0; i < fc; i++) {
      field_info_t fi = (field_info_t)vec_get(st->fields, i);
      if (field_values && field_values[i] && value_get_data(field_values[i])) {
        uint64_t fsize = type_get_size(fi->type);
        if (fsize > 0)
          memcpy((char *)data + fi->offset,
                 value_get_data(field_values[i]), (size_t)fsize);
      }
    }
  }

  value_t v = value_create(alloc, (type_t)st, data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

value_t create_struct_shadow(vm_t vm, struct_type_t st, bool initialized) {
  value_t v = value_create(vm_get_allocator(vm), (type_t)st, NULL, false);
  value_set_initialized(v, initialized);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

value_t _struct_value_member_addr(vm_t vm, value_t self, const char *name) {
  struct_type_t st = (struct_type_t)value_get_type(self);
  field_info_t fi = struct_type_find_field(st, name);
  if (!fi)
    return create_exception_value(vm, "struct '%s' has no field '%s'",
                              type_get_name((type_t)st), name);

  /* access control: private field only accessible from same module */
  if (_access_denied(struct_type_get_module_id(st), field_info_is_pub(fi),
                     vm_get_current_module_id(vm)))
    return create_exception_value(vm, "cannot access private field '%s' of struct '%s' from outside its module",
                              name, type_get_name((type_t)st));

  if (value_is_shadow(self) || !value_get_data(self))
    return create_exception_value(vm, "cannot take address of field in uninitialized struct");

  /* create pointer type: *FieldType */
  allocator_t alloc = vm_get_allocator(vm);
  pointer_type_t pt = pointer_type_create(alloc, fi->type, true, false);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->types, pt);

  /* pointer value data = address of field within struct buffer */
  void *field_addr = (char *)value_get_data(self) + fi->offset;
  return create_pointer_value_from_addr(vm, pt, field_addr);
}

/* ================================================================== */
/* VM convenience                                                      */
/* ================================================================== */

value_t vm_create_struct_type_value(vm_t vm, const char *name,
                                     bool mut, const char *module_id) {
  struct_type_t st = struct_type_create(vm_get_allocator(vm), name, mut, module_id);
  if (vm_get_current_scope(vm))
    vec_push(vm_get_current_scope(vm)->types, st);
  return create_type_value(vm, (type_t)st, NULL, false);
}

/* ================================================================== */
/* VTable: clone                                                       */
/* ================================================================== */

static value_t _struct_clone(vm_t vm, value_t self) {
  struct_type_t st = (struct_type_t)value_get_type(self);
  if (value_is_shadow(self))
    return create_struct_shadow(vm, st, value_is_initialized(self));

  type_t cloned_type = value_type_clone(vm, (type_t)st);

  /* memcpy the entire buffer */
  allocator_t alloc = vm_get_allocator(vm);
  uint64_t total_size = st->base.size;
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

static value_t _struct_equal(vm_t vm, value_t a, value_t b) {
  type_t tb = value_get_type(b);
  if (type_get_kind(tb) != TYPE_KIND_STRUCT)
    return create_bool_value(vm, false);
  if (value_is_shadow(a) || value_is_shadow(b))
    return vm_create_value_shadow(vm, value_get_type(a), NULL, true);

  struct_type_t sta = (struct_type_t)value_get_type(a);

  /* field-by-field equal */
  size_t fc = vec_get_size(sta->fields);
  for (size_t i = 0; i < fc; i++) {
    field_info_t fi = (field_info_t)vec_get(sta->fields, i);

    /* extract temporary field values (borrowed, own=false) */
    allocator_t alloc = vm_get_allocator(vm);
    value_t fa = value_create(alloc, fi->type,
                              (char *)value_get_data(a) + fi->offset, false);
    value_t fb = value_create(alloc, fi->type,
                              (char *)value_get_data(b) + fi->offset, false);

    value_t eq = value_equal(vm, fa, fb);

    /* clean up temps — they are borrowed (own=false), just free the value_t shell */
    allocator_free(alloc, &fa);
    allocator_free(alloc, &fb);

    if (type_get_kind(value_get_type(eq)) == TYPE_KIND_EXCEPTION)
      return eq;
    if (value_is_shadow(eq))
      return vm_create_value_shadow(vm, value_get_type(a), NULL, true);
    if (!(*(bool *)value_get_data(eq)))
      return create_bool_value(vm, false);
  }
  return create_bool_value(vm, true);
}

/* ================================================================== */
/* VTable: type_equal (duck typing)                                    */
/* ================================================================== */

static value_t _struct_type_equal(vm_t vm, type_t a, type_t b) {
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (b->kind != TYPE_KIND_STRUCT)
    return create_bool_value(vm, false);

  struct_type_t sa = (struct_type_t)a;
  struct_type_t sb = (struct_type_t)b;

  /* mut must match */
  if (a->mut != b->mut)
    return create_bool_value(vm, false);

  /* field count must match */
  size_t fa_count = vec_get_size(sa->fields);
  size_t fb_count = vec_get_size(sb->fields);
  if (fa_count != fb_count)
    return create_bool_value(vm, false);

  /* field-by-field type_equal */
  for (size_t i = 0; i < fa_count; i++) {
    field_info_t fia = (field_info_t)vec_get(sa->fields, i);
    field_info_t fib = (field_info_t)vec_get(sb->fields, i);
    vtable_t evt = type_get_vtable(fia->type);
    value_t eq;
    if (evt.type_equal)
      eq = evt.type_equal(vm, fia->type, fib->type);
    else
      eq = create_bool_value(vm, type_get_kind(fia->type) == type_get_kind(fib->type));
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
/* VTable: type_extends                                                */
/* ================================================================== */

static value_t _struct_type_extends(vm_t vm, type_t sub, type_t super) {
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  if (super->kind != TYPE_KIND_STRUCT)
    return create_bool_value(vm, false);

  struct_type_t ssub = (struct_type_t)sub;
  struct_type_t ssup = (struct_type_t)super;

  /* mut must match */
  if (sub->mut != super->mut)
    return create_bool_value(vm, false);

  /* super must have <= fields than sub (subset) */
  size_t sub_count = vec_get_size(ssub->fields);
  size_t sup_count = vec_get_size(ssup->fields);
  if (sup_count > sub_count)
    return create_bool_value(vm, false);

  /* each super field must type_extend from the corresponding sub field */
  for (size_t i = 0; i < sup_count; i++) {
    field_info_t fi_sub = (field_info_t)vec_get(ssub->fields, i);
    field_info_t fi_sup = (field_info_t)vec_get(ssup->fields, i);
    vtable_t evt = type_get_vtable(fi_sub->type);
    value_t ext;
    if (evt.type_extends)
      ext = evt.type_extends(vm, fi_sub->type, fi_sup->type);
    else if (type_get_kind(fi_sup->type) == TYPE_KIND_WILDCARD)
      ext = create_bool_value(vm, true);
    else
      ext = create_bool_value(vm, type_get_kind(fi_sub->type) == type_get_kind(fi_sup->type));
    if (type_get_kind(value_get_type(ext)) == TYPE_KIND_EXCEPTION)
      return ext;
    if (value_is_shadow(ext))
      return vm_create_value_shadow(vm, sub, NULL, true);
    if (!(*(bool *)value_get_data(ext)))
      return create_bool_value(vm, false);
  }
  return create_bool_value(vm, true);
}

/* ================================================================== */
/* VTable: safe_cast                                                   */
/* ================================================================== */

static value_t _struct_safe_cast(vm_t vm, value_t self, type_t to) {
  type_t from = value_get_type(self);
  /* strict type_equal required */
  value_t eq = _struct_type_equal(vm, from, to);
  if (type_get_kind(value_get_type(eq)) == TYPE_KIND_EXCEPTION)
    return eq;
  if (value_is_shadow(eq) || !(*(bool *)value_get_data(eq)))
    return create_exception_value(vm, "cannot safe_cast '%s' to '%s'",
                              type_get_name(from), type_get_name(to));
  if (value_is_shadow(self))
    return create_struct_shadow(vm, (struct_type_t)to, value_is_initialized(self));
  return self;
}

/* ================================================================== */
/* VTable: assignment                                                  */
/* ================================================================== */

static value_t _struct_assignment(vm_t vm, value_t lvalue, value_t rvalue) {
  type_t lt = value_get_type(lvalue);
  /* const check */
  if (value_is_initialized(lvalue) && !lt->mut)
    return create_exception_value(vm, "cannot assign to const '%s'", lt->name);

  /* shadow: just mark initialized */
  if (value_is_shadow(lvalue) || value_is_shadow(rvalue)) {
    value_set_initialized(lvalue, true);
    return create_void_value(vm);
  }

  /* type_equal check */
  value_t eq = _struct_type_equal(vm, lt, value_get_type(rvalue));
  if (type_get_kind(value_get_type(eq)) == TYPE_KIND_EXCEPTION)
    return eq;
  if (!(*(bool *)value_get_data(eq)))
    return create_exception_value(vm, "cannot assign '%s' to '%s'",
                              type_get_name(value_get_type(rvalue)),
                              type_get_name(lt));

  /* memcpy entire buffer */
  uint64_t size = type_get_size(lt);
  if (size > 0 && value_get_data(rvalue))
    memcpy(value_get_data(lvalue), value_get_data(rvalue), (size_t)size);

  value_set_initialized(lvalue, true);
  return create_void_value(vm);
}

/* ================================================================== */
/* VTable: to_string                                                   */
/* ================================================================== */

static value_t _struct_to_string(vm_t vm, value_t self) {
  if (value_is_shadow(self))
    return vm_create_value_shadow(vm, (type_t)value_get_data(vm_get_str_type(vm)), NULL, true);
  struct_type_t st = (struct_type_t)value_get_type(self);
  size_t fc = vec_get_size(st->fields);
  allocator_t alloc = vm_get_allocator(vm);

  /* build using string_t for dynamic growth */
  string_t result = (string_t)allocator_create(alloc, &g_string_class, NULL);

  /* type name */
  if (st->base.name)
    string_concat(result, st->base.name);

  string_concat(result, "{");

  for (size_t i = 0; i < fc; i++) {
    field_info_t fi = (field_info_t)vec_get(st->fields, i);
    if (i > 0)
      string_concat(result, ", ");

    /* field name */
    string_concat(result, fi->name);
    string_concat(result, ": ");

    /* field value */
    if (!value_is_shadow(self) && value_get_data(self)) {
      value_t fv = value_create(alloc, fi->type,
                                (char *)value_get_data(self) + fi->offset, false);
      value_t fvs = value_to_string(vm, fv);
      allocator_free(alloc, &fv);
      if (type_get_kind(value_get_type(fvs)) != TYPE_KIND_EXCEPTION &&
          type_get_kind(value_get_type(fvs)) == TYPE_KIND_STR) {
        string_t sdata = *(string_t *)value_get_data(fvs);
        const char *s = string_get(sdata);
        string_concat(result, s);
      }
    } else {
      string_concat(result, "<uninit>");
    }
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

static value_t _struct_get_field(vm_t vm, value_t self, const char *name) {
  struct_type_t st = (struct_type_t)value_get_type(self);
  field_info_t fi = struct_type_find_field(st, name);
  if (!fi)
    return create_exception_value(vm, "struct '%s' has no field '%s'",
                              type_get_name((type_t)st), name);

  /* access control: private field only accessible from same module */
  if (_access_denied(struct_type_get_module_id(st), field_info_is_pub(fi),
                     vm_get_current_module_id(vm)))
    return create_exception_value(vm, "cannot access private field '%s' of struct '%s' from outside its module",
                              name, type_get_name((type_t)st));

  if (value_is_shadow(self) || !value_get_data(self))
    return create_exception_value(vm, "cannot access field '%s' of uninitialized struct", name);

  /* memcpy field data out as temporary value */
  uint64_t fsize = type_get_size(fi->type);
  allocator_t alloc = vm_get_allocator(vm);
  void *data = NULL;
  if (fsize > 0) {
    data = allocator_alloc(alloc, fsize);
    memcpy(data, (char *)value_get_data(self) + fi->offset, (size_t)fsize);
  }

  value_t v = value_create(alloc, fi->type, data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

static value_t _struct_set_field(vm_t vm, value_t self, const char *name, value_t val) {
  struct_type_t st = (struct_type_t)value_get_type(self);
  field_info_t fi = struct_type_find_field(st, name);
  if (!fi)
    return create_exception_value(vm, "struct '%s' has no field '%s'",
                              type_get_name((type_t)st), name);

  /* access control: private field only accessible from same module */
  if (_access_denied(struct_type_get_module_id(st), field_info_is_pub(fi),
                     vm_get_current_module_id(vm)))
    return create_exception_value(vm, "cannot access private field '%s' of struct '%s' from outside its module",
                              name, type_get_name((type_t)st));

  /* const struct check */
  if (!type_is_mut((type_t)st))
    return create_exception_value(vm, "cannot assign to field of const struct");

  /* safe_cast val to field type */
  value_t casted = value_safe_cast(vm, val, fi->type);
  if (type_get_kind(value_get_type(casted)) == TYPE_KIND_EXCEPTION)
    return casted;

  if (!value_is_shadow(self) && value_get_data(self) && value_get_data(casted)) {
    uint64_t fsize = type_get_size(fi->type);
    if (fsize > 0)
      memcpy((char *)value_get_data(self) + fi->offset,
             value_get_data(casted), (size_t)fsize);
  } else {
    /* shadow assignment: just mark initialized */
    value_set_initialized(self, true);
  }

  return create_void_value(vm);
}

/* ================================================================== */
/* VTable: member_call                                                 */
/* ================================================================== */

static value_t _struct_member_call(vm_t vm, value_t self, const char *name,
                                   size_t argc, value_t *argv) {
  struct_type_t st = (struct_type_t)value_get_type(self);

  /* look up in methods table */
  value_t method = (value_t)strmap_find(st->methods, name);
  if (!method)
    return create_exception_value(vm, "struct '%s' has no method '%s'",
                              type_get_name((type_t)st), name);

  /* addrof(self) as first argument */
  value_t addr = value_addrof(vm, self);

  /* construct new argv: [addr, argv[0], argv[1], ...] */
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

static value_t _struct_type_get_prop(vm_t vm, type_t self, const char *name) {
  struct_type_t st = (struct_type_t)self;
  value_t val = (value_t)strmap_find(st->props, name);
  if (!val)
    return create_exception_value(vm, "struct '%s' has no static property '%s'",
                              type_get_name(self), name);

  /* access control: private prop only accessible from same module */
  if (_access_denied(struct_type_get_module_id(st), struct_type_is_prop_pub(st, name),
                     vm_get_current_module_id(vm)))
    return create_exception_value(vm, "cannot access private property '%s' of struct '%s' from outside its module",
                              name, type_get_name(self));

  return val;
}

static value_t _struct_type_set_prop(vm_t vm, type_t self, const char *name, value_t val) {
  struct_type_t st = (struct_type_t)self;
  value_t existing = (value_t)strmap_find(st->props, name);
  if (!existing)
    return create_exception_value(vm, "struct '%s' has no static property '%s'",
                              type_get_name(self), name);

  /* access control: private prop only accessible from same module */
  if (_access_denied(struct_type_get_module_id(st), struct_type_is_prop_pub(st, name),
                     vm_get_current_module_id(vm)))
    return create_exception_value(vm, "cannot access private property '%s' of struct '%s' from outside its module",
                              name, type_get_name(self));

  /* const check on existing prop */
  if (value_is_initialized(existing) && !type_is_mut(value_get_type(existing)))
    return create_exception_value(vm, "cannot assign to const static property '%s'", name);

  /* assign value */
  value_t result = value_assignment(vm, existing, val);
  if (type_get_kind(value_get_type(result)) == TYPE_KIND_EXCEPTION)
    return result;

  return create_void_value(vm);
}
