#include "ast/expression_member.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_expression_member(allocator_t allocator,
                                      position_t *position, const char *end,
                                      const char *filename) {
  ast_node_t node = NULL;
  ast_node_t err = NULL;
  position_t current = *position;
  if (*current.offset != '.') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  node = create_ast_node(allocator, NODE_TYPE_EXPRESSION_MEMBER);
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  ast_node_t field =
      read_ast_literal_identifier(allocator, &current, end, filename);
  if (!field) {
    err = create_ast_error(
        allocator, *position, current, filename,
        "invalid or unexpected token, missing field for member expression");
    goto onerror;
  }
  if (field->type == NODE_TYPE_ERROR) {
    err = field;
    goto onerror;
  }
  ast_add_child(allocator, node, "field", field);
  node->loc.begin = *position;
  node->loc.end = current;
  node->loc.filename = filename;
  *position = current;

  return node;
onerror:
  allocator_free(allocator, node);
  return err;
}
