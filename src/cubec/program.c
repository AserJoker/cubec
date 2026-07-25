#include "cubec/program.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/ast_factory.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/node.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_declaration_type.h"
#include "cubec/statement_empty.h"
#include "cubec/statement_function.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_union.h"
#include "cubec/statement_import.h"
#include "cubec/statement_export_from.h"
#include "cubec/statement_test.h"
#include "cubec/statement_comptime.h"
#include "cubec/token.h"
#include <stdint.h>
#include "engine/context.h"

static void _cubec_program_node_init(cubec_program_node_t self,
                                     allocator_t allocator,
                                     cubec_program_node_init_t *init) {
  if (!init) return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_PROGRAM,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  if (init->statements) {
    self->statements = init->statements;
  } else {
    self->statements =
        allocator_create(allocator, &g_vec_type, &(vec_init_t){true});
  }
}
static void _cubec_program_node_dispose(cubec_program_node_t self,
                                        allocator_t allocator) {
  allocator_free(allocator, &self->statements);
  g_node_type.dispose(&self->super, allocator);
}
static void _cubec_program_node_clone(cubec_program_node_t self,
                                      allocator_t allocator,
                                      cubec_program_node_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->statements = value_clone(allocator, another->statements);
}
static void _cubec_program_node_move(cubec_program_node_t self,
                                     allocator_t allocator,
                                     cubec_program_node_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->statements = value_move(allocator, another->statements);
}
type_t g_cubec_program_node_type = {
    .name = "cubec.cubec.program_node",
    .size = sizeof(struct _cubec_program_node_t),
    .init = (type_init_fn_t)_cubec_program_node_init,
    .dispose = (type_dispose_fn_t)_cubec_program_node_dispose,
    .clone = (type_clone_fn_t)_cubec_program_node_clone,
    .move = (type_move_fn_t)_cubec_program_node_move,
};

node_t read_program_node(context_t ctx, vec_t tokens, size_t *position,
                         const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  skip_whitespace(tokens, &current);

  token_t begin = vec_get(tokens, current);
  if (!begin) goto onerror;
  cubec_program_node_init_t init = {
      .location = *token_get_location(begin),
      .parent = NULL,
  };
  cubec_program_node_t node =
      allocator_create(allocator, &g_cubec_program_node_type, &init);
  if (!node) goto onerror;

  while (true) {
    skip_whitespace(tokens, &current);

    /* Try statement_import (import ...) */
    node_t statement = read_statement_import(ctx, tokens, &current, filename);
    if (!statement) {
      /* Try statement_export_from (export * from / export { } from) */
      statement = read_statement_export_from(ctx, tokens, &current, filename);
    }
    if (!statement) {
      /* Try statement_declaration (var ...) */
      statement = read_statement_declaration(ctx, tokens, &current, filename);
    }
    if (!statement) {
      /* Try statement_declaration_type (type ...) */
      statement = read_statement_declaration_type(ctx, tokens, &current, filename);
    }
    if (!statement) {
      /* Try statement_function (func ... / export func ... / inline func ... / extern func ...) */
      statement = read_statement_function(ctx, tokens, &current, filename);
    }
    if (!statement) {
      /* Try statement_interface (interface ... / export interface ...) */
      statement = read_statement_interface(ctx, tokens, &current, filename);
    }
    if (!statement) {
      /* Try statement_struct (struct ... / export struct ...) */
      statement = read_statement_struct(ctx, tokens, &current, filename);
    }
    if (!statement) {
      /* Try statement_enum (enum ... / export enum ...) */
      statement = read_statement_enum(ctx, tokens, &current, filename);
    }
    if (!statement) {
      /* Try statement_cunion (cunion ...) */
      statement = read_statement_cunion(ctx, tokens, &current, filename);
    }
    if (!statement) {
      /* Try statement_union (union ... / export union ...) */
      statement = read_statement_union(ctx, tokens, &current, filename);
    }
    if (!statement) {
      /* Try statement_test (test "name" { }) */
      statement = read_statement_test(ctx, tokens, &current, filename);
    }
    if (!statement) {
      /* Try comptime block/if/for (comptime { } / comptime if / comptime for) */
      statement = read_statement_comptime(ctx, tokens, &current, filename);
    }
    if (!statement) {
      /* Try statement_empty (;) */
      statement = read_statement_empty(ctx, tokens, &current, filename);
    }
    if (!statement) {
      break;
    }
    vec_push(node->statements, statement);
  }
  skip_whitespace(tokens, &current);
  token_t end = vec_get(tokens, current);
  if (!end) goto onerror;
  if (token_get_kind(end) != CUBEC_TOKEN_EOF) {
    token_t token = vec_get(tokens, current);
    location_t *location = token_get_location(token);
    goto onerror;
  }
  node->super.location.begin = token_get_location(begin)->begin;
  node->super.location.end = token_get_location(end)->begin;
  node->super.location.filename = filename;
  *position = current;
  return &node->super;
onerror:
  allocator_free(allocator, &node);
  return NULL;
}

node_t cubec_ast_create_program(context_t ctx, location_t loc,
                                vec_t statements) {
  allocator_t alloc = ctx->allocator;
                                    cubec_program_node_init_t init = {
                                    .statements = statements};
  return (node_t)allocator_create(alloc, &g_cubec_program_node_type, &init);
}