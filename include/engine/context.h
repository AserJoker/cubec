#ifndef _H_CUBEC_ENGINE_CONTEXT_
#define _H_CUBEC_ENGINE_CONTEXT_
#include "core/allocator.h"
#include "core/diagnostic.h"
#include "core/class.h"
#include "engine/vm.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

struct _module_t;
typedef struct _module_t *module_t;

struct _scope_t;
typedef struct _scope_t *scope_t;

struct context {
  allocator_t allocator;
  vm_t vm;                   /**< owned: VM instance */
  diagnostic_list_t diagnostics;
  scope_t root_scope;        /* borrowed: current module's root scope */
  scope_t current_scope;     /* borrowed: current traversal position */
};

typedef struct context *context_t;

extern class_t g_context_class;

context_t context_create(allocator_t allocator);

void context_dispose(context_t ctx);

int context_get_error_count(context_t ctx);

/* Module registry (delegates to vm) */

module_t context_get_module(context_t ctx, const char *abs_path);

/**
 * @brief Import a module by its import path (as written in source).
 *
 * Pipeline: resolve path → read file → tokenize → parse AST → create module.
 * Results are cached; repeated imports return the existing module.
 * Name collection is NOT performed here — it is a separate phase.
 *
 * @param ctx         Compiler context
 * @param import_path Import path as written in source
 * @return The imported module, or NULL on failure
 */
module_t context_import(context_t ctx, const char *import_path);

/* Scope stack */

void context_push_scope(context_t ctx, scope_t scope);
void context_pop_scope(context_t ctx);

#ifdef __cplusplus
}
#endif
#endif
