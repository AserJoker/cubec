#ifndef _H_CUBEC_AST_INTERFACE_DECLARATOR_
#define _H_CUBEC_AST_INTERFACE_DECLARATOR_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_interface_declarator_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t args;
  cubec_ast_node_t type;
} *cubec_ast_interface_declarator_t;

cubec_ast_interface_declarator_t
cubec_create_ast_interface_declarator(cubec_allocator_t allocator);

cubec_ast_node_t cubec_read_ast_interface_declarator(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
#ifdef __cplusplus
}
#endif
#endif