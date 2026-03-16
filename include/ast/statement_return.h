#ifndef _H_CUBEC_AST_STATEMENT_RETURN_
#define _H_CUBEC_AST_STATEMENT_RETURN_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_statement_return_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t value;
} *cubec_ast_statement_return_t;
cubec_ast_statement_return_t
cubec_create_ast_statement_return(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_statement_return(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end);
#ifdef __cplusplus
}
#endif
#endif