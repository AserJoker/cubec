#include "ast/expression_template_generator.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/type.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_expression_template_generator(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end,
    const char *filename) {
  cubec_ast_node_t node = cubec_create_ast_node(
      allocator, CUBEC_NODE_TYPE_EXPRESSION_TEMPLATE_GENERATOR);
  cubec_ast_node_t args =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "args", args);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '@') {
    goto onerror;
  }
  current.column++;
  current.offset++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t temp =
      cubec_read_ast_expression18(allocator, &current, end, filename);
  if (!temp) {
    err = cubec_create_ast_error(
        allocator, *position, current,
        "Invalid or unexpected template generator expression");
    goto onerror;
  }
  if (temp->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != '<') {
    err = cubec_create_ast_error(
        allocator, *position, current,
        "Invalid or unexpected template generator expression");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != '>') {
    for (;;) {
      cubec_ast_node_t item =
          cubec_read_ast_type(allocator, &current, end, filename);
      if (!item) {
        err = cubec_create_ast_error(
            allocator, *position, current,
            "Invalid or unexpected template generator expression");
        goto onerror;
      }
      if (item->type == CUBEC_NODE_TYPE_ERROR) {
        err = item;
        goto onerror;
      }
      cubec_ast_add_item(args, item);
      err = cubec_ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        goto onerror;
      }
      if (*current.offset == '>') {
        break;
      }
      if (*current.offset != ',') {
        err = cubec_create_ast_error(
            allocator, *position, current,
            "Invalid or unexpected template generator expression");
        goto onerror;
      }
      current.offset++;
      current.column++;
      err = cubec_ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        goto onerror;
      }
    }
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != '>') {
    err = cubec_create_ast_error(
        allocator, *position, current,
        "Invalid or unexpected template generator expression");
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