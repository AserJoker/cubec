#include "cubec/program.h"
#include "cubec/ast_create_helpers.h"
#include "cubec/node_error.h"
#include "core/token.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include "cubec/statement_error.h"

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

    /* Check for EOF */
    token_t next = vec_get(tokens, current);
    if (!next || token_get_kind(next) == CUBEC_TOKEN_EOF) {
      break;
    }

    /* Delegate to read_statement for unified dispatch + error recovery */
    node_t statement = read_statement(ctx, tokens, &current, filename);

    if (node_is_error(statement)) {
      /* Error node — push as placeholder, recovery already done by read_statement */
      vec_push(node->statements, statement);
      continue;
    }

    if (!statement) {
      /* No parser matched — skip token and create error placeholder */
      location_t loc = *token_get_location(next);
      loc.filename = filename;
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, loc,
                           "unexpected token at top level");
      ctx->error_count++;
      current++;
      statement = cubec_ast_create_error_stmt(ctx, loc);
      vec_push(node->statements, statement);
      continue;
    }

    vec_push(node->statements, statement);
  }

  token_t end = vec_get(tokens, current);
  if (!end) goto onerror;
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
