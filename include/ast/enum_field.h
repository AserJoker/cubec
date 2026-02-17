#ifndef _H_CUBEC_NODE_ENUM_FIELD_
#define _H_CUBEC_NODE_ENUM_FIELD_
#include "ast/node.h"
#include "core/list.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_enum_field_t {
  struct _cubec_ast_node_t super;
  cubec_list_t decorators;
  cubec_ast_node_t value;
  cubec_ast_node_t identifier;
} *cubec_ast_enum_field_t;
cubec_ast_enum_field_t cubec_create_ast_enum_field(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_enum_field(cubec_allocator_t allocator,
                                           cubec_position_t *position,
                                           const char *end);
#ifdef __cplusplus
}
#endif
#endif