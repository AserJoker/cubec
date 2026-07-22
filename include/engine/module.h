#ifndef _H_CUBEC_ENGINE_MODULE_
#define _H_CUBEC_ENGINE_MODULE_
#include "core/type.h"
#include "engine/checker.h"
#include "engine/scope.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Module loading state (for cycle detection).
 */
enum module_state {
  MODULE_PARSING,   /**< Declaration collection in progress */
  MODULE_PARSED,    /**< Declaration collection complete */
  MODULE_CHECKED,   /**< Type checking complete */
};

/**
 * @brief Entry in the module cache — tracks a loaded module's state and resources.
 */
typedef struct module_entry {
  scope_t scope;             /**< Module's global scope */
  enum module_state state;   /**< Current loading state */
  checker_t checker;         /**< Checker that compiled this module */
  char *resolved_path;       /**< Resolved file path (malloc'd, must free) */
  char *source;              /**< File content (malloc'd, must free) */
  vec_t tokens;              /**< Token list (owned by module's allocator) */
  node_t program;            /**< AST root (owned by module's allocator) */
} *module_entry_t;

/**
 * @brief Create a module entry.
 */
module_entry_t module_entry_create(const char *resolved_path);

/**
 * @brief Dispose a module entry and all its resources.
 */
void module_entry_dispose(module_entry_t entry);

/**
 * @brief Resolve an import path relative to the current file.
 * Returns a malloc'd absolute path with .cubec extension appended.
 * Returns NULL on failure.
 */
char *module_resolve_path(const char *import_path, const char *current_file);

/**
 * @brief Read a file's contents. Returns malloc'd buffer, or NULL on failure.
 */
char *module_read_file(const char *path, size_t *out_len);

#ifdef __cplusplus
}
#endif
#endif
