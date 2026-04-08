#include "ast/struct_field.h"
#include "ast/decorator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/variable_declarator.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_struct_field(cubec_allocator_t allocator,
                                             cubec_position_t *position,
                                             const char *end,
                                             const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STRUCT_FIELD);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t decorators =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "decorators", decorators);
  for (;;) {
    cubec_ast_node_t decorator =
        cubec_read_ast_decorator(allocator, &current, end, filename);
    if (!decorator) {
      break;
    }
    if (decorator->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_ast_add_item(decorators, decorator);
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  cubec_ast_node_t declarator =
      cubec_read_ast_variable_declarator(allocator, &current, end, filename);
  if (!declarator) {
    goto onerror;
  }
  if (declarator->type == CUBEC_NODE_TYPE_ERROR) {
    err = declarator;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "declarator", declarator);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ';') {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "Invalid struct field");
    goto onerror;
  }
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}