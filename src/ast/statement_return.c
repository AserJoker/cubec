#include "ast/statement_return.h"
#include "ast/expression.h"
#include "ast/expression_member.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"

ast_node_t read_ast_statement_return(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename) {
  ast_node_t node =
      create_ast_node(allocator, CUBEC_NODE_TYPE_STATEMENT_RETURN);
  ast_node_t err = NULL;
  position_t current = *position;
  ast_node_t token =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!token) {
    goto onerror;
  }
  if (token->type == CUBEC_NODE_TYPE_ERROR) {
    err = token;
    goto onerror;
  }
  if (!location_is(token->loc, "return")) {
    allocator_free(allocator, token);
    goto onerror;
  }
  allocator_free(allocator, token);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  ast_node_t value =
      read_ast_expression_member(allocator, &current, end, filename);
  if (!value) {
    value = read_ast_expression(allocator, &current, end, filename);
  }
  if (value) {
    if (value->type == CUBEC_NODE_TYPE_ERROR) {
      err = value;
      goto onerror;
    }
    ast_add_child(allocator, node, "value", value);
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset != ';') {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid statement, missing ';'");
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