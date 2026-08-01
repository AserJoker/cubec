#ifndef _H_CUBEC_CUBEC_STATEMENT_IF_
#define _H_CUBEC_CUBEC_STATEMENT_IF_
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
 * @brief AST node for if/else-if/else conditional statement.
 *
 * Syntax:
 *   if(condition) { <body> } else if(condition) { <body> } else { <body> }
 *
 * Condition must be parenthesized. Body is a block statement.
 * else-if chains are represented as nested statement_if nodes
 * in the else_branch field.
 *
 * Examples:
 *   if(x > 0) { }
 *   if(x > 0) { } else { }
 *   if(x > 0) { } else if(x < 0) { } else { }
 */
struct _cubec_statement_if_t;
struct _cubec_statement_if_t {
  struct _node_t super;
  node_t condition;    /**< Condition expression (required) */
  node_t then_branch;  /**< Block statement for then branch (required) */
  node_t else_branch;  /**< Block or if statement for else branch (nullable) */
};
typedef struct _cubec_statement_if_t *cubec_statement_if_t;

extern type_t g_cubec_statement_if_type;

struct _cubec_statement_if_init_t {
  location_t location;
  node_t parent;
  node_t condition;
  node_t then_branch;
  node_t else_branch;
};
typedef struct _cubec_statement_if_init_t cubec_statement_if_init_t;

/**
 * @brief Try to parse an if statement.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_statement_if_t node, or NULL if current token
 *         is not 'if' keyword.
 */
node_t read_statement_if(context_t ctx, vec_t tokens,
                          size_t *position, const char *filename);

node_t cubec_ast_create_if_stmt(context_t ctx, location_t loc, node_t cond, node_t then_branch, node_t else_branch);

#ifdef __cplusplus
}
#endif
#endif
