#include "ast/statement_declaration.h"
#include "ast/literal_identifier.h"
#include "ast/literal_symbol.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/variable_declarator.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

cubec_ast_node_t
cubec_read_ast_statement_declaration(cubec_allocator_t allocator,
                                     cubec_position_t *position,
                                     const char *end, const char *filename) {
  cubec_ast_node_t node =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_DECLARATION);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t kind =
      cubec_read_ast_literal_identifier(allocator, &current, end, filename);
  if (!kind) {
    goto onerror;
  }
  if (kind->type == CUBEC_NODE_TYPE_ERROR) {
    err = kind;
    goto onerror;
  }
  if (!cubec_location_is(kind->loc, "const") &&
      !cubec_location_is(kind->loc, "extern") &&
      !cubec_location_is(kind->loc, "let") &&
      !cubec_location_is(kind->loc, "comptime") &&
      !cubec_location_is(kind->loc, "register") &&
      !cubec_location_is(kind->loc, "builtin")) {
    cubec_allocator_free(allocator, kind);
    goto onerror;
  }
  cubec_ast_add_child(allocator, node, "kind", kind);
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t declarations =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  for (;;) {
    cubec_ast_node_t item =
        cubec_read_ast_variable_declarator(allocator, &current, end, filename);
    if (!item) {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    if (item->type == CUBEC_NODE_TYPE_ERROR) {
      err = item;
      goto onerror;
    }
    cubec_ast_add_item(allocator, declarations, item);
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    if (*current.offset == ';') {
      break;
    }
    if (*current.offset != ',') {
      err = cubec_create_ast_error(allocator, *position, current,
                                   "Invalid or unexpected token");
      goto onerror;
    }
    current.offset++;
    current.column++;
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t token =
      cubec_read_ast_literal_symbol(allocator, &current, end, filename);
  if (token && token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!token || !cubec_location_is(token->loc, ";")) {
    cubec_allocator_free(allocator, token);
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid expression statement, missing ';'");
    goto onerror;
  }
  cubec_allocator_free(allocator, token);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}