#include "cubec/program.h"
#include "core/allocator.h"
#include "core/error.h"
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
#include "cubec/token.h"
#include <stdint.h>

static void _cubec_program_node_init(cubec_program_node_t self,
                                     allocator_t allocator,
                                     cubec_program_node_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_PROGRAM,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->statements =
      TRY_LOCAL(onerror, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));
onerror:
  return;
}
static void _cubec_program_node_dispose(cubec_program_node_t self,
                                        allocator_t allocator) {
  allocator_free(allocator, &self->statements);
  g_node_type.dispose(&self->super, allocator);
}
static void _cubec_program_node_clone(cubec_program_node_t self,
                                      allocator_t allocator,
                                      cubec_program_node_t another) {
  TRY_VOID_LOCAL(cleanup, g_node_type.clone(&self->super, allocator, &another->super));
  self->statements = TRY_LOCAL(cleanup, value_clone(allocator, another->statements));
  return;

cleanup:
  allocator_free(allocator, &self->statements);
}
static void _cubec_program_node_move(cubec_program_node_t self,
                                     allocator_t allocator,
                                     cubec_program_node_t another) {
  TRY_VOID_LOCAL(cleanup, g_node_type.move(&self->super, allocator, &another->super));
  self->statements = TRY_LOCAL(cleanup, value_move(allocator, another->statements));
  return;

cleanup:
  allocator_free(allocator, &self->statements);
}
type_t g_cubec_program_node_type = {
    .name = "cubec.cubec.program_node",
    .size = sizeof(struct _cubec_program_node_t),
    .init = (type_init_fn_t)_cubec_program_node_init,
    .dispose = (type_dispose_fn_t)_cubec_program_node_dispose,
    .clone = (type_clone_fn_t)_cubec_program_node_clone,
    .move = (type_move_fn_t)_cubec_program_node_move,
};

node_t read_program_node(allocator_t allocator, vec_t tokens, size_t *position,
                         const char *filename) {
  size_t current = *position;
  TRY_VOID_LOCAL(onerror, skip_whitespace(tokens, &current));

  token_t begin = TRY_LOCAL(onerror, vec_get(tokens, current));
  cubec_program_node_init_t init = {
      .location = *token_get_location(begin),
      .parent = NULL,
  };
  cubec_program_node_t node =
      TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_program_node_type, &init));

  while (true) {
    TRY_VOID_LOCAL(onerror, skip_whitespace(tokens, &current));

    /* Try statement_import (import ...) */
    node_t statement = TRY_LOCAL(onerror, read_statement_import(allocator, tokens, &current, filename));
    if (!statement) {
      /* Try statement_declaration (var ...) */
      statement = TRY_LOCAL(onerror, read_statement_declaration(allocator, tokens, &current, filename));
    }
    if (!statement) {
      /* Try statement_declaration_type (type ...) */
      statement = TRY_LOCAL(onerror, read_statement_declaration_type(allocator, tokens, &current, filename));
    }
    if (!statement) {
      /* Try statement_function (func ... / export func ... / inline func ... / extern func ...) */
      statement = TRY_LOCAL(onerror, read_statement_function(allocator, tokens, &current, filename));
    }
    if (!statement) {
      /* Try statement_interface (interface ... / export interface ...) */
      statement = TRY_LOCAL(onerror, read_statement_interface(allocator, tokens, &current, filename));
    }
    if (!statement) {
      /* Try statement_struct (struct ... / export struct ...) */
      statement = TRY_LOCAL(onerror, read_statement_struct(allocator, tokens, &current, filename));
    }
    if (!statement) {
      /* Try statement_enum (enum ... / export enum ...) */
      statement = TRY_LOCAL(onerror, read_statement_enum(allocator, tokens, &current, filename));
    }
    if (!statement) {
      /* Try statement_cunion (cunion ...) */
      statement = TRY_LOCAL(onerror, read_statement_cunion(allocator, tokens, &current, filename));
    }
    if (!statement) {
      /* Try statement_union (union ... / export union ...) */
      statement = TRY_LOCAL(onerror, read_statement_union(allocator, tokens, &current, filename));
    }
    if (!statement) {
      /* Try statement_empty (;) */
      statement = TRY_LOCAL(onerror, read_statement_empty(allocator, tokens, &current, filename));
    }
    if (!statement) {
      break;
    }
    vec_push(node->statements, statement);
  }
  TRY_VOID_LOCAL(onerror, skip_whitespace(tokens, &current));
  token_t end = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (token_get_kind(end) != CUBEC_TOKEN_EOF) {
    token_t token = TRY_LOCAL(onerror, vec_get(tokens, current));
    location_t *location = token_get_location(token);
    THROW_LOCAL(onerror,
                "%s:%" PRIuPTR ":%" PRIuPTR " invalid or unexpected token",
                filename, location->begin.line + 1, location->begin.column);
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