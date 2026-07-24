#ifndef _H_CUBEC_ENGINE_CHECKER_
#define _H_CUBEC_ENGINE_CHECKER_
#include "core/allocator.h"
#include "core/node.h"
#include "core/type.h"
#include "engine/diagnostic.h"
#include "engine/scope.h"
#include "engine/semantic_type.h"
#include "engine/builtin.h"
#include "engine/source.h"
#include "engine/flow_state.h"
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
  vec_t all_scopes;           /**< all child scopes for cleanup */

  /* caches */
  strmap_t module_cache;      /**< resolved path -> module_entry_t (imported modules) */
  strmap_t type_name_table;   /**< type name -> semantic_type_t */
  strmap_t type_impl_cache;   /**< hash string -> type_impl_t (dedup) */

  /* all semantic types for cleanup */
  vec_t all_types;            /**< vec of semantic_type_t (auto_dispose) */

  /* diagnostics */
  diagnostic_list_t diagnostics;
  source_cache_t sources;

  /* error tracking */
  int error_count;

  /* loop tracking (for break/continue validation) */
  int loop_depth;

  /* flow analysis (for unreachable code, return exhaustiveness, TDZ) */
  flow_state_t current_flow;

  /* sentinel error type */
  semantic_type_t error_type;

  /* builtin types */
  semantic_type_t builtin_void;
  semantic_type_t builtin_bool;
  semantic_type_t builtin_i8, builtin_i16, builtin_i32, builtin_i64;
  semantic_type_t builtin_u8, builtin_u16, builtin_u32, builtin_u64;
  semantic_type_t builtin_f16, builtin_f32, builtin_f64;
  semantic_type_t builtin_char, builtin_str;
  semantic_type_t builtin_nil;
  semantic_type_t builtin_opaque;

  /* builtin registry */
  builtin_table_t builtin_table;

  /* comptime evaluator */
  struct comptime_eval *comptime_eval;

  /* test block tracking */
  int test_count;
  int test_fail_count;

  /* assignment context flag: union field write is allowed */
  bool in_assignment_lhs;

  /* test block context flag: assert is only allowed inside test blocks */
  bool in_test_block;

  /* fatal error flag: set by panic, stops all further evaluation */
  bool fatal_error;

  /* current file being compiled (for import path resolution) */
  const char *current_file;

  /* project context (lazy-initialized on first non-relative import) */
  const char *project_root;   /**< project root dir (manifest.json location, malloc'd) */
  const char *cubec_home;     /**< CUBEC_HOME path (malloc'd, from env or project_root) */
  strmap_t manifest_deps;     /**< dep_name -> "1" (declared dependencies) */

  /* generic monomorphization worklist (Pass 4) */
  vec_t body_check_worklist;   /**< vec of body_check_entry_t* */
  strmap_t checked_bodies;     /**< cache key -> "1" (already body-checked) */
};

typedef struct checker *checker_t;

/**
 * @brief Entry in the generic monomorphization worklist.
 *        Tracks a function body that needs to be checked.
 */
typedef struct {
  struct symbol *func_sym;     /**< Function symbol (template symbol for generics) */
  semantic_type_t inst_type;   /**< Instantiated function type (original type for non-generics) */
  strmap_t type_bindings;      /**< Name → concrete semantic_type_t (NULL for non-generics) */
  scope_t scope_root;          /**< Parent scope for body checking */
  bool is_method;              /**< True if this is a type method */
  semantic_type_t host_type;   /**< Host type for methods (NULL for free functions) */
} body_check_entry_t;

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
 * @brief Check an expression and return its semantic type.
 *        Public wrapper for internal _check_expression, used by resolver.
 */
semantic_type_t checker_check_expression(checker_t ctx, node_t expr);

/**
 * @brief Get the number of errors recorded.
 */
int checker_get_error_count(checker_t ctx);

#ifdef __cplusplus
}
#endif
#endif
