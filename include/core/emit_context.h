#ifndef _H_CUBEC_CORE_EMIT_CONTEXT_
#define _H_CUBEC_CORE_EMIT_CONTEXT_
#include "core/string.h"
#include "core/token.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Context for the AST → token emission phase.
 *
 * Holds the source token list (for comment recovery), the output token list,
 * a string table (for newly created tokens), and the current indent level.
 */
struct _emit_context_t {
  allocator_t allocator;
  vec_t source_tokens;     /**< Source token list from lexing */
  bool owns_source_tokens; /**< Whether we own source_tokens */
  size_t source_token_idx; /**< Current scan position in source_tokens */
  vec_t output_tokens;     /**< Output token list (vec_t of token_t) */
  vec_t string_table;      /**< String table (vec_t of string_t) */
  int32_t indent_level;    /**< Current indentation level */
};
typedef struct _emit_context_t *emit_context_t;

/**
 * @brief Create an emit context.
 * @param allocator      The allocator to use.
 * @param source_tokens  The source token list (must outlive this context).
 */
emit_context_t emit_context_create(allocator_t allocator, vec_t source_tokens);


/**
 * @brief Dispose an emit context and all its resources.
 */
void emit_context_dispose(emit_context_t ctx);

/* ---- Emit helpers ---- */

/** @brief Emit a keyword token (CUBEC_TOKEN_KEYWORD). */
void emit_keyword(emit_context_t ctx, const char *text);
/** @brief Emit a symbol token (CUBEC_TOKEN_SYMBOL). */
void emit_symbol(emit_context_t ctx, const char *text);
/** @brief Emit an identifier token (CUBEC_TOKEN_IDENTIFIER). */
void emit_identifier(emit_context_t ctx, const char *text);
/** @brief Emit a numeric literal token (CUBEC_TOKEN_NUMERIC). */
void emit_numeric(emit_context_t ctx, const char *text);
/** @brief Emit a string literal token (CUBEC_TOKEN_STRING). */
void emit_string_literal(emit_context_t ctx, const char *text);
/** @brief Emit a char literal token (CUBEC_TOKEN_CHAR). */
void emit_char_literal(emit_context_t ctx, const char *text);

/* ---- Formatting helpers ---- */

/** @brief Emit a space (CUBEC_TOKEN_WHITESPACE " "). */
void emit_space(emit_context_t ctx);
/** @brief Emit a newline + indentation (CUBEC_TOKEN_WHITESPACE "\n" + indent). */
void emit_newline(emit_context_t ctx);
/** @brief Adjust indent level by delta. Does NOT emit any token. */
void emit_indent(emit_context_t ctx, int32_t delta);

/* ---- Comment recovery ---- */

/**
 * @brief Recover comment tokens from source between current cursor and target.
 *
 * Scans source_tokens from source_token_idx, cloning any COMMENT or
 * MULTILINE_COMMENT tokens whose offset is before target_offset into
 * the output. Advances source_token_idx.
 */
void recover_comments_to(emit_context_t ctx, const char *target_offset);

#ifdef __cplusplus
}
#endif
#endif
