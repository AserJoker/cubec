#ifndef _H_CUBEC_ENGINE_MODULE_
#define _H_CUBEC_ENGINE_MODULE_
#include "core/type.h"
#include "engine/context.h"
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
  context_t ctx;         /**< Checker that compiled this module */
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

/**
 * @brief Resolve an import path with support for std/project/global deps.
 *
 * Resolution priority:
 * 1. Relative path (./ ../) → relative to current_file (delegates to module_resolve_path)
 * 2. std/ prefix → ${cubec_home}/library/${import_path}/index.cubec
 * 3. Project dep → ${project_root}/library/${import_path}/index.cubec
 * 4. Global dep → ${cubec_home}/library/${import_path}/index.cubec
 *
 * Directory paths automatically resolve to index.cubec.
 * The first path segment of a non-relative import is the dependency name.
 * If manifest_deps is non-NULL and the dep name is not in it, is_ghost is set true.
 *
 * @param import_path   Raw import path string
 * @param current_file  Current source file path (for relative resolution)
 * @param cubec_home    CUBEC_HOME path (may be NULL → defaults to ".")
 * @param project_root  Project root dir (may be NULL → single-file mode)
 * @param manifest_deps Dependency name set (may be NULL → allow all)
 * @param is_ghost      Output: true if dep not declared in manifest (ghost dep)
 * @return Resolved absolute path (malloc'd), or NULL on failure
 */
char *module_resolve_import(const char *import_path,
                            const char *current_file,
                            const char *cubec_home,
                            const char *project_root,
                            strmap_t manifest_deps,
                            bool *is_ghost);

#ifdef __cplusplus
}
#endif
#endif
