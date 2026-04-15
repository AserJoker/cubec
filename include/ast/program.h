#ifndef _H_CUBEC_AST_TYPE_PROGRAM_
#define _H_CUBEC_AST_TYPE_PROGRAM_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_ast_program(allocator_t allocator, position_t *position,
                            const char *end, const char *filename);
#ifdef __cplusplus
}
#endif
#endif