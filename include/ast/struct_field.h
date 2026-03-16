#ifndef _H_CUBEC_AST_STRUCT_FIELD_
#define _H_CUBEC_AST_STRUCT_FIELD_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_struct_field_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t decorators;
  cubec_ast_node_t declarator;
} *cubec_ast_struct_field_t;
cubec_ast_struct_field_t
cubec_create_ast_struct_field(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_struct_field(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end);
#ifdef __cplusplus
}
#endif
#endif