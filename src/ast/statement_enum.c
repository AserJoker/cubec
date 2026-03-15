#include "ast/statement_enum.h"
#include "ast/enum_declarator.h"
#include "ast/node_type.h"

static void cubec_ast_statement_enum_dispose(cubec_ast_statement_enum_t self,
                                             cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->enu);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_statement_enum_t
cubec_create_ast_statement_enum(cubec_allocator_t allocator) {
  cubec_ast_statement_enum_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_enum_t),
      (cubec_dispose_fn_t)cubec_ast_statement_enum_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STATEMENT_ENUM;
  self->enu = NULL;
  return self;
}
cubec_ast_node_t cubec_read_ast_statement_enum(cubec_allocator_t allocator,
                                               cubec_position_t *position,
                                               const char *end) {
  cubec_ast_statement_enum_t node = cubec_create_ast_statement_enum(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t enu =
      cubec_read_ast_enum_declarator(allocator, &current, end);
  if (!enu) {
    goto onerror;
  }
  if (enu->type == CUBEC_NODE_TYPE_ERROR) {
    err = enu;
    goto onerror;
  }
  node->enu = enu;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == ';') {
    current.offset++;
    current.column++;
  } else {
    current = enu->loc.end;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}