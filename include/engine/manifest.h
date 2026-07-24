#ifndef _H_CUBEC_ENGINE_MANIFEST_
#define _H_CUBEC_ENGINE_MANIFEST_
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Manifest.json parsing for the Cubec module system.
 *
 * Extracts project name and dependency names from manifest.json.
 * Uses cJSON for JSON parsing.
 */

/**
 * @brief Parse manifest.json in the given directory.
 *
 * Reads `<dir>/manifest.json` and extracts:
 * - `name` field → out_name (malloc'd, caller must free)
 * - `deps` keys → out_dep_names (array of malloc'd strings, NULL-terminated)
 *
 * @param dir           Directory containing manifest.json
 * @param out_name      Output: project name (malloc'd), or NULL if not needed
 * @param out_dep_names Output: NULL-terminated array of dep name strings,
 *                      each malloc'd (caller must free each string and the array).
 *                      NULL if no deps or on failure.
 * @return 0 on success, -1 on failure (file not found, parse error, etc.)
 */
int manifest_parse(const char *dir, char **out_name, char ***out_dep_names);

/**
 * @brief Find the project root directory by walking up from a file path.
 *
 * Searches for manifest.json starting from the file's directory,
 * then walking up the directory tree. Returns the first directory
 * containing manifest.json, or NULL if none found.
 *
 * @param file_path  Starting file path
 * @return Project root directory (malloc'd, caller must free), or NULL
 */
char *manifest_find_root(const char *file_path);

/**
 * @brief Free a dep_names array returned by manifest_parse.
 */
void manifest_free_dep_names(char **dep_names);

#ifdef __cplusplus
}
#endif
#endif
