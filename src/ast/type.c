#include "ast/type.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
static void cubec_ast_type_dispose(cubec_ast_type_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->expression);
  cubec_ast_node_dispose(allocator, &self->super);
}

cubec_ast_type_t cubec_create_ast_type(cubec_allocator_t allocator) {
  cubec_ast_type_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ast_type_t),
                            (cubec_dispose_fn_t)cubec_ast_type_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_TYPE;
  cubec_ast_set_field(self, allocator, expression);
  return self;
}
cubec_ast_node_t cubec_read_ast_type(cubec_allocator_t allocator,
                                     cubec_position_t *position,
                                     const char *end) {
  cubec_ast_type_t node = cubec_create_ast_type(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t expr = cubec_read_ast_expression18(allocator, &current, end);
  if (!expr) {
    goto onerror;
  }
  if (expr->type == CUBEC_NODE_TYPE_ERROR) {
    err = expr;
    goto onerror;
  }
  node->expression = expr;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->expression, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}