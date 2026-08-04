#ifndef _H_CUBEC_CUBEC_STATEMENT_WHILE_
#define _H_CUBEC_CUBEC_STATEMENT_WHILE_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "core/writer.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for while loop statement.
 *
 * Syntax:
 *   while(condition) { <body> }
 *
 * Condition must be parenthesized.
 *
 * Examples:
 *   while(x > 0) { }
 */
struct _cubec_statement_while_t;
struct _cubec_statement_while_t {
  struct _node_t super;
  node_t condition; /**< Condition expression (required) */
  node_t body;      /**< Block statement (required) */
};
typedef struct _cubec_statement_while_t *cubec_statement_while_t;

extern type_t g_cubec_statement_while_type;

struct _cubec_statement_while_init_t {
  location_t location;
  node_t parent;
  node_t condition;
  node_t body;
};
typedef struct _cubec_statement_while_init_t cubec_statement_while_init_t;

/**
 * @brief Try to parse a while statement.
 */
node_t read_statement_while(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename);

node_t create_create_while(context_t ctx, location_t loc, node_t cond,
                           node_t body);

void write_statement_while(writer_t writer, node_t node);

void emit_statement_while(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
