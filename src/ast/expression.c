#include "ast/expression.h"
#include "ast/array_declarator.h"
#include "ast/callable_declarator.h"
#include "ast/enum_declarator.h"
#include "ast/expression_call.h"
#include "ast/expression_comma.h"
#include "ast/expression_compute_member.h"
#include "ast/expression_group.h"
#include "ast/expression_member.h"
#include "ast/expression_slice.h"
#include "ast/expression_template_generator.h"
#include "ast/function_declarator.h"
#include "ast/initialize_list.h"
#include "ast/literal_char.h"
#include "ast/literal_identifier.h"
#include "ast/literal_numeric.h"
#include "ast/literal_string.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/ptr_declarator.h"
#include "ast/slice_declarator.h"
#include "ast/struct_declarator.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_expression(allocator_t allocator, position_t *position,
                               const char *end, const char *filename) {
  return read_ast_expression_comma(allocator, position, end, filename);
}

ast_node_t read_ast_expression_value(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename) {
  ast_node_t node = NULL;
  position_t current = *position;
  node = read_ast_expression_atom(allocator, &current, end, filename);
  if (node) {
    if (node->type == NODE_TYPE_ERROR) {
      return node;
    }
    for (;;) {
      position_t curr = current;
      ast_node_t err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == NODE_TYPE_ERROR) {
        return err;
      }
      ast_node_t next = NULL;
      if (!next) {
        next = read_ast_expression_slice(allocator, &current, end, filename);
      }
      if (!next) {
        next = read_ast_expression_member(allocator, &current, end, filename);
      }
      if (!next) {
        next = read_ast_expression_compute_member(allocator, &current, end,
                                                  filename);
      }
      if (!next) {
        next = read_ast_expression_call(allocator, &current, end, filename);
      }
      if (next) {
        if (next->type == NODE_TYPE_ERROR) {
          allocator_free(allocator, node);
          next->loc.begin = *position;
          return next;
        }
        if (next->type == NODE_TYPE_EXPRESSION_MEMBER) {
          ast_add_child(allocator, next, "host", node);
          node = next;
          next->loc.begin = *position;

        } else if (next->type == NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
          ast_add_child(allocator, next, "host", node);
          node = next;
          next->loc.begin = *position;
        } else if (next->type == NODE_TYPE_EXPRESSION_CALL) {
          ast_add_child(allocator, next, "callee", node);
          node = next;
          next->loc.begin = *position;

        } else if (next->type == NODE_TYPE_EXPRESSION_SLICE) {
          ast_add_child(allocator, next, "host", node);
          node = next;
          next->loc.begin = *position;
        }
      } else {
        current = curr;
        break;
      }
    }
  }
  *position = current;
  return node;
}
ast_node_t read_ast_expression_atom(allocator_t allocator, position_t *position,
                                    const char *end, const char *filename) {
  ast_node_t node = NULL;
  node = read_ast_initialize_list(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_ptr_declarator(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_expression_member(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_slice_declarator(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_array_declarator(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_struct_declarator(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_expression_group(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_expression_template_generator(allocator, position, end,
                                                filename);
  if (node) {
    return node;
  }
  node = read_ast_callable_declarator(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_function_declarator(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_enum_declarator(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_literal_numeric(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_literal_char(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_literal_string(allocator, position, end, filename);
  if (node) {
    return node;
  }
  node = read_ast_literal_identifier(allocator, position, end, filename);
  if (node) {
    return node;
  }
  return NULL;
}