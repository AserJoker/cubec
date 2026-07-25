#ifndef _H_CUBEC_CUBEC_STATEMENT_DO_WHILE_
#define _H_CUBEC_CUBEC_STATEMENT_DO_WHILE_
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
 * @brief AST node for do-while loop statement.
 *
 * Syntax:
 *   do { <body> } while(condition);
 *
 * Condition must be parenthesized. Ends with semicolon.
 *
 * Examples:
 *   do { } while(x > 0);
 */
struct _cubec_statement_do_while_t;
struct _cubec_statement_do_while_t {
  struct _node_t super;
  node_t body;       /**< Block statement (required) */
  node_t condition;  /**< Condition expression (required) */
};
typedef struct _cubec_statement_do_while_t *cubec_statement_do_while_t;

extern type_t g_cubec_statement_do_while_type;

struct _cubec_statement_do_while_init_t {
  location_t location;
  node_t parent;
  node_t body;
  node_t condition;
};
typedef struct _cubec_statement_do_while_init_t cubec_statement_do_while_init_t;

/**
 * @brief Try to parse a do-while statement.
 */
node_t read_statement_do_while(context_t ctx, vec_t tokens,
                                size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
