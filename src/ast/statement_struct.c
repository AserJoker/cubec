#include "ast/statement_struct.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/struct_declarator.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_statement_struct(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename) {
  ast_node_t node = create_ast_node(allocator, NODE_TYPE_STATEMENT_STRUCT);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t token =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (token) {
    if (token->type == NODE_TYPE_ERROR) {
      err = token;
      goto onerror;
    }
    if (location_is(token->loc, "pub")) {
      ast_add_child(allocator, node, "pub", token);
      err = ast_skip_all(allocator, &current, end, filename);
      if (err && err->type == NODE_TYPE_ERROR) {
        return err;
      }
    } else {
      current = token->loc.begin;
      allocator_free(allocator, token);
    }
  }
  ast_node_t stru =
      read_ast_struct_declarator(allocator, &current, end, filename);
  if (!stru) {
    goto onerror;
  }
  if (stru->type == NODE_TYPE_ERROR) {
    err = stru;
    goto onerror;
  }
  ast_add_child(allocator, node, "struct", stru);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == ';') {
    current.offset++;
    current.column++;
  } else {
    current = stru->loc.end;
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