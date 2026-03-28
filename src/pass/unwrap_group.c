#include "pass/unwrap_group.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/context.h"

cubec_ast_node_t cubec_visit_unwrap_group(cubec_allocator_t allocator,
                                          cubec_ast_node_t node,
                                          cubec_context_t ctx) {
  if (node->type == CUBEC_NODE_TYPE_EXPRESSION_GROUP) {
    cubec_ast_node_t parent = node->parent;
    cubec_ast_node_t body = cubec_ast_move_child(allocator, node, "body");
    if (parent->type == CUBEC_NODE_TYPE_LIST) {
      size_t idx = cubec_ast_get_item_index(parent, node);
      cubec_ast_set_item(allocator, parent, idx, body);
    } else {
      const char *name = cubec_ast_get_child_name(parent, node);
      cubec_ast_set_child(allocator, parent, name, body);
    }
    return parent;
  }
  return node;
}