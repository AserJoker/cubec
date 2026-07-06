#include "cubec/program.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/location.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/node.h"
#include "cubec/statement_empty.h"
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
  g_node_type.init(&self->super, allocator, &super_init);
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

node_t read_program_node(allocator_t allocator, vec_t tokens, size_t *position,
                         const char *filename) {
  cubec_program_node_t node =
      TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_program_node_type, NULL));
  size_t current = *position;
  TRY_VOID_LOCAL(onerror, skip_whitespace(tokens, &current));
  while (true) {
    TRY_VOID_LOCAL(onerror, skip_whitespace(tokens, &current));
    node_t statement = TRY_LOCAL(onerror,read_statement_empty(allocator, tokens, position, filename));
    if(!statement) {
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
  token_t begin = TRY_LOCAL(onerror, vec_get(tokens, *position));
  node->super.location.begin = token_get_location(begin)->begin;
  node->super.location.end = token_get_location(end)->begin;
  node->super.location.filename = filename;
  *position = current;
  return &node->super;
onerror:
  allocator_free(allocator, &node);
  return NULL;
}