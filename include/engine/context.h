#ifndef _H_CUBEC_ENGINE_CONTEXT_
#define _H_CUBEC_ENGINE_CONTEXT_
#include "core/allocator.h"
#include "core/diagnostic.h"
#include "core/rbtree.h"
#include "core/strmap.h"
#include "core/type.h"
#include "core/vec.h"
#include "engine/value.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

struct _module_t;
typedef struct _module_t *module_t;

struct _scope_t;
typedef struct _scope_t *scope_t;

struct _stype_t;
typedef struct _stype_t *stype_t;

struct context {
  allocator_t allocator;
  diagnostic_list_t diagnostics;
  strmap_t modules;      /* absolute path (string key) → module_t */
  scope_t global_scope;  /* owned: global scope */
  scope_t root_scope;    /* borrowed: current module's root scope */
  scope_t current_scope; /* borrowed: current traversal position */
  rbtree_t types;        /* owned: hash(uint64_t) → stype_t (auto-dispose) */

  /* Primitive type singletons — borrowing pointers into ctx->types */
  stype_t t_void;
  stype_t t_bool;
  stype_t t_i8, t_i16, t_i32, t_i64;
  stype_t t_u8, t_u16, t_u32, t_u64;
  stype_t t_f16, t_f32, t_f64;
  stype_t t_char;
  stype_t t_str;
  stype_t t_nil;
};

typedef struct context *context_t;

extern type_t g_context_type;

context_t context_create(allocator_t allocator);

void context_dispose(context_t ctx);

int context_get_error_count(context_t ctx);

/* Module registry */
module_t context_get_module(context_t ctx, const char *abs_path);

/**
 * @brief Import a module by its import path (as written in source).
 *
 * Pipeline: resolve path → read file → tokenize → parse AST → create module.
 * Results are cached; repeated imports return the existing module.
 * Name collection is NOT performed here — it is a separate phase.
 *
 * Path resolution:
 *   - Relative paths ("./io", "../utils") resolve relative to the importing
 *     module's directory (derived from ctx->root_scope->owner).
 *   - Bare names ("std", "std/vec") resolve against the module search paths.
 *
 * @param ctx         Compiler context
 * @param import_path Import path as written in source (e.g., "std", "./io")
 * @return The imported module, or NULL on failure
 */
module_t context_import(context_t ctx, const char *import_path);

/* Scope stack */

/** Set root_scope and current_scope to the given scope. */
void context_push_scope(context_t ctx, scope_t scope);

/** Restore current_scope to its parent. */
void context_pop_scope(context_t ctx);

/* --------------------------------------------------------------------------
 *  Value creation — unified entry point with generic instantiation
 * -------------------------------------------------------------------------- */

/**
 * @brief Create a value_t with type, optional generic args, and comptime data.
 *
 * If type is generic and generic_args is provided, performs instantiation:
 * computes instance hash from generic_args, looks up in type->implements rbtree,
 * creates new instance if not found.
 *
 * @param ctx           Compiler context
 * @param type          Value's type (borrowing)
 * @param generic_args  vec of comptime_value_t generic arguments (borrowing, nullable)
 * @param data          Comptime value data (borrowing, nullable — NULL for uninitialized)
 * @param is_export     Exported from module
 * @param is_exportlib  Exported with C ABI
 * @param is_extern     External linkage
 * @param is_builtin    Compiler-provided
 * @param is_comptime   Compile-time evaluated
 * @param is_using      Auto-defer at scope exit
 */
value_t context_create_value(context_t ctx, stype_t type, vec_t generic_args,
                             comptime_value_t data,
                             bool is_export, bool is_exportlib, bool is_extern,
                             bool is_builtin, bool is_comptime, bool is_using);

/* Per-kind convenience wrappers (no generic args, all flags false) */

/** @brief Create an integer value. type must be an integer type_kind. */
value_t context_create_int_value(context_t ctx, stype_t type, uint64_t val);

/** @brief Create a float value. type must be a float type_kind. */
value_t context_create_float_value(context_t ctx, stype_t type, double val);

/** @brief Create a bool value. */
value_t context_create_bool_value(context_t ctx, stype_t type, bool val);

/** @brief Create a char value. */
value_t context_create_char_value(context_t ctx, stype_t type, uint8_t val);

/** @brief Create a string value from C string (copied). */
value_t context_create_str_value(context_t ctx, stype_t type, const char *val);

/** @brief Create a nil value. */
value_t context_create_nil_value(context_t ctx, stype_t type);

#ifdef __cplusplus
}
#endif
#endif