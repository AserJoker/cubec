#include "ast/expression.h"
#include "ast/array_declarator.h"
#include "ast/callable_declarator.h"
#include "ast/enum_declarator.h"
#include "ast/expression_assigment.h"
#include "ast/expression_call.h"
#include "ast/expression_comma.h"
#include "ast/expression_generics.h"
#include "ast/expression_group.h"
#include "ast/expression_member.h"
#include "ast/expression_slice.h"
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

ast_node_t read_expression(allocator_t allocator, token_stream_t stream) {
  return read_expression_comma(allocator, stream);
}
ast_node_t read_expression_single(allocator_t allocator,
                                  token_stream_t stream) {
  return read_expression_assigment(allocator, stream);
}

ast_node_t read_expression_value(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  size_t position = stream->position;
  node = read_expression_atom(allocator, stream);
  if (node) {
    if (node->type == NODE_TYPE_ERROR) {
      return node;
    }
    for (;;) {
      size_t curr = stream->position;
      skip_comments(stream);
      ast_node_t next = NULL;
      if (!next) {
        next = read_expression_slice(allocator, stream);
      }
      if (!next) {
        next = read_expression_member(allocator, stream);
      }
      if (!next) {
        next = read_expression_call(allocator, stream);
      }
      if (!next) {
        next = read_expression_generics(allocator, stream);
      }
      if (next) {
        if (next->type == NODE_TYPE_ERROR) {
          allocator_free(allocator, node);
          next->start = position;
          err = next;
          goto onerror;
        }
        if (next->type == NODE_TYPE_EXPRESSION_MEMBER) {
          ast_add_child(allocator, next, "host", node);
          node = next;
          next->start = position;
        } else if (next->type == NODE_TYPE_EXPRESSION_GENERICS) {
          ast_add_child(allocator, next, "host", node);
          node = next;
          next->start = position;
        } else if (next->type == NODE_TYPE_EXPRESSION_CALL) {
          ast_add_child(allocator, next, "callee", node);
          node = next;
          next->start = position;
        } else if (next->type == NODE_TYPE_EXPRESSION_SLICE) {
          ast_add_child(allocator, next, "host", node);
          node = next;
          next->start = position;
        }
      } else {
        stream->position = curr;
        break;
      }
    }
  }
  return node;
onerror:
  stream->position = position;
  allocator_free(allocator, node);
  return err;
}
ast_node_t read_expression_atom(allocator_t allocator, token_stream_t stream) {
  ast_node_t node = read_literal_string(allocator, stream);
  if (node) {
    return node;
  }
  node = read_expression_group(allocator, stream);
  if (node) {
    return node;
  }
  node = read_literal_identifier(allocator, stream);
  if (node) {
    return node;
  }
  node = read_literal_numeric(allocator, stream);
  if (node) {
    return node;
  }
  node = read_literal_char(allocator, stream);
  if (node) {
    return node;
  }
  node = read_initialize_list(allocator, stream);
  if (node) {
    return node;
  }
  node = read_callable_declarator(allocator, stream);
  if (node) {
    return node;
  }
  node = read_function_declarator(allocator, stream);
  if (node) {
    return node;
  }
  node = read_struct_declarator(allocator, stream);
  if (node) {
    return node;
  }
  node = read_enum_declarator(allocator, stream);
  if (node) {
    return node;
  }
  node = read_ptr_declarator(allocator, stream);
  if (node) {
    return node;
  }
  node = read_slice_declarator(allocator, stream);
  if (node) {
    return node;
  }
  node = read_array_declarator(allocator, stream);
  if (node) {
    return node;
  }
  return node;
}