#ifndef _H_CUBEC_AST_TYPE_
#define _H_CUBEC_AST_TYPE_
#include "ast/node.h"
#include "core/allocator.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
cubec_ast_node_t cubec_read_ast_type(cubec_allocator_t allocator,
                                     cubec_position_t *position,
                                     const char *end, const char *filename);
#ifdef __cplusplus
}
#endif
#endif