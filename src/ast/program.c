#include "ast/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_declaration.h"
#include "ast/statement_expression.h"
#include "ast/statement_import.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/position.h"

static void cubec_program_dispose(cubec_ast_program_t self,
                                  cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->statements);
}

cubec_ast_program_t cubec_create_ast_program(cubec_allocator_t allocator) {
  cubec_ast_program_t program =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ast_program_t),
                            (cubec_dispose_fn_t)cubec_program_dispose);
  cubec_ast_node_initialize(allocator, &program->super);
  program->super.type = CUBEC_NODE_TYPE_PROGRAM;
  cubec_list_initialize_t initialize = {.autofree = true};
  program->statements = cubec_create_list(allocator, &initialize);
  return program;
}

cubec_ast_node_t cubec_read_ast_program(cubec_allocator_t allocator,
                                        cubec_position_t *position,
                                        const char *end) {
  cubec_position_t current = *position;
  cubec_ast_program_t program = NULL;
  cubec_ast_node_t err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_allocator_free(allocator, err);
  program = cubec_create_ast_program(allocator);
  for (;;) {
    cubec_ast_node_t stat =
        cubec_read_ast_statement_import(allocator, &current, end);
    if (!stat) {
      stat = cubec_read_ast_statement_declaration(allocator, &current, end);
    }
    if (!stat) {
      stat = cubec_read_ast_statement_expression(allocator, &current, end);
    }
    if (!stat) {
      break;
    }
    if (stat->type == CUBEC_NODE_TYPE_ERROR) {
      err = stat;
      goto onerror;
    }
    cubec_list_append(program->statements, allocator, stat);
    err = cubec_ast_skip_all(allocator, &current, end);
    if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
      goto onerror;
    }
    cubec_allocator_free(allocator, err);
  }
  err = cubec_ast_skip_all(allocator, &current, end);
  if (err && err->type == CUBEC_NODE_TYPE_ERROR) {
    goto onerror;
  }
  cubec_allocator_free(allocator, err);
  if (*current.offset) {
    err = cubec_create_ast_error(allocator, *position, current,
                                 "Invalid or unexpected token");
    goto onerror;
  }
  program->super.loc.begin = *position;
  program->super.loc.end = current;
  *position = current;
  return &program->super;
onerror:
  cubec_allocator_free(allocator, program);
  return err;
}