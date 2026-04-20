#include "ast/struct_field.h"
#include "ast/decorator.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/variable_declarator.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_struct_field(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STRUCT_FIELD);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t decorators = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "decorators", decorators);
  for (;;) {
    ast_node_t decorator =
        read_ast_decorator(allocator, &current, end, filename);
    if (!decorator) {
      break;
    }
    if (decorator->type == NODE_TYPE_ERROR) {
      goto onerror;
    }
    ast_add_item(decorators, decorator);
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      return err;
    }
  }
  ast_node_t mut =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (mut) {
    if (mut->type == NODE_TYPE_ERROR) {
      err = mut;
      goto onerror;
    }
    if (location_is(mut->loc, "const")) {
      ast_add_child(allocator, node, "mut", mut);
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == NODE_TYPE_ERROR) {
        return err;
      }
    } else {
      current = mut->loc.begin;
      allocator_free(allocator, mut);
    }
  }
  ast_node_t declarator =
      read_ast_variable_declarator(allocator, &current, end, filename);
  if (!declarator) {
    goto onerror;
  }
  if (declarator->type == NODE_TYPE_ERROR) {
    err = declarator;
    goto onerror;
  }
  ast_add_child(allocator, node, "declarator", declarator);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ';') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid struct field");
    goto onerror;
  }
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}