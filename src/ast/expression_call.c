#include "ast/expression_call.h"
#include "ast/expression.h"
#include "ast/expression_spread.h"
#include "ast/initialize_list.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
cubec_ast_node_t cubec_read_ast_expression_call(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end,
                                                const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_EXPRESSION_CALL);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t args =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "args", args);
  if (*current.offset != '(') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ')') {
    cubec_ast_node_t initialize_list =
        cubec_read_ast_initialize_list(allocator, &current, end, filename);
    if (initialize_list) {
      if (initialize_list->type == CUBEC_NODE_TYPE_ERROR) {
        err = initialize_list;
        goto onerror;
      }
      cubec_ast_add_item(args, initialize_list);
    } else {
      for (;;) {
        cubec_ast_node_t item = cubec_read_ast_expression_spread(
            allocator, &current, end, filename);
        if (!item) {
          item = cubec_read_ast_expression2(allocator, &current, end, filename);
        }
        if (!item) {
          err = cubec_create_ast_error(allocator, *position, current,
                                       "Invalid or unexpected token");
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
        if (*current.offset == ')') {
          break;
        }
        if (*current.offset != ',') {
          err = cubec_create_ast_error(allocator, *position, current,
                                       "Invalid or unexpected token");
          goto onerror;
        }
        current.column++;
        current.offset++;
        err = cubec_ast_skip_all(allocator, &current, end, filename);
        if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
          goto onerror;
        }
      }
    }
  }

  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  if (*current.offset != ')') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid or unexpected token");
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
  cubec_allocator_free(allocator, node);
  return err;
}