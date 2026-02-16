#ifndef _H_CUBEC_NODE_STATEMENT_DECLARATION_
#define _H_CUBEC_NODE_STATEMENT_DECLARATION_
#include "ast/node.h"
#include "core/list.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_statement_declaration_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t kind;
  cubec_list_t declarations;
} *cubec_ast_statement_declaration_t;
cubec_ast_statement_declaration_t
cubec_create_ast_statement_declaration(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_statement_declaration(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
#ifdef __cplusplus
}
#endif
#endif