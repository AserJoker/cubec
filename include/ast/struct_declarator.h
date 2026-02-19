#ifndef _H_CUBEC_NODE_STRUCT_DECLARATOR_
#define _H_CUBEC_NODE_STRUCT_DECLARATOR_
#include "ast/node.h"
#include "core/list.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_struct_declarator_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t identifier;
  cubec_list_t decorators;
  cubec_list_t fields;
  cubec_list_t methods;
  cubec_list_t attributes;
} *cubec_ast_struct_declarator_t;
cubec_ast_struct_declarator_t
cubec_create_ast_struct_declarator(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_struct_declarator(cubec_allocator_t allocator,
                                                  cubec_position_t *position,
                                                  const char *end);
#ifdef __cplusplus
}
#endif
#endif