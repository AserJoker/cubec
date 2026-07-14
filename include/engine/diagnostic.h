#ifndef _H_CUBEC_ENGINE_DIAGNOSTIC_
#define _H_CUBEC_ENGINE_DIAGNOSTIC_
#include "core/location.h"
#include "core/type.h"
#include "core/vec.h"
#include <stddef.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Diagnostic severity levels.
 */
enum diagnostic_severity {
  DIAGNOSTIC_ERROR,   /**< Compilation error */
  DIAGNOSTIC_WARNING, /**< Warning */
  DIAGNOSTIC_NOTE     /**< Supplementary note */
};

/**
 * @brief A supplementary note attached to a diagnostic.
 *        Points to a secondary location with an explanation.
 */
struct diagnostic_note {
  location_t location;   /**< Secondary location */
  char message[256];     /**< Note message */
};

/**
 * @brief A single diagnostic (error, warning, or note).
 *        Has a primary location/message and optional supplementary notes.
 */
struct diagnostic {
  enum diagnostic_severity severity; /**< Severity level */
  location_t primary;                /**< Primary location */
  char message[512];                 /**< Primary message */
  vec_t notes;                       /**< vec of diagnostic_note* */
};

/**
 * @brief List of diagnostics collected during compilation.
 *        Supports emitting with rustc-style formatting.
 */
struct _diagnostic_list_t;
typedef struct _diagnostic_list_t *diagnostic_list_t;

/** @brief Initialization parameters for diagnostic_list_t. */
typedef struct _diagnostic_list_init_t diagnostic_list_init_t;
struct _diagnostic_list_init_t {
  FILE *output; /**< Output stream (NULL defaults to stderr) */
};

/** @brief Virtual table for diagnostic_list_t. */
extern type_t g_diagnostic_list_type;

/** @brief Get the number of diagnostics. */
size_t diagnostic_list_get_size(diagnostic_list_t self);

/** @brief Get the number of errors. */
size_t diagnostic_list_get_error_count(diagnostic_list_t self);

/**
 * @brief Add a diagnostic.
 * @return Pointer to the added diagnostic (for attaching notes).
 */
struct diagnostic *diagnostic_list_push(diagnostic_list_t self,
                                        enum diagnostic_severity severity,
                                        location_t primary, const char *fmt,
                                        ...);

/**
 * @brief Add a note to the most recently pushed diagnostic.
 */
void diagnostic_list_push_note(diagnostic_list_t self, location_t location,
                               const char *fmt, ...);

/**
 * @brief Emit all diagnostics to the output stream.
 *        Uses rustc-style formatting with source lines and caret spans.
 *        Requires a source_cache_t for source line extraction.
 */
typedef struct _source_cache_t *source_cache_fwd_t;
void diagnostic_list_emit(diagnostic_list_t self, source_cache_fwd_t sources);

/**
 * @brief Clear all diagnostics.
 */
void diagnostic_list_clear(diagnostic_list_t self);

#ifdef __cplusplus
}
#endif
#endif
