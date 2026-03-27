#ifndef _H_CUBEC_AST_STATEMENT_DEFER_
#define _H_CUBEC_AST_STATEMENT_DEFER_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
cubec_ast_node_t cubec_read_ast_statement_defer(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end);
#ifdef __cplusplus
}
#endif
#endif