#ifndef _H_CUBEC_ENGINE_CONTEXT_
#define _H_CUBEC_ENGINE_CONTEXT_
#include "core/allocator.h"
#include "core/diagnostic.h"
#include "core/strmap.h"
#include "core/type.h"
#include "core/vec.h"
#include <stddef.h>
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
  vec_t types;           /* owned: global type_t registry (auto-dispose) */

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

#ifdef __cplusplus
}
#endif
#endif