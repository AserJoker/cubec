#ifndef _H_CUBEC_CORE_LOCATION_
#define _H_CUBEC_CORE_LOCATION_
#include "core/allocator.h"
#include "core/position.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
/**
 * @brief A source-code span defined by a filename and begin/end positions.
 *        The span is half-open: [begin.offset, end.offset).
 */
typedef struct _location_t location_t;
struct _location_t {
  const char *filename;   /**< Source file name */
  position_t begin;       /**< Start position (inclusive) */
  position_t end;         /**< End position (exclusive) */
};

/**
 * @brief Extract the source text covered by this location.
 * @param loc       The location span.
 * @param allocator Allocator for the returned string.
 * @return Null-terminated copy of the source text between begin and end offsets.
 */
char *location_get(location_t *loc, allocator_t allocator);

/**
 * @brief Compare the location's source text against a given string.
 * @param loc  The location span.
 * @param str  The string to compare against.
 * @return true if the source text matches str exactly (length and content).
 */
bool location_is(location_t *loc, const char *str);
#ifdef __cplusplus
}
#endif
#endif