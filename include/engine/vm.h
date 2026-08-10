#ifndef _H_CUBEC_ENGINE_VM_
#define _H_CUBEC_ENGINE_VM_
#include "core/allocator.h"
#include "core/class.h"
#include "core/slotmap.h"
#include "core/strmap.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _scope_t;
typedef struct _scope_t *scope_t;

struct _module_t;
typedef struct _module_t *module_t;

/**
 * @brief VM — central execution context.
 *
 * Holds runtime state: virtual pointer table, module registry, global scope.
 * VM is NOT a value container — slot_map is accessed directly
 * for pointer indirection, not through vm wrapper APIs.
 */
struct _vm_t;
typedef struct _vm_t *vm_t;

/** @brief Type descriptor for allocator_create. */
extern class_t g_vm_class;

/**
 * @brief Create a VM instance.
 */
vm_t vm_create(allocator_t allocator);

/**
 * @brief Dispose a VM. Frees slot_map, modules, and global_scope.
 */
void vm_dispose(vm_t self, allocator_t allocator);

/* ---- Accessors ---- */

slotmap_t *vm_get_slots(vm_t self);
strmap_t   vm_get_modules(vm_t self);
scope_t    vm_get_global_scope(vm_t self);

/* ---- Module registry ---- */

module_t vm_get_module(vm_t self, const char *abs_path);

#ifdef __cplusplus
}
#endif
#endif
