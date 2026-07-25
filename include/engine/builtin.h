#ifndef _H_CUBEC_ENGINE_BUILTIN_
#define _H_CUBEC_ENGINE_BUILTIN_
#include "engine/semantic_type.h"
#include "core/allocator.h"
#include "core/strmap.h"
#include "core/vec.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/* forward declarations */
struct context;
struct comptime_eval;
struct comptime_value;
struct builtin_entry;
typedef struct _node_t *node_t;

/**
 * @brief Comptime evaluation callback for builtin functions.
 *        Called when a builtin function is invoked at comptime.
 *        Return the evaluated result, or COMPTIME_VALUE_ERROR on failure.
 */
typedef struct comptime_value *(*builtin_eval_call_fn)(
    struct comptime_eval *eval, struct context *ctx, node_t node,
    struct builtin_entry *be);

/**
 * @brief A single entry in the builtin registry.
 */
struct builtin_entry {
  const char *name;               /**< Builtin name (e.g., "assert") */
  semantic_type_t type;           /**< Function type declaration */
  builtin_eval_call_fn eval_call; /**< Comptime eval callback (NULL for types) */
};

typedef struct builtin_entry *builtin_entry_t;

/** @brief Virtual table for builtin_entry. */
extern type_t g_builtin_entry_type;

/**
 * @brief Builtin registry — maps names to builtin entries.
 */
struct builtin_table {
  allocator_t allocator; /**< owning allocator */
  strmap_t entries;    /**< name → builtin_entry* */
  vec_t all_entries;   /**< for iteration/cleanup */
};

typedef struct builtin_table *builtin_table_t;

/* ===== lifecycle ===== */

builtin_table_t builtin_table_create(allocator_t allocator);
void builtin_table_dispose(builtin_table_t table, allocator_t allocator);

/* ===== registration ===== */

/**
 * @brief Register a builtin entry. Name is NOT copied — must outlive the table.
 */
void builtin_table_register(builtin_table_t table, const char *name,
                            semantic_type_t type, builtin_eval_call_fn eval_call);

/* ===== query ===== */

builtin_entry_t builtin_table_lookup(builtin_table_t table, const char *name);

/* ===== defaults ===== */

/**
 * @brief Register the default set of builtins (assert, etc.).
 *        Must be called after checker's builtin types are initialized.
 */
void builtin_table_init_defaults(builtin_table_t table, struct context *ctx);

#ifdef __cplusplus
}
#endif
#endif
