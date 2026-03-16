#include "ast/expression_member.h"
#include "ast/literal_identifier.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
static void
cubec_ast_expression_member_dispose(cubec_ast_expression_member_t self,
                                    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->host);
  cubec_allocator_free(allocator, self->field);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_expression_member_t
cubec_create_ast_expression_member(cubec_allocator_t allocator) {
  cubec_ast_expression_member_t member = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_expression_member_t),
      (cubec_dispose_fn_t)cubec_ast_expression_member_dispose);
  cubec_ast_node_initialize(allocator, &member->super);
  member->super.type = CUBEC_NODE_TYPE_EXPRESSION_MEMBER;
  cubec_ast_set_field(member, allocator, host);
  cubec_ast_set_field(member, allocator, field);
  return member;
}

cubec_ast_node_t cubec_read_ast_expression_member(cubec_allocator_t allocator,
                                                  cubec_position_t *position,
                                                  const char *end) {
  cubec_ast_expression_member_t node = NULL;
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '.') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  node = cubec_create_ast_expression_member(allocator);
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_ast_node_t field =
      cubec_read_ast_literal_identifier(allocator, &current, end);
  if (!field) {
    err = cubec_create_ast_error(
        allocator, *position, current,
        "Invalid or unexpected token, missing field for member expression");
    goto onerror;
  }
  if (field->type == CUBEC_NODE_TYPE_ERROR) {
    err = field;
    goto onerror;
  }
  node->field = field;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->host, &node->super);
  cubec_ast_set_parent(node->field, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}
