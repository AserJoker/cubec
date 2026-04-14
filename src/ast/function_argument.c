
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
cubec_ast_node_t cubec_read_ast_function_argument(cubec_allocator_t allocator,
                                                  cubec_position_t *position,
                                                  const char *end,
                                                  const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_FUNCTION_ARGUMENT);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t decorators =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "decorators", decorators);
  for (;;) {
    cubec_ast_node_t decorator =
        cubec_read_ast_decorator(allocator, &current, end, filename);
    if (!decorator) {
      break;
    }
    if (decorator->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_ast_add_item(decorators, decorator);
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end, filename);
  if (!identifier) {
    identifier =
        cubec_read_ast_literal_symbol(allocator, &current, end, filename);
    if (identifier && !cubec_location_is(identifier->loc, "...")) {
      current = identifier->loc.begin;
      cubec_allocator_free(allocator, identifier);
      identifier = NULL;
    }
  }
  if (!identifier) {
    goto onerror;
  }
  if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
    err = identifier;
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "identifier", identifier);
  if (identifier->type != CUBEC_NODE_TYPE_LITERAL_SYMBOL) {
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    if (*current.offset != ':') {
      err = cubec_create_ast_error(allocator, *position, current, filename,
                                   "invalid function argument, missing ':'");
      goto onerror;
    }
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_ast_node_t type =
        cubec_read_ast_expression18(allocator, &current, end, filename);
    if (!type) {
      err = cubec_create_ast_error(allocator, *position, current, filename,
                                   "invalid function argument, missing type");
      goto onerror;
    }
    if (type->type == CUBEC_NODE_TYPE_ERROR) {
      err = type;
      goto onerror;
    }
    cubec_ast_add_child(allocator, node, "type", type);
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