#include "ast/expression_template_generator.h"
#include "ast/expression.h"
#include "ast/expression_condition.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_expression_template_generator(allocator_t allocator,
                                                  position_t *position,
                                                  const char *end,
                                                  const char *filename) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  position_t current = *position;
  if (*current.offset != '@') {
    goto onerror;
  }
  current.column++;
  current.offset++;
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_TEMPLATE_GENERATOR);
  ast_node_t args = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "args", args);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t temp =
      read_ast_expression_value(allocator, &current, end, filename);
  if (!temp) {
    err =
        create_ast_error(allocator, *position, current, filename,
                         "invalid or unexpected template generator expression");
    goto onerror;
  }
  if (temp->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != '<') {
    err =
        create_ast_error(allocator, *position, current, filename,
                         "invalid or unexpected template generator expression");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != '>') {
    for (;;) {
      ast_node_t item =
          read_ast_expression_single(allocator, &current, end, filename);
      if (!item) {
        err = create_ast_error(
            allocator, *position, current, filename,
            "invalid or unexpected template generator expression");
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
      if (*current.offset == '>') {
        break;
      }
      if (*current.offset != ',') {
        err = create_ast_error(
            allocator, *position, current, filename,
            "invalid or unexpected template generator expression");
        goto onerror;
      }
      current.offset++;
      current.column++;
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
  if (*current.offset != '>') {
    err =
        create_ast_error(allocator, *position, current, filename,
                         "invalid or unexpected template generator expression");
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