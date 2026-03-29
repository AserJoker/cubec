#ifndef _H_CUBEC_AST_STATEMENT_TEST_
#define _H_CUBEC_AST_STATEMENT_TEST_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
cubec_ast_node_t cubec_read_ast_statement_test(cubec_allocator_t allocator,
                                               cubec_position_t *position,
                                               const char *end,
                                               const char *filename);
#ifdef __cplusplus
}
#endif
#endif