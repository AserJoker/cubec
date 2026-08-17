#ifndef _H_CUBEC_ENGINE_VM_
#define _H_CUBEC_ENGINE_VM_
#include "core/allocator.h"
#include "core/class.h"
#include "core/strmap.h"
#include "core/vec.h"
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

/**
 * @brief Call frame — one entry in the VM call stack.
 * name: function/method name (owned, cloned on init)
 * message: optional context info, displayed as <name> (<message>) (owned,
 * cloned on init) Managed as a class object via g_call_frame_class, owned by
 * vm->call_stack (auto_dispose).
 */
struct _call_frame_t {
  char *name;
  char *message;
};
typedef struct _call_frame_t *call_frame_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_call_frame_class;

/** @brief Init args for g_call_frame_class. */
typedef struct call_frame_init_t {
  const char *name;
  const char *message;
} call_frame_init_t;

/** @brief Type descriptor for allocator_create. */
extern class_t g_vm_class;

/* ---- Lifecycle ---- */

vm_t vm_create(allocator_t allocator);
void vm_dispose(vm_t self, allocator_t allocator);

/* ---- Accessors ---- */

allocator_t vm_get_allocator(vm_t self);
strmap_t vm_get_modules(vm_t self);
scope_t vm_get_global_scope(vm_t self);
scope_t vm_get_root_scope(vm_t self);
scope_t vm_get_current_scope(vm_t self);
value_t vm_get_type_type(vm_t self);
value_t vm_get_exception_type(vm_t self);
value_t vm_get_interrupt_type(vm_t self);
value_t vm_get_error_type(vm_t self);

/** @brief Get/set the current module identifier.
 *  Used for access control: private fields/props are only accessible
 *  from within the same module. "<builtin>" for built-in types. */
const char *vm_get_current_module_id(vm_t self);
void vm_set_current_module_id(vm_t self, const char *module_id);
value_t vm_get_bool_type(vm_t self);
value_t vm_get_wildcard_type(vm_t self);
value_t vm_get_void_type(vm_t self);
value_t vm_get_nil_type(vm_t self);
value_t vm_get_opaque_type(vm_t self);
value_t vm_get_const_bool_type(vm_t self);
value_t vm_get_i8_type(vm_t self);
value_t vm_get_i16_type(vm_t self);
value_t vm_get_i32_type(vm_t self);
value_t vm_get_i64_type(vm_t self);
value_t vm_get_const_i8_type(vm_t self);
value_t vm_get_const_i16_type(vm_t self);
value_t vm_get_const_i32_type(vm_t self);
value_t vm_get_const_i64_type(vm_t self);
value_t vm_get_u8_type(vm_t self);
value_t vm_get_u16_type(vm_t self);
value_t vm_get_u32_type(vm_t self);
value_t vm_get_u64_type(vm_t self);
value_t vm_get_const_u8_type(vm_t self);
value_t vm_get_const_u16_type(vm_t self);
value_t vm_get_const_u32_type(vm_t self);
value_t vm_get_const_u64_type(vm_t self);
value_t vm_get_f16_type(vm_t self);
value_t vm_get_f32_type(vm_t self);
value_t vm_get_f64_type(vm_t self);
value_t vm_get_const_f16_type(vm_t self);
value_t vm_get_const_f32_type(vm_t self);
value_t vm_get_const_f64_type(vm_t self);
value_t vm_get_str_type(vm_t self);
value_t vm_get_const_str_type(vm_t self);
value_t vm_get_wildcard_tuple_type(vm_t self);

/** @brief Get the global unique wildcard value (type=wildcard).
 *  Used as a sentinel in value-type generic parameters (e.g. Array[i32, ?]).
 *  In type_equal/type_extends, if a parameter is this value, skip comparison.
 */
value_t vm_get_wildcard_value(vm_t self);

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

/* ---- Call stack ---- */

/** @brief Push a call frame onto the VM call stack.
 *  name and message are borrowed references (not cloned/freed).
 *  C functions should call this at entry. */
void vm_push_frame(vm_t self, const char *name, const char *message);

/** @brief Pop the top call frame from the VM call stack.
 *  C functions should call this at exit. */
void vm_pop_frame(vm_t self);

/** @brief Get the current call stack (vec of call_frame_t).
 *  Index 0 = bottom (oldest), last = top (most recent). */
vec_t vm_get_call_stack(vm_t self);

/* ---- Value creation ---- */

/** @brief Create a value with owned copy of data, add to current_scope->values.
 *  If data is NULL, allocates zeroed buffer of type_get_size(type).
 *  If name is non-NULL, creates a name entry in current_scope. */
value_t vm_create_value(vm_t self, type_t type, const void *data,
                        const char *name);

value_t vm_create_value_ref(vm_t self, type_t type, const void *data,
                            const char *name);

/** @brief Create a shadow value (data=NULL, own=false), add to
 * current_scope->values. initialized=true for compile-time placeholder, false
 * for TDZ (undefined). If name is non-NULL, creates a name entry in
 * current_scope. */
value_t vm_create_value_shadow(vm_t self, type_t type, const char *name,
                               bool initialized);

/** @brief Create an array type, register in vm's current scope, and wrap as a
 * value. The array_type_t is added to current_scope->types (auto-dispose). The
 * type value is added to current_scope->values. Returns the type value
 * (value.data = array_type_t, own=true). */
value_t vm_create_array_type_value(vm_t self, type_t element_type,
                                   uint64_t count, bool mut);

/** @brief Create a slice type, register in vm's current scope, and wrap as a
 * value. The slice_type_t is added to current_scope->types (auto-dispose).
 *  Returns the type value (value.data = slice_type_t, own=false). */
value_t vm_create_slice_type_value(vm_t self, type_t element_type, bool mut);

/** @brief Create a tuple type, register in vm's current scope, and wrap as a
 * value. The tuple_type_t is added to current_scope->types (auto-dispose).
 *  Returns the type value (value.data = tuple_type_t, own=false). */
value_t vm_create_tuple_type_value(vm_t self, vec_t element_types, bool mut);

/** @brief Create a callable type, register in vm's current scope, and wrap as a
 * value. The callable_type_t is added to current_scope->types (auto-dispose).
 *  Returns the type value (value.data = callable_type_t, own=false). */
value_t vm_create_callable_type_value(vm_t self, vec_t param_types,
                                      type_t return_type, bool is_variadic,
                                      bool mut, const char *module_id);

/** @brief Create a pointer type, register in vm's current scope, and wrap as a
 * value. The pointer_type_t is added to current_scope->types (auto-dispose).
 *  Returns the type value (value.data = pointer_type_t, own=false). */
value_t vm_create_pointer_type_value(vm_t self, type_t pointee_type, bool mut,
                                     bool is_volatile);

#ifdef __cplusplus
}
#endif
#endif
