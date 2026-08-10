#include "engine/vm.h"
#include "engine/scope.h"
#include "core/strmap.h"
#include <string.h>

struct _vm_t {
  allocator_t allocator;
  slotmap_t  *slots;
  strmap_t    modules;      /* absolute path → module_t (auto-dispose) */
  scope_t     global_scope; /* owned: global scope */
};

static void _vm_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  vm_t vm = (vm_t)self;
  vm->allocator = allocator;
  vm->slots = slotmap_create(allocator);

  strmap_init_t sm_init = {.value_auto_dispose = true};
  vm->modules = (strmap_t)allocator_create(allocator, &g_strmap_class, &sm_init);

  vm->global_scope = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
}

static void _vm_dispose(void *self, allocator_t allocator) {
  vm_t vm = (vm_t)self;
  (void)allocator;
  if (vm->slots) {
    slotmap_dispose(vm->slots);
    vm->slots = NULL;
  }
  allocator_free(vm->allocator, &vm->modules);
  allocator_free(vm->allocator, &vm->global_scope);
}

static void _vm_clone(void *self, allocator_t allocator, void *another) {
  (void)another;
  vm_t dst = (vm_t)self;
  dst->allocator = allocator;
  dst->slots = slotmap_create(allocator);
  dst->modules = NULL;
  dst->global_scope = NULL;
}

static void _vm_move(void *self, allocator_t allocator, void *another) {
  (void)allocator;
  vm_t dst = (vm_t)self;
  vm_t src = (vm_t)another;
  dst->allocator = src->allocator;
  dst->slots = src->slots;
  dst->modules = src->modules;
  dst->global_scope = src->global_scope;
  src->slots = NULL;
  src->modules = NULL;
  src->global_scope = NULL;
}

class_t g_vm_class = {
    .size = sizeof(struct _vm_t),
    .name = "cubec.engine.vm",
    .init = (class_init_fn_t)_vm_init,
    .dispose = (class_dispose_fn_t)_vm_dispose,
    .clone = (class_clone_fn_t)_vm_clone,
    .move = (class_move_fn_t)_vm_move,
};

vm_t vm_create(allocator_t allocator) {
  return (vm_t)allocator_create(allocator, &g_vm_class, NULL);
}

void vm_dispose(vm_t self, allocator_t allocator) {
  if (!self) return;
  allocator_free(allocator, &self);
}

slotmap_t *vm_get_slots(vm_t self) { return self->slots; }
strmap_t   vm_get_modules(vm_t self) { return self->modules; }
scope_t    vm_get_global_scope(vm_t self) { return self->global_scope; }

module_t vm_get_module(vm_t self, const char *abs_path) {
  return (module_t)strmap_find(self->modules, abs_path);
}
