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

/* forward declaration */
struct checker;

/**
 * @brief Dispatch identifiers for builtin entities.
 *        Used by comptime eval and codegen to route builtin calls.
 */
enum builtin_dispatch {
  BUILTIN_DISPATCH_NONE = 0,
  BUILTIN_DISPATCH_ASSERT,
  BUILTIN_DISPATCH_LENGTH,
  BUILTIN_DISPATCH_TUPLE,
  BUILTIN_DISPATCH_GET,
  BUILTIN_DISPATCH_SET,
};

/**
 * @brief Kind of builtin entity.
 */
enum builtin_kind {
  BUILTIN_FUNC,
  BUILTIN_VAR,
  BUILTIN_TYPE,
};

/**
 * @brief A single entry in the builtin registry.
 */
struct builtin_entry {
  const char *name;               /**< Builtin name (e.g., "assert") */
  enum builtin_kind kind;         /**< Function, variable, or type */
  semantic_type_t type;           /**< Template type (may contain TYPE_GENERIC_PARAM) */
  enum builtin_dispatch dispatch; /**< Dispatch ID for comptime/codegen */
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
                            enum builtin_kind kind, semantic_type_t type,
                            enum builtin_dispatch dispatch);

/* ===== query ===== */

builtin_entry_t builtin_table_lookup(builtin_table_t table, const char *name);

/* ===== defaults ===== */

/**
 * @brief Register the default set of builtins (assert, etc.).
 *        Must be called after checker's builtin types are initialized.
 */
void builtin_table_init_defaults(builtin_table_t table, struct checker *ctx);

#ifdef __cplusplus
}
#endif
#endif
