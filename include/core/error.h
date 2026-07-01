#ifndef _H_CUBEC_CORE_ERROR_
#define _H_CUBEC_CORE_ERROR_
#include "core/allocator.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _error_frame_t error_frame_t;
struct _error_frame_t {
  const char *filename;
  const char *funcname;
  size_t line;
};
typedef struct _error_t error_t;
struct _error_t {
  char message[1024];
  error_frame_t stack[64];
  size_t stacktop;
};
extern _Thread_local error_t *g_error;
void throw_error(const char *filename, const char *funcname, size_t line,
                 const char *fmt, ...);
void error_push(const char *filename, const char *funcname, size_t line);
char *error_to_string(error_t *error, allocator_t allocator);
void error_clear();
#define CATCH_ERROR(expr, onerror)                                             \
  ({                                                                           \
    __auto_type res = (expr);                                                  \
    if (g_error) {                                                             \
      error_push(__FILE__, __func__, __LINE__);                                \
      onerror;                                                                 \
    }                                                                          \
    res;                                                                       \
  })
#define CATCH_VOID_ERROR(expr, onerror)                                        \
  do {                                                                         \
    (expr);                                                                    \
    if (g_error) {                                                             \
      error_push(__FILE__, __func__, __LINE__);                                \
      onerror;                                                                 \
    }                                                                          \
  } while (0)
#define TRY(err, expr) CATCH_ERROR(expr, do { return err; } while (0))
#define TRY_VOID(err, expr) CATCH_VOID_ERROR(expr, do { return err; } while (0))
#define TRY_LOCAL(onerror, expr)                                               \
  CATCH_ERROR(expr, do { goto onerror; } while (0))
#define TRY_VOID_LOCAL(onerror, expr)                                          \
  CATCH_VOID_ERROR(expr, do { goto onerror; } while (0))
#define THROW(err, fmt, ...)                                                   \
  do {                                                                         \
    throw_error(__FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);             \
    return err;                                                                \
  } while (0)
#define THROW_LOCAL(onerror, fmt, ...)                                         \
  do {                                                                         \
    throw_error(__FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);             \
    goto onerror;                                                              \
  } while (0)
#ifdef __cplusplus
}
#endif
#endif