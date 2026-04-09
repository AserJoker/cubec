#include "pass/type_fix.h"
#include "ast/expression_group.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "engine/context.h"
cubec_ast_node_t cubec_pass_type_fix(cubec_allocator_t allocator,
                                     cubec_ast_node_t node,
                                     cubec_context_t ctx) {
  if (node->type == CUBEC_NODE_TYPE_VARIABLE_DECLARATOR) {
    cubec_ast_node_t initialize = cubec_ast_get_child(node, "initialize");
    if (initialize->type == CUBEC_NODE_TYPE_INITIALIZE_LIST) {
      cubec_ast_node_t itype = cubec_ast_get_child(initialize, "type");
      if (!itype) {
        cubec_ast_node_t type = cubec_ast_move_child(node, "type");
        if (!type) {
          return cubec_create_ast_error(allocator, node->loc.begin,
                                        node->loc.end, node->loc.filename,
                                        "Missing initialize list type");
        }
        cubec_ast_node_t realtype = cubec_ast_unwrap_group(type);
        if (realtype != type) {
          realtype = cubec_ast_move_child(realtype->parent, "expression");
          cubec_allocator_free(allocator, type);
          type = realtype;
        }
        cubec_ast_add_child(allocator, initialize, "type", type);
      }
    }
  }
  return node;
}