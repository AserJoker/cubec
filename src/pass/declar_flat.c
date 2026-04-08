#include "pass/declar_flat.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
cubec_ast_node_t cubec_pass_declar_flat(cubec_allocator_t allocator,
                                        cubec_ast_node_t node,
                                        cubec_context_t ctx) {
  if (node->type != CUBEC_NODE_TYPE_STATEMENT_DECLARATION) {
    return node;
  }
  cubec_ast_node_t declarations = cubec_ast_get_child(node, "declarations");
  if (cubec_ast_get_length(declarations) == 1) {
    return node;
  }
  cubec_ast_node_t parent = node->parent;
  size_t pos = cubec_ast_get_item_index(parent, node);
  node = cubec_ast_move_item(parent, pos);
  cubec_ast_node_t kind = cubec_ast_get_child(node, "kind");
  for (size_t idx = 0; idx < cubec_ast_get_length(declarations); idx++) {
    cubec_ast_node_t declar = cubec_ast_get_item(declarations, idx);
    declar = cubec_clone_ast_node(allocator, declar);
    cubec_ast_node_t sts =
        cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_DECLARATION);
    cubec_ast_node_t ckind = cubec_clone_ast_node(allocator, kind);
    cubec_ast_add_child(allocator, sts, "kind", ckind);
    cubec_ast_node_t declars =
        cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
    cubec_ast_add_item(declars, declar);
    cubec_ast_add_child(allocator, sts, "declarations", declars);
    cubec_ast_insert_item(parent, pos, sts);
    pos++;
  }
  cubec_allocator_free(allocator, node);
  return parent;
}