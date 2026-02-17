#ifndef _H_CUBEC_NODE_INITIALIZE_LIST_
#define _H_CUBEC_NODE_INITIALIZE_LIST_
#include "ast/node.h"
#include "core/list.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_initialize_list_t {
  struct _cubec_ast_node_t super;
  cubec_list_t fields;
} *cubec_ast_initialize_list_t;
cubec_ast_initialize_list_t
cubec_create_ast_initialize_list(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_initialize_list(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end);
#ifdef __cplusplus
}
#endif
#endif