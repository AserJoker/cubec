#ifndef _H_CUBEC_CORE_POSITION_
#define _H_CUBEC_CORE_POSITION_
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>
/**
 * @brief A 1-based line/column position in source code, with a raw pointer into
 *        the source buffer for efficient text extraction.
 */
typedef struct _position_t position_t;
struct _position_t {
  size_t line;          /**< 1-based line number */
  size_t column;        /**< 1-based column number (byte offset within line) */
  const char *offset;   /**< Pointer into the source text buffer at this position */
};
#ifdef __cplusplus
}
#endif
#endif