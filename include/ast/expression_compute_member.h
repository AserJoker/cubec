#ifndef _H_CUBEC_AST_EXPRESSION_COMPUTE_MEMBER_
#define _H_CUBEC_AST_EXPRESSION_COMPUTE_MEMBER_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

ast_node_t read_ast_expression_compute_member(allocator_t allocator,
                                              position_t *position,
                                              const char *end,
                                              const char *filename);

#ifdef __cplusplus
}
#endif
#endif