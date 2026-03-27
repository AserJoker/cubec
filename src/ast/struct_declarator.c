#include "ast/struct_declarator.h"
#include "ast/decorator.h"
#include "ast/enum_declarator.h"
#include "ast/function_declarator.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_declaration.h"
#include "ast/struct_field.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_struct_declarator(cubec_allocator_t allocator,
                                                  cubec_position_t *position,
                                                  const char *end) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STRUCT_DECLARATOR);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t decorators =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "decorators", decorators);
  cubec_ast_node_t fields =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "fields", fields);
  cubec_ast_node_t methods =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "methods", methods);
  cubec_ast_node_t attributes =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, node, "attributes", attributes);
  for (;;) {
    cubec_ast_node_t decorator =
        cubec_read_ast_decorator(allocator, &current, end);
    if (!decorator) {
      break;
    }
    if (decorator->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_ast_add_item(allocator, decorators, decorator);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      return err;
    }
  }
  cubec_ast_node_t token =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!token) {
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!cubec_location_is(token->loc, "struct") &&
      !cubec_location_is(token->loc, "union")) {
    cubec_allocator_free(allocator, token);
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  cubec_ast_node_t identifier =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (identifier) {
    if (identifier->type == CUBEC_NODE_TYPE_ERROR) {
      err = identifier;
      goto onerror;
    }
    cubec_ast_add_child(allocator, node, "identifier", identifier);
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '{') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid struct declarator, missing '{'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '}') {
    for (;;) {
      cubec_ast_node_t item =
          cubec_read_ast_struct_declarator(allocator, &current, end);
      if (item) {
        if (item->type == CUBEC_NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        cubec_ast_add_item(allocator, attributes, item);
        goto next;
      }
      item = cubec_read_ast_enum_declarator(allocator, &current, end);
      if (item) {
        if (item->type == CUBEC_NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        cubec_ast_add_item(allocator, attributes, item);
        goto next;
      }
      item = cubec_read_ast_function_declarator(allocator, &current, end);
      if (item) {
        if (item->type == CUBEC_NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        cubec_ast_add_item(allocator, methods, item);
        goto next;
      }
      item = cubec_read_ast_statement_declaration(allocator, &current, end);
      if (item) {
        if (item->type == CUBEC_NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        cubec_ast_add_item(allocator, attributes, item);
        goto next;
      }
      item = cubec_read_ast_struct_field(allocator, &current, end);
      if (item) {
        if (item->type == CUBEC_NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        cubec_ast_add_item(allocator, fields, item);
        current.offset++;
        current.column++;
        goto next;
      }
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid struct field");
      goto onerror;
    next:
      err = cubec_ast_skip_all(allocator, &current, end);
      if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
        return err;
      }
      if (*current.offset == '}') {
        break;
      }
    }
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '}') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid struct declarator, missing '}'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  node->loc.begin = *position;
  node->loc.end = current;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}