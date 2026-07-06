#ifndef _H_CUBEC_CORE_ERROR_
#define _H_CUBEC_CORE_ERROR_
#include "core/allocator.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <threads.h>
#ifdef __cplusplus
extern "C" {
#endif
/** @brief A single call-stack frame recording where an error was propagated. */
typedef struct _error_frame_t error_frame_t;
struct _error_frame_t {
  const char *filename;  /**< Source file name */
  const char *funcname;  /**< Function name */
  size_t line;           /**< Line number */
};

/** @brief Error object containing a message and a call-stack trace (up to 64 frames). */
typedef struct _error_t err_t;
struct _error_t {
  char message[1024];          /**< Formatted error message (vsnprintf, 1024 bytes max) */
  error_frame_t stack[64];     /**< Call-stack frames (frame 0 = throw site) */
  size_t stacktop;             /**< Number of frames currently in the stack */
};

/** @brief Thread-local pointer to the current in-flight error (NULL if no error). */
extern thread_local err_t *g_error;

/**
 * @brief Throw a formatted error at the current location.
 *        Sets g_error and records the throw site as frame 0.
 * @note  Usually invoked via the THROW() or THROW_LOCAL() macros.
 */
void throw_error(const char *filename, const char *funcname, size_t line,
                 const char *fmt, ...);

/**
 * @brief Push the current source location onto the error call-stack.
 *        NULL-safe: no-op if no error is in flight.
 * @note  Usually invoked via TRY() / CATCH_ERROR() macros.
 */
void error_push(const char *filename, const char *funcname, size_t line);

/**
 * @brief Format the error and its full call-stack into a human-readable string.
 * @param error     The error to format (if NULL, returns NULL).
 * @param allocator Allocator for the result string (if NULL, uses malloc).
 * @return Formatted error string (caller must free via matching allocator or free()).
 */
char *error_to_string(err_t *error, allocator_t allocator);

/**
 * @brief Clear the current thread-local error. After this call, g_error is NULL.
 */
void error_clear();
/**
 * @brief Execute expr; if it throws, push this location and run onerror.
 *        Uses GCC statement-expression, returns the result of expr.
 * @note  Requires -std=gnu11 or equivalent (GCC extensions).
 */
#define CATCH_ERROR(expr, onerror)                                             \
  ({                                                                           \
    typeof(expr) res = (expr);                                                  \
    if (g_error) {                                                             \
      error_push(__FILE__, __func__, __LINE__);                                \
      onerror;                                                                 \
    }                                                                          \
    res;                                                                       \
  })

/**
 * @brief Like CATCH_ERROR but for void expressions (no return value).
 */
#define CATCH_VOID_ERROR(expr, onerror)                                        \
  do {                                                                         \
    (expr);                                                                    \
    if (g_error) {                                                             \
      error_push(__FILE__, __func__, __LINE__);                                \
      onerror;                                                                 \
    }                                                                          \
  } while (0)

/**
 * @brief Rust-style ? operator: execute expr, return err if it throws.
 *        Expands to: CATCH_ERROR(expr, do { return err; } while (0))
 */
#define TRY(err, expr) CATCH_ERROR(expr, do { return err; } while (0))

/** @brief Like TRY but for void expressions. */
#define TRY_VOID(err, expr) CATCH_VOID_ERROR(expr, do { return err; } while (0))

/**
 * @brief Execute expr; on error, push this location and goto onerror label.
 */
#define TRY_LOCAL(onerror, expr)                                               \
  CATCH_ERROR(expr, do { goto onerror; } while (0))

/** @brief Like TRY_LOCAL but for void expressions. */
#define TRY_VOID_LOCAL(onerror, expr)                                          \
  CATCH_VOID_ERROR(expr, do { goto onerror; } while (0))

/**
 * @brief Throw a formatted error and return err from the current function.
 */
#define THROW(err, fmt, ...)                                                   \
  do {                                                                         \
    throw_error(__FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);             \
    return err;                                                                \
  } while (0)

/**
 * @brief Throw a formatted error and goto onerror label (for cleanup paths).
 */
#define THROW_LOCAL(onerror, fmt, ...)                                         \
  do {                                                                         \
    throw_error(__FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__);             \
    goto onerror;                                                              \
  } while (0)
#ifdef __cplusplus
}
#endif
#endif