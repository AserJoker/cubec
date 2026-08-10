#ifndef _H_CUBEC_ENGINE_VM_
#define _H_CUBEC_ENGINE_VM_
#include "core/allocator.h"
#include "core/class.h"
#include "core/slotmap.h"
#include "core/strmap.h"
#include "engine/value.h"
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

/* ---- Value creation ---- */

/**
 * @brief Create a value with owned copy of data, register in slot_map.
 *
 * @param self  VM
 * @param type  Type object (must not be NULL)
 * @param data  Source data to copy (may be NULL for shadow values)
 * @return New value_t pointer with addr set to its slot handle
 */
value_t vm_create_value(vm_t self, type_t type, const void *data);

/**
 * @brief Create a reference value with borrowed data, register in slot_map.
 *
 * The new value does NOT own data (own=false). Useful for variable
 * references, field references, etc.
 *
 * @param self  VM
 * @param type  Type object (must not be NULL)
 * @param data  Borrowed data pointer (may be NULL)
 * @return New value_t (own=false)
 */
value_t vm_create_value_ref(vm_t self, type_t type, void *data);

#ifdef __cplusplus
}
#endif
#endif
