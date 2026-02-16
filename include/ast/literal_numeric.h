#ifndef _H_CUBEC_NODE_LITERAL_NUMERIC_
#define _H_CUBEC_NODE_LITERAL_NUMERIC_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_literal_numeric_t {
  struct _cubec_ast_node_t super;
} *cubec_ast_literal_numeric_t;
cubec_ast_literal_numeric_t
cubec_create_ast_literal_numeric(cubec_allocator_t allocator);

cubec_ast_node_t cubec_read_ast_literal_numeric(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end);
#ifdef __cplusplus
}
#endif
#endif