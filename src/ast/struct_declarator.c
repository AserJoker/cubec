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

ast_node_t read_ast_struct_declarator(allocator_t allocator,
                                      position_t *position, const char *end,
                                      const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STRUCT_DECLARATOR);
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
  ast_node_t token =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!token) {
    goto onerror;
  }
  if (token->type == NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!location_is(token->loc, "struct") && !location_is(token->loc, "union")) {
    allocator_free(allocator, token);
    goto onerror;
  }
  ast_node_t fields = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "fields", fields);
  allocator_free(allocator, token);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t identifier =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (identifier) {
    if (identifier->type == NODE_TYPE_ERROR) {
      err = identifier;
      goto onerror;
    }
    ast_add_child(allocator, node, "identifier", identifier);
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '{') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid struct declarator, missing '{'");
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != '}') {
    for (;;) {
      ast_node_t item =
          read_ast_struct_declarator(allocator, &current, end, filename);
      if (item) {
        if (item->type == NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        ast_add_item(fields, item);
        goto next;
      }
      item = read_ast_enum_declarator(allocator, &current, end, filename);
      if (item) {
        if (item->type == NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        ast_add_item(fields, item);
        goto next;
      }
      item = read_ast_function_declarator(allocator, &current, end, filename);
      if (item) {
        if (item->type == NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        ast_add_item(fields, item);
        goto next;
      }
      item = read_ast_statement_declaration(allocator, &current, end, filename);
      if (item) {
        if (item->type == NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        ast_add_item(fields, item);
        goto next;
      }
      item = read_ast_struct_field(allocator, &current, end, filename);
      if (item) {
        if (item->type == NODE_TYPE_ERROR) {
          err = item;
          goto onerror;
        }
        ast_add_item(fields, item);
        current.offset++;
        current.column++;
        goto next;
      }
      err = create_ast_error(allocator, *position, current, filename,
                             "invalid struct field");
      goto onerror;
    next:
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
                           "invalid struct declarator, missing '}'");
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