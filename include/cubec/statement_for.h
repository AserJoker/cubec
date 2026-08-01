#ifndef _H_CUBEC_CUBEC_STATEMENT_FOR_
#define _H_CUBEC_CUBEC_STATEMENT_FOR_
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for C-style for loop statement.
 *
 * Syntax:
 *   for(init; condition; increment) { <body> }
 *
 * C-style three-part for loop. Each part is a statement or expression.
 * Parts are separated by semicolons.
 *
 * Examples:
 *   for(var i = 0; i < 10; i = i + 1) { }
 *   for(;;) { }
 */
struct _cubec_statement_for_t;
struct _cubec_statement_for_t {
  struct _node_t super;
  node_t init;       /**< Init statement/expression (nullable) */
  node_t condition;  /**< Condition expression (nullable) */
  node_t increment;  /**< Increment expression (nullable) */
  node_t body;       /**< Block statement (required) */
};
typedef struct _cubec_statement_for_t *cubec_statement_for_t;

extern type_t g_cubec_statement_for_type;

struct _cubec_statement_for_init_t {
  location_t location;
  node_t parent;
  node_t init;
  node_t condition;
  node_t increment;
  node_t body;
};
typedef struct _cubec_statement_for_init_t cubec_statement_for_init_t;

/**
 * @brief Try to parse a for statement.
 */
node_t read_statement_for(context_t ctx, vec_t tokens,
                           size_t *position, const char *filename);

node_t cubec_ast_create_for_stmt(context_t ctx, location_t loc, node_t init, node_t cond, node_t incr, node_t body);

#ifdef __cplusplus
}
#endif
#endif
