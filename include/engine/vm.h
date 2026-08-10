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

struct _vm_t;
typedef struct _vm_t *vm_t;

/** @brief Type descriptor for allocator_create. */
extern class_t g_vm_class;

/* ---- Lifecycle ---- */

vm_t vm_create(allocator_t allocator);
void vm_dispose(vm_t self, allocator_t allocator);

/* ---- Accessors ---- */

strmap_t vm_get_modules(vm_t self);
scope_t  vm_get_global_scope(vm_t self);
scope_t  vm_get_root_scope(vm_t self);
scope_t  vm_get_current_scope(vm_t self);

/* ---- Module registry ---- */

module_t vm_get_module(vm_t self, const char *abs_path);

/**
 * @brief Import a module by its import path.
 *
 * Pipeline: resolve path → read file → tokenize → parse AST → create module.
 * Results are cached; repeated imports return the existing module.
 */
struct context;
module_t vm_import(vm_t self, struct context *ctx, const char *import_path);

/* ---- Scope stack ---- */

void vm_push_scope(vm_t self, scope_t scope);
void vm_pop_scope(vm_t self);

/* ---- Value creation ---- */

/** @brief Create a value with owned copy of data, add to current_scope->values. */
value_t vm_create_value(vm_t self, type_t type, const void *data);

/** @brief Create a reference value with borrowed data (own=false), add to current_scope->values. */
value_t vm_create_value_ref(vm_t self, type_t type, void *data);

#ifdef __cplusplus
}
#endif
#endif
