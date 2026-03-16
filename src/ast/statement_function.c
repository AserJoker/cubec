#include "ast/statement_function.h"
#include "ast/function_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/position.h"
static void
cubec_ast_statement_function_dispose(cubec_ast_statement_function_t self,
                                     cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->function);
  cubec_ast_node_dispose(allocator, &self->super);
}
cubec_ast_statement_function_t
cubec_create_ast_statement_function(cubec_allocator_t allocator) {
  cubec_ast_statement_function_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_function_t),
      (cubec_dispose_fn_t)cubec_ast_statement_function_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STATEMENT_FUNCTION;
  cubec_ast_set_field(self, allocator, function);
  return self;
}
cubec_ast_node_t cubec_read_ast_statement_function(cubec_allocator_t allocator,
                                                   cubec_position_t *position,
                                                   const char *end) {
  cubec_ast_statement_function_t node =
      cubec_create_ast_statement_function(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  cubec_ast_node_t function =
      cubec_read_ast_function_declarator(allocator, &current, end);
  if (!function) {
    goto onerror;
  }
  if (function->type == CUBEC_NODE_TYPE_ERROR) {
    err = function;
    goto onerror;
  }
  node->function = function;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    return err;
  }
  if (*current.offset == ';') {
    current.offset++;
    current.column++;
  } else {
    current = function->loc.end;
  }
  node->super.loc.begin = *position;
  node->super.loc.end = current;
  *position = current;
  cubec_ast_set_parent(node->function, &node->super);
  return &node->super;
onerror:
  cubec_allocator_free(allocator, node);
  return err;
}