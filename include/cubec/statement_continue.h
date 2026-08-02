#ifndef _H_CUBEC_CUBEC_STATEMENT_CONTINUE_
#define _H_CUBEC_CUBEC_STATEMENT_CONTINUE_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/writer.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for continue statement.
 *
 * Syntax:
 *   continue;
 *
 * Simple continue without label support.
 */
struct _cubec_statement_continue_t;
struct _cubec_statement_continue_t {
  struct _node_t super;
};
typedef struct _cubec_statement_continue_t *cubec_statement_continue_t;

extern type_t g_cubec_statement_continue_type;

struct _cubec_statement_continue_init_t {
  location_t location;
  node_t parent;
};
typedef struct _cubec_statement_continue_init_t cubec_statement_continue_init_t;

/**
 * @brief Try to parse a continue statement.
 */
node_t read_statement_continue(context_t ctx, vec_t tokens, size_t *position,
                               const char *filename);

node_t create_statement_continue(context_t ctx, location_t loc);

void write_statement_continue(writer_t writer, node_t stmt);

#ifdef __cplusplus
}
#endif
#endif
