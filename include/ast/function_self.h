#ifndef _H_CUBEC_NODE_FUNCTION_SELF_
#define _H_CUBEC_NODE_FUNCTION_SELF_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_function_self_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t argument;
} *cubec_ast_function_self_t;
cubec_ast_function_self_t
cubec_create_ast_function_self(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_function_self(cubec_allocator_t allocator,
                                              cubec_position_t *position,
                                              const char *end);
#ifdef __cplusplus
}
#endif
#endif