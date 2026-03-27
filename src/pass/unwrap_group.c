#include "pass/unwrap_group.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/map.h"

cubec_ast_node_t cubec_pass_unwrap_group(cubec_allocator_t allocator,
                                         cubec_ast_node_t node, void *ctx) {
  if (node->type == CUBEC_NODE_TYPE_EXPRESSION_GROUP) {
    cubec_ast_node_t body = cubec_map_move(node->children, "body", NULL);
    body->parent = node->parent;
    return body;
  }
  return node;
}