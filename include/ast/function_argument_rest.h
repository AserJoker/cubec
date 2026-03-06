#ifndef _H_CUBEC_NODE_FUNCTION_ARGUMENT_REST_
#define _H_CUBEC_NODE_FUNCTION_ARGUMENT_REST_
#include "ast/node.h"
#include "core/list.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_function_argument_rest_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t identifier;
  cubec_ast_node_t type;
  cubec_list_t decorators;
} *cubec_ast_function_argument_rest_t;
cubec_ast_function_argument_rest_t
cubec_create_ast_function_argument_rest(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_function_argument_rest(cubec_allocator_t allocator,
                                                  cubec_position_t *position,
                                                  const char *end);
#ifdef __cplusplus
}
#endif
#endif