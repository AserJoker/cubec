#ifndef _H_CUBEC_CORE_TOKEN_
#define _H_CUBEC_CORE_TOKEN_
#include "core/location.h"
#include "core/type.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
/**
 * @brief Base class for all tokens produced by the lexer.
 *        Stores the token kind and its source location.
 */
struct _token_t;
typedef struct _token_t *token_t;

/** @brief Virtual table for token_t. */
extern type_t g_token_type;

/** @brief Initialization parameters for token_t. */
struct _token_init_t {
  uint32_t kind;          /**< Token kind (see cubec/token.h for enum) */
  location_t location;    /**< Source-code span */
};
typedef struct _token_init_t token_init_t;

/**
 * @brief Get the token's kind (e.g., IDENTIFIER, NUMERIC, SYMBOL, etc.).
 */
uint32_t token_get_kind(token_t self);

/**
 * @brief Get a mutable pointer to the token's source location.
 */
location_t *token_get_location(token_t self);

/**
 * @brief Check if the token matches both a kind and an exact source text.
 * @param self  The token.
 * @param kind  Expected kind.
 * @param text  Expected source text (compared via location_is).
 * @return true if both match.
 */
bool token_is(token_t self, uint32_t kind, const char *text);

/**
 * @brief Get the token's source text as a null-terminated C string.
 *        The pointer is valid as long as the token (and its source buffer) is alive.
 */
const char *token_get_string(token_t self);

/**
 * @brief Get the length of the token's source text in bytes.
 */
size_t token_get_string_length(token_t self);
#ifdef __cplusplus
}
#endif
#endif