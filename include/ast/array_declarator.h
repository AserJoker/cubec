#ifndef _H_CUBEC_AST_ARRAY_DECLARATOR_
#define _H_CUBEC_AST_ARRAY_DECLARATOR_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cubec_ast_array_declarator_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t length;
  cubec_ast_node_t item_type;
} *cubec_ast_array_declarator_t;

cubec_ast_array_declarator_t
cubec_create_ast_array_declarator(cubec_allocator_t allocator);

cubec_ast_node_t cubec_read_ast_array_declarator(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end);

#ifdef __cplusplus
}
#endif
#endif