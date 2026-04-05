#include "pass/type_fix.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include <string.h>

cubec_ast_node_t cubec_pass_type_fix(cubec_allocator_t allocator,
                                     cubec_ast_node_t node,
                                     cubec_context_t ctx) {
  if (node->type == CUBEC_NODE_TYPE_VARIABLE_DECLARATOR) {
    cubec_ast_node_t type = cubec_ast_get_child(node, "type");
    cubec_ast_node_t initialize = cubec_ast_get_child(node, "initialize");
    if (!type) {
      if (!initialize) {
        return cubec_create_ast_error(
            allocator, node->loc.begin, node->loc.end,
            "declaration type and initialize missing");
      }
      // TODO: eval initialize
      if (initialize->type == CUBEC_NODE_TYPE_LITERAL_NUMERIC) {
        char *type_str = cubec_context_create_cstring(ctx, "i32");
        type = cubec_create_ast_node(allocator,
                                     CUBEC_NODE_TYPE_LITERAL_IDENTIFIER);
        type->loc.begin.offset = type_str;
        type->loc.end.offset = type_str + strlen(type_str);
        cubec_ast_add_child(allocator, node, "type", type);
      }
    }
  }
  return node;
}