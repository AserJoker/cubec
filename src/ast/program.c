#include "ast/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement.h"
#include "core/allocator.h"
#include "core/position.h"

ast_node_t read_ast_program(allocator_t allocator, position_t *position,
                            const char *end, const char *filename) {
  position_t current = *position;
  ast_node_t program = NULL;
  ast_node_t err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  allocator_free(allocator, err);
  program = create_ast_node(allocator, NODE_TYPE_PROGRAM);
  ast_node_t statements = create_ast_node(allocator, NODE_TYPE_LIST);
  ast_add_child(allocator, program, "statements", statements);
  for (;;) {
    ast_node_t stat = read_ast_statement(allocator, &current, end, filename);
    if (!stat) {
      break;
    }
    if (stat->type == NODE_TYPE_ERROR) {
      err = stat;
      goto onerror;
    }
    ast_add_item(statements, stat);
    err = ast_skip_all(allocator, &current, end, filename);
    if (err && err->type == NODE_TYPE_ERROR) {
      goto onerror;
    }
    if (!*current.offset) {
      break;
    }
  }
  err = ast_skip_all(allocator, &current, end, filename);
  if (err && err->type == NODE_TYPE_ERROR) {
    goto onerror;
  }
  allocator_free(allocator, err);
  if (*current.offset) {
    err = create_ast_error(allocator, *position, current, filename,
                           "invalid or unexpected token");
    goto onerror;
  }
  program->loc.begin = *position;
  program->loc.end = current;
  program->loc.filename = filename;
  *position = current;
  return program;
onerror:
  allocator_free(allocator, program);
  return err;
}