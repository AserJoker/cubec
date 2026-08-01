#ifndef _H_CUBEC_CUBEC_STATEMENT_BREAK_
#define _H_CUBEC_CUBEC_STATEMENT_BREAK_
#include "core/allocator.h"
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for break statement.
 *
 * Syntax:
 *   break;
 *
 * Simple break without label support.
 */
struct _cubec_statement_break_t;
struct _cubec_statement_break_t {
  struct _node_t super;
};
typedef struct _cubec_statement_break_t *cubec_statement_break_t;

extern type_t g_cubec_statement_break_type;

struct _cubec_statement_break_init_t {
  location_t location;
  node_t parent;
};
typedef struct _cubec_statement_break_init_t cubec_statement_break_init_t;

/**
 * @brief Try to parse a break statement.
 */
node_t read_statement_break(context_t ctx, vec_t tokens,
                             size_t *position, const char *filename);

node_t cubec_ast_create_break_stmt(context_t ctx, location_t loc);

#ifdef __cplusplus
}
#endif
#endif
