#ifndef _H_CUBEC_ENGINE_CHECKER_
#define _H_CUBEC_ENGINE_CHECKER_
#include "core/allocator.h"
#include "core/node.h"
#include "core/type.h"
#include "engine/diagnostic.h"
#include "engine/scope.h"
#include "engine/semantic_type.h"
#include "engine/source.h"
#include "core/strmap.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The semantic checker context.
 *        Owns all semantic analysis state: scopes, types, diagnostics.
 */
struct checker {
  allocator_t allocator;

  /* scopes */
  scope_t global_scope;
  scope_t current_scope;

  /* caches */
  strmap_t module_cache;      /**< module name -> scope_t */
  strmap_t type_name_table;   /**< type name -> semantic_type_t */
  strmap_t type_impl_cache;   /**< hash string -> type_impl_t (dedup) */

  /* diagnostics */
  diagnostic_list_t diagnostics;
  source_cache_t sources;

  /* error tracking */
  int error_count;

  /* loop tracking (for break/continue validation) */
  int loop_depth;

  /* sentinel error type */
  semantic_type_t error_type;

  /* builtin types */
  semantic_type_t builtin_void;
  semantic_type_t builtin_bool;
  semantic_type_t builtin_i8, builtin_i16, builtin_i32, builtin_i64;
  semantic_type_t builtin_u8, builtin_u16, builtin_u32, builtin_u64;
  semantic_type_t builtin_f16, builtin_f32, builtin_f64;
  semantic_type_t builtin_char, builtin_string;
  semantic_type_t builtin_nil;
};

typedef struct checker *checker_t;

/** @brief Virtual table for checker. */
extern type_t g_checker_type;

/**
 * @brief Create a new checker context.
 *        Initializes builtin types, global scope, and diagnostic system.
 */
checker_t checker_create(allocator_t allocator);

/**
 * @brief Dispose a checker context and all owned resources.
 */
void checker_dispose(checker_t ctx);

/**
 * @brief Run the full semantic analysis pipeline on a program AST.
 *        Performs multiple passes:
 *        1. Declaration collection (register names)
 *        2. Type resolution (resolve type expressions)
 *        3. Body checking (check function bodies, expressions, statements)
 *
 * @param ctx     The checker context.
 * @param program The program AST root node.
 */
void checker_check_program(checker_t ctx, node_t program);

/**
 * @brief Get the number of errors recorded.
 */
int checker_get_error_count(checker_t ctx);

#ifdef __cplusplus
}
#endif
#endif
