#include "ast/statement_block.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/position.h"
static void cubec_ast_statement_block_dispose(cubec_ast_statement_block_t self,
                                              cubec_allocator_t allocator) {}
cubec_ast_statement_block_t
cubec_create_ast_statement_block(cubec_allocator_t allocator) {
  cubec_ast_statement_block_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_ast_statement_block_t),
      (cubec_dispose_fn_t)cubec_ast_statement_block_dispose);
  cubec_ast_node_initialize(allocator, &self->super);
  self->super.type = CUBEC_NODE_TYPE_STATEMENT_BLOCK;
  cubec_list_initialize_t initialize = {
      .autofree = true,
  };
  self->statements = cubec_create_list(allocator, &initialize);
  return self;
}
cubec_ast_node_t cubec_read_ast_statement_block(cubec_allocator_t allocator,
                                                cubec_position_t *position,
                                                const char *end) {
  cubec_ast_statement_block_t node =
      cubec_create_ast_statement_block(allocator);
  cubec_ast_node_t err = NULL;
  cubec_position_t current = *position;
  if (*current.offset != '{') {
    goto onerror;
  }
  current.offset++;
  current.column++;
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  for (;;) {
    cubec_ast_node_t sts = cubec_read_ast_statement(allocator, &current, end);
    if (!sts) {
      break;
    }
    if (sts->type == CUBEC_NODE_TYPE_ERROR) {
      err = sts;
      goto onerror;
    }
    cubec_list_append(node->statements, allocator, sts);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
  }
  if (*current.offset != '}') {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid statement, missing '}'");
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