#ifndef _H_CUBEC_CUBEC_STATEMENT_
#define _H_CUBEC_CUBEC_STATEMENT_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
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
node_t read_statement(allocator_t allocator, vec_t tokens, size_t *position,
                      const char *filename);

#ifdef __cplusplus
}
#endif
#endif
