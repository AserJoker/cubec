#include "ast/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/position.h"

cubec_ast_node_t cubec_read_ast_program(cubec_allocator_t allocator,
                                        cubec_position_t *position,
                                        const char *end, const char *filename) {
  cubec_position_t current = *position;
  cubec_ast_node_t program = NULL;
  cubec_ast_node_t err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_allocator_free(allocator, err);
  program = cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_PROGRAM);
  cubec_ast_node_t statements =
      cubec_create_ast_node(allocator, CUBEC_NODE_TYPE_LIST);
  cubec_ast_add_child(allocator, program, "statements", statements);
  for (;;) {
    cubec_ast_node_t stat =
        cubec_read_ast_statement(allocator, &current, end, filename);
    if (!stat) {
      break;
    }
    if (stat->type == CUBEC_NODE_TYPE_ERROR) {
      err = stat;
      goto onerror;
    }
    cubec_ast_add_item(statements, stat);
    err = cubec_ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
  }
  err = cubec_ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_allocator_free(allocator, err);
  if (*current.offset) {
    err = cubec_create_ast_error(allocator, *position, current, filename,
                                 "invalid or unexpected token");
    goto onerror;
  }
  program->loc.begin = *position;
  program->loc.end = current;
  *position = current;
  return program;
onerror:
  cubec_allocator_free(allocator, program);
  return err;
}