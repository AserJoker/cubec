#ifndef _H_CUBEC_AST_SWITCH_CASE_
#define _H_CUBEC_AST_SWITCH_CASE_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_switch_case_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t condition;
  cubec_ast_node_t statements;
} *cubec_ast_switch_case_t;
cubec_ast_switch_case_t
cubec_create_ast_switch_case(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_switch_case(cubec_allocator_t allocator,
                                            cubec_position_t *position,
                                            const char *end);
#ifdef __cplusplus
}
#endif
#endif