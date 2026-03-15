#include "ast/statement_empty.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
static void cubec_ast_statement_empty_dispose(cubec_ast_statement_empty_t self,
                                              cubec_allocator_t allocator) {
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_statement_empty_t
cubec_create_ast_statement_empty(cubec_allocator_t allocator) {
  cubec_ast_statement_empty_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_empty_t),
      (cubec_dispose_fn_t)cubec_ast_statement_empty_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STATEMENT_EMPTY;
  return self;
}
cubec_ast_node_t cubec_read_ast_statement_empty(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end) {
  cubec_ast_statement_empty_t node =
      cubec_create_ast_statement_empty(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != ';') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}