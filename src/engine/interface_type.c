#include "engine/interface_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/callable_type.h"
#include "engine/struct_type.h"
#include "engine/union_type.h"
#include "engine/bool_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/type.h"
#include "core/string.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* ---- Forward declarations for static helpers ---- */

static interface_type_t _it_create(allocator_t allocator, const char *name,
                                    bool mut, const char *module_id);
static void _it_add_method(allocator_t allocator, interface_type_t it,
                            const char *name, callable_type_t ct);
static bool _it_seal(interface_type_t it);
static strmap_t _it_get_methods(interface_type_t self);
static bool _it_is_sealed(interface_type_t self);
static callable_type_t _it_find_method(interface_type_t self, const char *name);
static const char *_it_get_module_id(interface_type_t self);

/* ---- Interface type class ---- */

static void _interface_type_init(void *self, allocator_t allocator, void *arg) {
  interface_type_t it = (interface_type_t)self;
  interface_type_init_t *init = (interface_type_init_t *)arg;

  it->base.kind  = init->kind;
  it->base.name  = init->name ? cstring_clone(allocator, init->name) : NULL;
  it->base.size  = 0;
  it->base.align = 0;
  it->base.mut   = init->mut;
  it->base.vtable = init->vtable;

  strmap_init_t si = {.value_auto_dispose = false};
  it->methods = (strmap_t)allocator_create(allocator, &g_strmap_class, &si);
  it->sealed   = false;
  it->module_id = init->module_id;
}

static void _interface_type_dispose(void *self, allocator_t allocator) {
  interface_type_t it = (interface_type_t)self;
  allocator_free(allocator, &it->methods);
  if (it->base.name) {
    void *p = it->base.name;
    allocator_free(allocator, &p);
    it->base.name = NULL;
  }
}

static void _interface_type_clone(void *self, allocator_t allocator, void *another) {
  interface_type_t dst = (interface_type_t)self;
  interface_type_t src = (interface_type_t)another;

  dst->base.kind   = src->base.kind;
  dst->base.name   = src->base.name ? cstring_clone(allocator, src->base.name) : NULL;
  dst->base.size   = src->base.size;
  dst->base.align  = src->base.align;
  dst->base.mut    = src->base.mut;
  dst->base.vtable = src->base.vtable;

  strmap_init_t si = {.value_auto_dispose = false};
  dst->methods = (strmap_t)allocator_create(allocator, &g_strmap_class, &si);
  /* deep-clone each method callable_type_t */
  strmap_iter_t iter = strmap_iter_first(src->methods);
  const char *key;
  while ((key = strmap_iter_next(&iter)) != NULL) {
    callable_type_t src_ct = (callable_type_t)strmap_find(src->methods, key);
    callable_type_t cloned_ct = (callable_type_t)alloc_clone(allocator, src_ct);
    strmap_insert(dst->methods, key, cloned_ct);
  }

  dst->sealed   = src->sealed;
  dst->module_id = src->module_id;
}

class_t g_interface_type_class = {
    .size    = sizeof(struct _interface_type_t),
    .name    = "cubec.engine.interface_type",
    .init    = (class_init_fn_t)_interface_type_init,
    .dispose = (class_dispose_fn_t)_interface_type_dispose,
    .clone   = (class_clone_fn_t)_interface_type_clone,
    .move    = NULL,
};

/* ---- Type creation ---- */

static interface_type_t _it_create(allocator_t allocator, const char *name,
                                    bool mut, const char *module_id) {
  interface_type_init_t init = {
      .kind      = TYPE_KIND_INTERFACE,
      .name      = name,
      .mut       = mut,
      .vtable    = (vtable_t){0}, /* no vtable — interface is compile-time only */
      .module_id = module_id,
  };
  return (interface_type_t)allocator_create(allocator, &g_interface_type_class, &init);
}

static void _it_add_method(allocator_t allocator, interface_type_t it,
                            const char *name, callable_type_t ct) {
  if (it->sealed) {
    fprintf(stderr, "error: cannot add method '%s' to sealed interface '%s'\n",
            name, type_get_name((type_t)it));
    return;
  }
  if (strmap_find(it->methods, name)) {
    fprintf(stderr, "error: duplicate method '%s' in interface '%s'\n",
            name, type_get_name((type_t)it));
    return;
  }
  callable_type_t cloned = (callable_type_t)alloc_clone(allocator, ct);
  strmap_insert(it->methods, name, cloned);
}

static bool _it_seal(interface_type_t it) {
  if (strmap_get_size(it->methods) == 0)
    return false;
  it->sealed = true;
  return true;
}

/* ---- Accessors ---- */

static strmap_t    _it_get_methods(interface_type_t self) { return self->methods; }
static bool        _it_is_sealed(interface_type_t self) { return self->sealed; }
static const char *_it_get_module_id(interface_type_t self) { return self->module_id; }

static callable_type_t _it_find_method(interface_type_t self, const char *name) {
  return (callable_type_t)strmap_find(self->methods, name);
}

