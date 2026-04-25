#include "ast/expression_call.h"
#include "ast/expression.h"
#include "ast/expression_spread.h"
#include "ast/initialize_list.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
ast_node_t read_ast_expression_call(allocator_t allocator, position_t *position,
                                    const char *end, const char *filename) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  position_t current = *position;
  if (*current.offset != '(') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_CALL);
  ast_node_t args = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "arguments", args);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ')') {

    for (;;) {
      ast_node_t item =
          read_ast_expression_spread(allocator, &current, end, filename);
      if (!item) {
        item = read_ast_initialize_list(allocator, &current, end, filename);
      }
      if (!item) {
        item = read_ast_expression3(allocator, &current, end, filename);
      }
      if (!item) {
        err = create_ast_error(allocator, *position, current, filename,
                               "invalid or unexpected token");
        goto onerror;
      }
      if (item->type == NODE_TYPE_ERROR) {
        err = item;
        goto onerror;
      }
      ast_add_item(args, item);
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == NODE_TYPE_ERROR) {
        goto onerror;
      }
      if (*current.offset == ')') {
        break;
      }
      if (*current.offset != ',') {
        err = create_ast_error(allocator, *position, current, filename,
                               "invalid or unexpected token");
        goto onerror;
      }
      current.column++;
      current.offset++;
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == NODE_TYPE_ERROR) {
        goto onerror;
      }
    }
  }

  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ')') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid or unexpected token");
    goto onerror;
  }
  current.column++;
  current.offset++;
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;
  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}