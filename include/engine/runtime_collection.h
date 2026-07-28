#ifndef _H_CUBEC_ENGINE_RUNTIME_COLLECTION_
#define _H_CUBEC_ENGINE_RUNTIME_COLLECTION_
#include "core/allocator.h"
#include "core/strmap.h"
#include "core/vec.h"
#include "engine/semantic_type.h"
#include "engine/symbol.h"
#include "core/node.h"
#ifdef __cplusplus
extern "C" {
#endif

struct context;  /* forward declaration — avoid circular include with context.h */

/**
 * @brief Runtime collection result — types and functions needed at C output time.
 *
 * Populated by Pass 5 (context_collect_runtime) using demand-driven
 * diffusion from entry-point functions. Only types and functions
 * reachable from exported/main functions are included.
 */
typedef struct _runtime_collection_t {
  vec_t runtime_types;      /**< vec of semantic_type_t — all types needed at runtime */
  vec_t runtime_functions;  /**< vec of struct symbol*   — all functions needed at runtime */
  vec_t runtime_variables;  /**< vec of struct symbol*   — all global variables needed at runtime */
  vec_t type_worklist;      /**< internal: pending types to diffuse (semantic_type_t) */
  vec_t func_worklist;      /**< internal: pending functions to diffuse (struct symbol*) */
  strmap_t seen_types;      /**< dedup: pointer key -> "1" */
  strmap_t seen_funcs;      /**< dedup: pointer key -> "1" */
  strmap_t seen_vars;       /**< dedup: pointer key -> "1" */
} *runtime_collection_t;

/** @brief Create a runtime collection. */
runtime_collection_t runtime_collection_create(allocator_t allocator);

/** @brief Dispose a runtime collection. */
void runtime_collection_dispose(runtime_collection_t rc, allocator_t allocator);

/**
 * @brief Pass 5: Collect all runtime-needed types and functions.
 *
 * Starts from non-comptime, non-builtin, non-generic exported functions
 * (and the main function if generate_executable is true), then diffuses
 * through type references to find everything needed at C output time.
 *
 * @param ctx                 Checker context (with fully checked AST)
 * @param program             Root AST node
 * @param generate_executable If true, include the main function as an entry point
 */
void context_collect_runtime(struct context *ctx, node_t program, bool generate_executable);

#ifdef __cplusplus
}
#endif
#endif
