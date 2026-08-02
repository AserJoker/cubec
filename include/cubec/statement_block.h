#ifndef _H_CUBEC_CUBEC_STATEMENT_BLOCK_
#define _H_CUBEC_CUBEC_STATEMENT_BLOCK_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/writer.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_statement_block_t;
struct _cubec_statement_block_t {
  struct _node_t super;
  vec_t statements; /**< vec_t of statement nodes */
};
typedef struct _cubec_statement_block_t *cubec_statement_block_t;

extern type_t g_cubec_statement_block_type;

struct _cubec_statement_block_init_t {
  location_t location;
  node_t parent;
  vec_t statements;
};
typedef struct _cubec_statement_block_init_t cubec_statement_block_init_t;

/**
 * @brief Try to parse a block statement: \c { <statements> }
 *
 * A block is a sequence of statements enclosed in curly braces. It introduces
 * a new scope. The block may be empty (\c {} ).
 *
 * @return A new cubec_statement_block_t node, or NULL if the current token
 *         is not \c "{".
 */
node_t read_statement_block(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename);

node_t cubec_ast_create_block(context_t ctx, location_t loc, vec_t statements);

void cubec_ast_write_block_stmt(writer_t writer, node_t stmt);

#ifdef __cplusplus
}
#endif
#endif
