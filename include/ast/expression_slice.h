#ifndef _H_CUBEC_NODE_EXPRESSION_SLICE_
#define _H_CUBEC_NODE_EXPRESSION_SLICE_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cubec_ast_expression_slice_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t host;
  cubec_ast_node_t start;
  cubec_ast_node_t end;
} *cubec_ast_expression_slice_t;

cubec_ast_expression_slice_t
cubec_create_ast_expression_slice(cubec_allocator_t allocator);

cubec_ast_node_t cubec_read_ast_expression_slice(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end);

#ifdef __cplusplus
}
#endif
#endif