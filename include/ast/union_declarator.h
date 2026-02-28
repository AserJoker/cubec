#ifndef _H_CUBEC_NODE_UNION_DECLARATOR_
#define _H_CUBEC_NODE_UNION_DECLARATOR_
#include "ast/node.h"
#include "core/list.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_union_declarator_t {
  struct _cubec_ast_node_t super;
  cubec_list_t types;
} *cubec_ast_union_declarator_t;
cubec_ast_union_declarator_t
cubec_create_ast_union_declarator(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_union_declarator(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end);
#ifdef __cplusplus
}
#endif
#endif