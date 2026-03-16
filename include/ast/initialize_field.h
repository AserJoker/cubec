#ifndef _H_CUBEC_AST_INITIALIZE_FIELD_
#define _H_CUBEC_AST_INITIALIZE_FIELD_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_initialize_field_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t identifier;
  cubec_ast_node_t initialize;
} *cubec_ast_initialize_field_t;
cubec_ast_initialize_field_t
cubec_create_ast_initialize_field(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_initialize_field(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end);
#ifdef __cplusplus
}
#endif
#endif