#include "cubec/statement_block.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/ast_factory.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/node.h"
#include "cubec/statement.h"
#include "cubec/token.h"

static void _cubec_statement_block_init(cubec_statement_block_t self,
                                         allocator_t allocator,
                                         cubec_statement_block_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_BLOCK,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->statements = init->statements;
onerror:
  return;
}

static void _cubec_statement_block_dispose(cubec_statement_block_t self,
                                            allocator_t allocator) {
  allocator_free(allocator, &self->statements);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_block_clone(cubec_statement_block_t self,
                                          allocator_t allocator,
                                          cubec_statement_block_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->statements = TRY_LOCAL(onerror, value_clone(allocator, another->statements));
  return;
onerror:
  return;
}

static void _cubec_statement_block_move(cubec_statement_block_t self,
                                         allocator_t allocator,
                                         cubec_statement_block_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->statements = TRY_LOCAL(onerror, value_move(allocator, another->statements));
  return;
onerror:
  return;
}

type_t g_cubec_statement_block_type = {
    .name = "cubec.cubec.statement_block",
    .size = sizeof(struct _cubec_statement_block_t),
    .init = (type_init_fn_t)_cubec_statement_block_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_block_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_block_clone,
    .move = (type_move_fn_t)_cubec_statement_block_move,
};

node_t read_statement_block(allocator_t allocator, vec_t tokens,
                            size_t *position, const char *filename) {
  size_t current = *position;

  /* Expect '{' */
  token_t lbrace = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(lbrace, CUBEC_TOKEN_SYMBOL, "{")) {
    return NULL;
  }
  current++;

  /* Create statements vec with auto_dispose */
  vec_t statements =
      TRY_LOCAL(onerror, allocator_create(allocator, &g_vec_type, &(vec_init_t){true}));

  /* Parse statements until '}' */
  while (true) {
    TRY_VOID_LOCAL(cleanup, skip_whitespace(tokens, &current));

    /* Check for '}' */
    token_t next = TRY_LOCAL(cleanup, vec_get(tokens, current));
    if (token_is(next, CUBEC_TOKEN_SYMBOL, "}")) {
      current++;
      break;
    }

    /* Try to parse a statement */
    node_t stmt = TRY_LOCAL(cleanup, read_statement(allocator, tokens, &current, filename));
    if (!stmt) {
      location_t *loc = token_get_location(next);
      THROW_LOCAL(cleanup,
                  "%s:%" PRIuPTR ":%" PRIuPTR " unexpected token in block",
                  filename, loc->begin.line + 1, loc->begin.column);
    }
    vec_push(statements, stmt);
  }

  /* Build location spanning from '{' to '}' */
  location_t *start_loc = token_get_location(lbrace);
  token_t rbrace = TRY_LOCAL(cleanup, vec_get(tokens, current - 1));
  location_t *end_loc = token_get_location(rbrace);
  location_t loc = {
      .begin = start_loc->begin,
      .end = end_loc->end,
      .filename = filename,
  };

  cubec_statement_block_init_t init = {
      .location = loc,
      .parent = NULL,
      .statements = statements,
  };
  cubec_statement_block_t node =
      TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_block_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &statements);
onerror:
  return NULL;
}

node_t cubec_ast_create_block(allocator_t alloc, location_t loc,
                              vec_t statements) {
  cubec_statement_block_init_t init = {.location = loc, .parent = NULL,
                                       .statements = statements};
  return (node_t)allocator_create(alloc, &g_cubec_statement_block_type,
                                  &init);
}
