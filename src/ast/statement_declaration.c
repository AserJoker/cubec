#include "ast/statement_declaration.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/variable_declarator.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_statement_declaration(allocator_t allocator,
                                          position_t *position, const char *end,
                                          const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_DECLARATION);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t kind =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!kind) {
    goto onerror;
  }
  if (kind->type == NODE_TYPE_ERROR) {
    err = kind;
    goto onerror;
  }
  if (!location_is(kind->loc, "extern") &&
      !location_is(kind->loc, "comptime") &&
      !location_is(kind->loc, "register")) {
    current = kind->loc.begin;
    allocator_free(allocator, kind);
  } else {
    ast_add_child(allocator, node, "kind", kind);
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t type =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!type) {
    goto onerror;
  }
  if (type->type == NODE_TYPE_ERROR) {
    err = type;
    goto onerror;
  }
  if (!location_is(type->loc, "const") && !location_is(type->loc, "let")) {
    allocator_free(allocator, type);
    goto onerror;
  }
  ast_add_child(allocator, node, "type", type);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t declarations = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, node, "declarations", declarations);
  for (;;) {
    ast_node_t item =
        read_ast_variable_declarator(allocator, &current, end, filename);
    if (!item) {
      err = create_ast_error(allocator, *position, current, filename,
                             "invalid or unexpected token");
      goto onerror;
    }
    if (item->type == NODE_TYPE_ERROR) {
      err = item;
      goto onerror;
    }
    ast_add_item(declarations, item);
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      goto onerror;
    }
    if (*current.offset == ';') {
      break;
    }
    if (*current.offset != ',') {
      err = create_ast_error(allocator, *position, current, filename,
                             "invalid or unexpected token");
      goto onerror;
    }
    current.offset++;
    current.column++;
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      goto onerror;
    }
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t token =
      read_ast_literal_symbol(allocator, &current, end, filename);
  if (token && token->type == NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!token || !location_is(token->loc, ";")) {
    allocator_free(allocator, token);
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid expression statement, missing ';'");
    goto onerror;
  }
  allocator_free(allocator, token);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}