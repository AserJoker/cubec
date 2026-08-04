#ifndef _H_CUBEC_CUBEC_STATEMENT_
#define _H_CUBEC_CUBEC_STATEMENT_
#include "core/emit_context.h"
#include "core/node.h"
#include "core/vec.h"
#include "core/writer.h"
#include "engine/context.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Try to parse a statement.
 *
 * Tries each statement parser in order. Currently supported:
 *   - expression statement: \c <expression> ";"
 *   - empty statement: \c ";"
 *
 * @return A new statement node, or NULL if the current token does not start a
 *         valid statement.
 */
node_t read_statement(context_t ctx, vec_t tokens, size_t *position,
                      const char *filename);

void write_statement(writer_t wr, node_t stmt);

void emit_statement(emit_context_t ctx, node_t stmt);

#ifdef __cplusplus
}
#endif
#endif
