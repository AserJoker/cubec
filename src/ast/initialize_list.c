#include "ast/initialize_list.h"
#include "ast/expression.h"
#include "ast/expression_spread.h"
#include "ast/initialize_field.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
ast_node_t read_ast_initialize_list(allocator_t allocator, position_t *position,
                                    const char *end, const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_INITIALIZE_LIST);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t type = read_ast_expression18(allocator, &current, end, filename);
  if (type) {
    if (type->type == NODE_TYPE_ERROR) {
      err = type;
      goto onerror;
    }
    ast_add_child(allocator, node, "type", type);
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '{') {
    goto onerror;
  }
  current.offset++;
  current.column++;

  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t fields = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "fields", fields);
  if (*current.offset != '}') {
    for (;;) {
      ast_node_t item =
          read_ast_initialize_field(allocator, &current, end, filename);
      if (!item) {
        item = read_ast_expression_spread(allocator, &current, end, filename);
      }
      if (!item) {
        item = read_ast_expression3(allocator, &current, end, filename);
      }
      if (!item) {
        err = create_ast_error(allocator, *position, current, filename,
                               "invalid initialize list");
        goto onerror;
      }
      if (item->type == NODE_TYPE_ERROR) {
        err = item;
        goto onerror;
      }
      ast_add_item(fields, item);
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == '}') {
        break;
      }
      if (*current.offset != ',') {
        err = create_ast_error(allocator, *position, current, filename,
                               "invalid initialize list");
        goto onerror;
      }
      current.offset++;
      current.column++;
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == '}') {
        break;
      }
    }
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '}') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid initialize list");
    goto onerror;
  }
  current.offset++;
  current.column++;
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}