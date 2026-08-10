#ifndef _H_CUBEC_ENGINE_VM_
#define _H_CUBEC_ENGINE_VM_
#include "core/allocator.h"
#include "core/class.h"
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
 * Holds runtime state: module registry, global scope.
 * Dangling pointers are UB — no safety net for now.
 */
struct _vm_t;
typedef struct _vm_t *vm_t;

/** @brief Type descriptor for allocator_create. */
extern class_t g_vm_class;

vm_t vm_create(allocator_t allocator);
void vm_dispose(vm_t self, allocator_t allocator);

/* ---- Accessors ---- */

strmap_t vm_get_modules(vm_t self);
scope_t  vm_get_global_scope(vm_t self);

/* ---- Module registry ---- */

module_t vm_get_module(vm_t self, const char *abs_path);

/* ---- Value creation ---- */

/**
 * @brief Create a value with owned copy of data.
 */
value_t vm_create_value(vm_t self, type_t type, const void *data);

/**
 * @brief Create a reference value with borrowed data (own=false).
 */
value_t vm_create_value_ref(vm_t self, type_t type, void *data);

#ifdef __cplusplus
}
#endif
#endif
