#ifndef _H_CUBEC_ENGINE_VM_
#define _H_CUBEC_ENGINE_VM_
#include "core/allocator.h"
#include "core/class.h"
#include "core/strmap.h"
#include "engine/name.h"
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

allocator_t vm_get_allocator(vm_t self);
strmap_t    vm_get_modules(vm_t self);
scope_t  vm_get_global_scope(vm_t self);
scope_t  vm_get_root_scope(vm_t self);
scope_t  vm_get_current_scope(vm_t self);
value_t  vm_get_type_type(vm_t self);
value_t  vm_get_error_type(vm_t self);
value_t  vm_get_bool_type(vm_t self);
value_t  vm_get_wildcard_type(vm_t self);
value_t  vm_get_void_type(vm_t self);
value_t  vm_get_const_bool_type(vm_t self);
value_t  vm_get_i8_type(vm_t self);
value_t  vm_get_i16_type(vm_t self);
value_t  vm_get_i32_type(vm_t self);
value_t  vm_get_i64_type(vm_t self);
value_t  vm_get_const_i8_type(vm_t self);
value_t  vm_get_const_i16_type(vm_t self);
value_t  vm_get_const_i32_type(vm_t self);
value_t  vm_get_const_i64_type(vm_t self);
value_t  vm_get_u8_type(vm_t self);
value_t  vm_get_u16_type(vm_t self);
value_t  vm_get_u32_type(vm_t self);
value_t  vm_get_u64_type(vm_t self);
value_t  vm_get_const_u8_type(vm_t self);
value_t  vm_get_const_u16_type(vm_t self);
value_t  vm_get_const_u32_type(vm_t self);
value_t  vm_get_const_u64_type(vm_t self);
value_t  vm_get_f16_type(vm_t self);
value_t  vm_get_f32_type(vm_t self);
value_t  vm_get_f64_type(vm_t self);
value_t  vm_get_const_f16_type(vm_t self);
value_t  vm_get_const_f32_type(vm_t self);
value_t  vm_get_const_f64_type(vm_t self);
value_t  vm_get_str_type(vm_t self);
value_t  vm_get_const_str_type(vm_t self);

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

/* ---- Scope management ---- */

/** @brief Push a child scope of current_scope. For block/for/while etc.
 *  Sets root_scope and current_scope to the new child scope. */
void vm_push_scope(vm_t self, scope_t scope);

/** @brief Pop back to current_scope->parent. */
void vm_pop_scope(vm_t self);

/** @brief Set current_scope, return previous current_scope.
 *  For function call / closure: switch to an independent scope tree. */
scope_t vm_set_scope(vm_t self, scope_t scope);

/** @brief Set root_scope, return previous root_scope.
 *  Used together with vm_set_scope for function call / closure. */
scope_t vm_set_root_scope(vm_t self, scope_t scope);

/* ---- Value creation ---- */

/** @brief Create a value with owned copy of data, add to current_scope->values.
 *  If data is NULL, allocates zeroed buffer of type_get_size(type).
 *  If name is non-NULL, creates a name entry in current_scope. */
value_t vm_create_value(vm_t self, type_t type, const void *data,
                        const char *name);

/** @brief Create a shadow value (data=NULL, own=false), add to current_scope->values.
 *  initialized=true for compile-time placeholder, false for TDZ (undefined).
 *  If name is non-NULL, creates a name entry in current_scope. */
value_t vm_create_value_shadow(vm_t self, type_t type,
                               const char *name, bool initialized);

#ifdef __cplusplus
}
#endif
#endif
