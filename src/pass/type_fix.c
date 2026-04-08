#include "pass/type_fix.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include <string.h>
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
        cubec_ast_add_child(allocator, initialize, "type", type);
      }
    }
    if (initialize->type == CUBEC_NODE_TYPE_LITERAL_NUMERIC) {
      cubec_ast_node_t type = cubec_ast_move_child(node, "type");
      if (!type) {
        bool is_floating = false;
        for (const char *ch = initialize->loc.begin.offset;
             ch != initialize->loc.end.offset; ch++) {
          if (*ch == 'e' || *ch == 'E' || *ch == '.') {
            is_floating = true;
            break;
          }
        }
        type = cubec_create_ast_node(allocator,
                                     CUBEC_NODE_TYPE_LITERAL_IDENTIFIER);
        char *s = NULL;
        if (is_floating) {
          s = cubec_context_create_cstring(ctx, "f64");
        } else {
          s = cubec_context_create_cstring(ctx, "i32");
        }
        type->loc.begin.offset = s;
        type->loc.end.offset = s + strlen(s);
      }
      initialize = cubec_ast_move_child(node, "initialize");
      cubec_ast_node_t initialize_list =
          cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_INITIALIZE_LIST);
      cubec_ast_node_t fields =
          cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
      cubec_ast_node_t initialize_field =
          cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_INITIALIZE_FIELD);
      cubec_ast_add_child(allocator, initialize_list, "type", type);
      cubec_ast_add_child(allocator, initialize_list, "fields", fields);
      cubec_ast_add_item(fields, initialize_field);
      cubec_ast_add_child(allocator, initialize_field, "initialize",
                          initialize);
      cubec_ast_add_child(allocator, node, "initialize", initialize_list);
    }
  }
  return node;
}