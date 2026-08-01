#ifndef _H_CUBEC_CUBEC_STATEMENT_
#define _H_CUBEC_CUBEC_STATEMENT_
#include "engine/context.h"
#include "core/node.h"
#include "core/vec.h"
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

#ifdef __cplusplus
}
#endif
#endif
