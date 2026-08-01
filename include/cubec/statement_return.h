#ifndef _H_CUBEC_CUBEC_STATEMENT_RETURN_
#define _H_CUBEC_CUBEC_STATEMENT_RETURN_
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
 * @brief AST node for a return statement.
 *
 * Syntax:
 *   return [expression] ;
 *
 * Examples:
 *   return;
 *   return x + 1;
 */
struct _cubec_statement_return_t;
struct _cubec_statement_return_t {
  struct _node_t super;
  node_t expression; /**< Return expression (nullable = bare return) */
};
typedef struct _cubec_statement_return_t *cubec_statement_return_t;

extern type_t g_cubec_statement_return_type;

struct _cubec_statement_return_init_t {
  location_t location;
  node_t parent;
  node_t expression;
};
typedef struct _cubec_statement_return_init_t cubec_statement_return_init_t;

/**
 * @brief Try to parse a return statement.
 * @return A new cubec_statement_return_t node, or NULL if current token
 *         is not 'return' keyword.
 */
node_t read_statement_return(context_t ctx, vec_t tokens,
                              size_t *position, const char *filename);

node_t cubec_ast_create_return_stmt(context_t ctx, location_t loc, node_t expr);

#ifdef __cplusplus
}
#endif
#endif
