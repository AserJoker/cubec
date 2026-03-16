#ifndef _H_CUBEC_AST_STATEMENT_TEST_
#define _H_CUBEC_AST_STATEMENT_TEST_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_statement_test_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t body;
  cubec_ast_node_t name;
} *cubec_ast_statement_test_t;
cubec_ast_statement_test_t
cubec_create_ast_statement_test(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_statement_test(cubec_allocator_t allocator,
                                               cubec_position_t *position,
                                               const char *end);
#ifdef __cplusplus
}
#endif
#endif