#ifndef _H_CUBEC_ENGINE_SOURCE_
#define _H_CUBEC_ENGINE_SOURCE_
#include "core/strmap.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vec.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Cached source file: content + precomputed line offsets for fast
 *        line extraction.
 */
struct source_entry {
  string_t content;   /**< File content as string_t */
  vec_t line_offsets; /**< vec of size_t: byte offset of each line start */
};

/**
 * @brief Source file cache: maps filename -> source_entry.
 *        Loads files on demand, caches for subsequent lookups.
 */
struct _source_cache_t;
typedef struct _source_cache_t *source_cache_t;

/** @brief Virtual table for source_cache_t. */
extern type_t g_source_cache_type;

/**
 * @brief Load a source file into the cache (or return existing entry).
 * @param filename  File path to load.
 * @param content   File content (takes ownership if take_ownership is true).
 * @param take_ownership If true, the content string is moved into the cache.
 * @return Pointer to the source_entry, or NULL on failure.
 */
struct source_entry *source_cache_load(source_cache_t self,
                                       const char *filename,
                                       const char *content,
                                       bool take_ownership);

/**
 * @brief Find a previously loaded source entry.
 * @return Pointer to the source_entry, or NULL if not loaded.
 */
struct source_entry *source_cache_find(source_cache_t self,
                                       const char *filename);

/**
 * @brief Get a specific line from a source entry.
 * @param entry  The source entry.
 * @param line   1-based line number.
 * @return The line content (without newline), or "" if out of range.
 *         The returned pointer is valid as long as the source_entry is alive.
 */
const char *source_entry_get_line(struct source_entry *entry, size_t line);

/**
 * @brief Get the number of lines in a source entry.
 */
size_t source_entry_get_line_count(struct source_entry *entry);

#ifdef __cplusplus
}
#endif
#endif
