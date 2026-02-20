#include "ast/statement_struct.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/struct_declarator.h"
#include "core/allocator.h"
#include "core/position.h"
static void
cubec_ast_statement_struct_dispose(cubec_ast_statement_struct_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->stru);
}
cubec_ast_statement_struct_t
cubec_create_ast_statement_struct(cubec_allocator_t allocator) {
  cubec_ast_statement_struct_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_struct_t),
      (cubec_dispose_fn_t)cubec_ast_statement_struct_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STATEMENT_STRUCT;
  self->stru = NULL;
  return self;
}
cubec_ast_node_t cubec_read_ast_statement_struct(cubec_allocator_t allocator,
                                                 cubec_position_t *position,
                                                 const char *end) {
  cubec_ast_statement_struct_t node =
      cubec_create_ast_statement_struct(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t stru =
      cubec_read_ast_struct_declarator(allocator, &current, end);
  if (!stru) {
    goto onerror;
  }
  if (stru->type == CUBEC_NODE_TYPE_ERROR) {
    err = stru;
    goto onerror;
  }
  node->stru = stru;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == ';') {
    current.offset++;
    current.column++;
  } else {
    current = stru->loc.end;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}