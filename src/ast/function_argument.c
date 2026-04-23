
#include "ast/function_argument.h"
#include "ast/decorator.h"
#include "ast/expression.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
ast_node_t read_ast_function_argument(allocator_t allocator,
                                      position_t *position, const char *end,
                                      const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_FUNCTION_ARGUMENT);
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
  ast_node_t identifier =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!identifier) {
    identifier = read_ast_literal_symbol(allocator, &current, end, filename);
    if (identifier && !location_is(identifier->loc, "...")) {
      current = identifier->loc.begin;
      allocator_free(allocator, identifier);
      identifier = NULL;
    }
  }
  if (!identifier) {
    goto onerror;
  }
  if (identifier->type == NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  ast_add_child(allocator, node, "identifier", identifier);
  if (identifier->type != NODE_TYPE_LITERAL_SYMBOL) {
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      goto onerror;
    }
    if (*current.offset != ':') {
      err = create_ast_error(allocator, *position, current, filename,
                             "invalid function argument, missing ':'");
      goto onerror;
    }
    current.offset++;
    current.column++;
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      goto onerror;
    }
    ast_node_t type = read_ast_expression18(allocator, &current, end, filename);
    if (!type) {
      err = create_ast_error(allocator, *position, current, filename,
                             "invalid function argument, missing type");
      goto onerror;
    }
    if (type->type == NODE_TYPE_ERROR) {
      err = type;
      goto onerror;
    }
    ast_add_child(allocator, node, "type", type);
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