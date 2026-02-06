#ifndef _H_CUBEC_NODE_IMPORT_DECLARATOR_
#define _H_CUBEC_NODE_IMPORT_DECLARATOR_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
typedef struct _cubec_ast_import_declarator {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t identifier;
  cubec_ast_node_t alias;
} *cubec_ast_import_declarator;

cubec_ast_import_declarator
cubec_create_ast_import_declarator(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_import_declarator(cubec_allocator_t allocator,
                                                  cubec_position_t *position,
                                                  cubec_position_t *end);
cubec_ast_node_t cubec_read_ast_import_namespace(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 cubec_position_t *end);
#endif