/* ---- VM convenience ---- */

value_t vm_create_interface_type_value(vm_t vm, const char *name,
                                        bool mut, const char *module_id) {
  interface_type_t it = _it_create(vm_get_allocator(vm), name, mut, module_id);
  if (vm_get_current_scope(vm))
    vec_push(vm_get_current_scope(vm)->types, it);
  return create_type_value(vm, (type_t)it, NULL, false);
}

/* ---- Interface extends check (called by struct/union type_extends) ---- */

value_t _interface_type_check_extends(vm_t vm, interface_type_t it, strmap_t sub_methods) {
  strmap_iter_t iter = strmap_iter_first(it->methods);
  const char *key;
  while ((key = strmap_iter_next(&iter)) != NULL) {
    callable_type_t sup_ct = (callable_type_t)strmap_find(it->methods, key);
    value_t sub_val = (value_t)strmap_find(sub_methods, key);
    if (!sub_val)
      return create_bool_value(vm, false);
    /* sub_val is a callable value; its type is the callable_type_t */
    callable_type_t sub_ct = (callable_type_t)value_get_type(sub_val);
    vtable_t vt = type_get_vtable((type_t)sub_ct);
    value_t ext;
    if (vt.type_extends)
      ext = vt.type_extends(vm, (type_t)sub_ct, (type_t)sup_ct);
    else if (vt.type_equal)
      ext = vt.type_equal(vm, (type_t)sub_ct, (type_t)sup_ct);
    else
      ext = create_bool_value(vm, type_get_kind((type_t)sub_ct) == type_get_kind((type_t)sup_ct));
    if (type_get_kind(value_get_type(ext)) == TYPE_KIND_EXCEPTION)
      return ext;
    if (!value_is_shadow(ext) && !(*(bool *)value_get_data(ext)))
      return create_bool_value(vm, false);
  }
  return create_bool_value(vm, true);
}

/* ================================================================== */
/* Value-based public API wrappers                                     */
/* ================================================================== */

static interface_type_t _unwrap_interface_type(vm_t vm, value_t type_val) {
  if (type_get_kind(value_get_type(type_val)) != TYPE_KIND_TYPE)
    return NULL;
  type_t inner = (type_t)value_get_data(type_val);
  if (type_get_kind(inner) != TYPE_KIND_INTERFACE)
    return NULL;
  return (interface_type_t)inner;
}

value_t vm_interface_add_method(vm_t vm, value_t type_val,
                                 const char *name, value_t callable_type_val) {
  interface_type_t it = _unwrap_interface_type(vm, type_val);
  if (!it)
    return create_exception_value(vm, "vm_interface_add_method: expected interface type value");
  if (it->sealed)
    return create_exception_value(vm, "cannot add method '%s' to sealed interface '%s'",
                                  name, type_get_name((type_t)it));
  if (strmap_find(it->methods, name))
    return create_exception_value(vm, "duplicate method '%s' in interface '%s'",
                                  name, type_get_name((type_t)it));
  callable_type_t ct = (callable_type_t)value_get_data(callable_type_val);
  _it_add_method(vm_get_allocator(vm), it, name, ct);
  return create_void_value(vm);
}

value_t vm_interface_seal(vm_t vm, value_t type_val) {
  interface_type_t it = _unwrap_interface_type(vm, type_val);
  if (!it)
    return create_exception_value(vm, "vm_interface_seal: expected interface type value");
  if (!_it_seal(it))
    return create_exception_value(vm, "cannot seal empty interface '%s'",
                                  type_get_name((type_t)it));
  return create_void_value(vm);
}

strmap_t vm_interface_get_methods(vm_t vm, value_t type_val) {
  interface_type_t it = _unwrap_interface_type(vm, type_val);
  (void)vm;
  if (!it) return NULL;
  return _it_get_methods(it);
}

bool vm_interface_is_sealed(vm_t vm, value_t type_val) {
  interface_type_t it = _unwrap_interface_type(vm, type_val);
  (void)vm;
  if (!it) return false;
  return _it_is_sealed(it);
}

callable_type_t vm_interface_find_method(vm_t vm, value_t type_val, const char *name) {
  interface_type_t it = _unwrap_interface_type(vm, type_val);
  (void)vm;
  if (!it) return NULL;
  return _it_find_method(it, name);
}

const char *vm_interface_get_module_id(vm_t vm, value_t type_val) {
  interface_type_t it = _unwrap_interface_type(vm, type_val);
  (void)vm;
  if (!it) return NULL;
  return _it_get_module_id(it);
}

value_t vm_interface_check_extends(vm_t vm, value_t type_val, strmap_t sub_methods) {
  interface_type_t it = _unwrap_interface_type(vm, type_val);
  if (!it)
    return create_exception_value(vm, "vm_interface_check_extends: expected interface type value");
  return _interface_type_check_extends(vm, it, sub_methods);
}
