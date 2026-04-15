#include "ast/enum_field.h"
#include "ast/decorator.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
ast_node_t read_ast_enum_field(allocator_t allocator, position_t *position,
                               const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_ENUM_FIELD);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t decorators = create_ast_node(allocator, NODE_TYPE_LIST);
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
  ast_node_t identifier =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!identifier) {
    goto onerror;
  }
  if (identifier->type == NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  ast_add_child(allocator, node, "identifier", identifier);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == '=') {
    current.offset++;
    current.column++;
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      return err;
    }
    ast_node_t value = read_ast_expression3(allocator, &current, end, filename);
    if (!value) {
      err = create_ast_error(allocator, *position, current, filename,
                             "invalid enum field, missing value");
      goto onerror;
    }
    if (value->type == NODE_TYPE_ERROR) {
      err = value;
      goto onerror;
    }
    ast_add_child(allocator, node, "value", value);
  } else {
    current = identifier->loc.end;
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