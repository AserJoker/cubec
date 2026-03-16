#ifndef _H_CUBEC_AST_ENUM_DECLARATOR_
#define _H_CUBEC_AST_ENUM_DECLARATOR_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_enum_declarator_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t decorators;
  cubec_ast_node_t fields;
  cubec_ast_node_t identifier;
  cubec_ast_node_t type;
} *cubec_ast_enum_declarator_t;
cubec_ast_enum_declarator_t
cubec_create_ast_enum_declarator(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_enum_declarator(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end);
#ifdef __cplusplus
}
#endif
#endif