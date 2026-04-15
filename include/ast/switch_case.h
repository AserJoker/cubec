#ifndef _H_CUBEC_AST_SWITCH_CASE_
#define _H_CUBEC_AST_SWITCH_CASE_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_ast_switch_case(allocator_t allocator, position_t *position,
                                const char *end, const char *filename);
#ifdef __cplusplus
}
#endif
#endif