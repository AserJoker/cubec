#ifndef _H_CUBEC_NODE_STATEMENT_IMPORT_
#define _H_CUBEC_NODE_STATEMENT_IMPORT_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/position.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_statement_import_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t source;
  cubec_list_t declarators;
} *cubec_ast_statement_import_t;
cubec_ast_statement_import_t
cubec_create_ast_statement_import(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_statement_import(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 cubec_position_t *end);
#ifdef __cplusplus
}
#endif
#endif