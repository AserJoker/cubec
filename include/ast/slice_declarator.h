#ifndef _H_AST_SLICE_DECLARATOR_
#define _H_AST_SLICE_DECLARATOR_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

ast_node_t read_ast_slice_declarator(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename);

#ifdef __cplusplus
}
#endif
#endif