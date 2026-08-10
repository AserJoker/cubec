#ifndef _H_CUBEC_CORE_STRING_
#define _H_CUBEC_CORE_STRING_
#ifdef __cplusplus
extern "C" {
#endif
#include "core/allocator.h"
#include "core/class.h"
#include <stdint.h>
/**
 * @brief Dynamic null-terminated string (like std::string).
 *        Capacity grows by doubling. Supports clone and move semantics.
 */
struct _string_t;
typedef struct _string_t *string_t;

/** @brief Initialization parameters for string_t. */
typedef struct _string_init_t string_init_t;
struct _string_init_t {
  const char *str;  /**< Initial string content (may be NULL for empty) */
};

/** @brief Virtual table for string_t. */
extern class_t g_string_class;

/**
 * @brief Get the null-terminated C string content.
 * @return Pointer to internal buffer (valid as long as the string_t is alive).
 */
const char *string_get(string_t self);

/**
 * @brief Replace the string content with a new C string.
 * @return New size in bytes (including null terminator).
 */
size_t string_set(string_t self, const char *str);

/**
 * @brief Get the string length in bytes (excluding null terminator).
 */
size_t string_get_length(string_t self);

/**
 * @brief Append a C string to the end. Reallocates if capacity is insufficient.
 * @return New size in bytes (including null terminator).
 */
size_t string_concat(string_t self, const char *another);

/**
 * @brief Append exactly len bytes from another (not necessarily null-terminated).
 *        Appends a null terminator after the copied bytes.
 * @return New size in bytes (including null terminator).
 */
size_t string_nconcat(string_t self, const char *another, size_t len);

/**
 * @brief Clone a C string using the allocator.
 *
 * Replaces strdup(). Allocates exactly strlen(str)+1 bytes via allocator_alloc.
 * Use cstring_free() to release — do NOT use free().
 *
 * @param allocator  Allocator to allocate from
 * @param str        Source C string (must not be NULL)
 * @return New null-terminated copy owned by the caller
 */
char *cstring_clone(allocator_t allocator, const char *str);

#ifdef __cplusplus
}
#endif
#endif