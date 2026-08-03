#include "cubec/statement_block.h"
#include "core/token.h"
#include "core/vec.h"
#include "core/writer.h"
#include "cubec/node_error.h"
#include "cubec/statement.h"
#include "cubec/statement_error.h"
#include "cubec/token.h"

static void _cubec_statement_block_init(cubec_statement_block_t self,
                                        allocator_t allocator,
                                        cubec_statement_block_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_BLOCK,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->statements = init->statements;
}

static void _cubec_statement_block_dispose(cubec_statement_block_t self,
                                           allocator_t allocator) {
  allocator_free(allocator, &self->statements);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_block_clone(cubec_statement_block_t self,
                                         allocator_t allocator,
                                         cubec_statement_block_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->statements = value_clone(allocator, another->statements);
  return;
}

static void _cubec_statement_block_move(cubec_statement_block_t self,
                                        allocator_t allocator,
                                        cubec_statement_block_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->statements = value_move(allocator, another->statements);
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

node_t read_statement_block(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;

  /* Expect '{' */
  token_t lbrace = vec_get(tokens, current);
  if (!token_is(lbrace, CUBEC_TOKEN_SYMBOL, "{")) {
    return NULL;
  }
  current++;

  /* Create statements vec with auto_dispose */
  vec_t statements =
      allocator_create(allocator, &g_vec_type, &(vec_init_t){true});

  /* Parse statements until '}' */
  while (true) {
    skip_whitespace(tokens, &current);
    size_t before = current;

    /* Check for '}' */
    token_t next = vec_get(tokens, current);
    if (token_is(next, CUBEC_TOKEN_SYMBOL, "}")) {
      current++;
      break;
    }

    /* Check for EOF (unterminated block) */
    if (next && token_get_kind(next) == CUBEC_TOKEN_EOF) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           *token_get_location(lbrace),
                           "unterminated block (missing '}')");
      break;
    }

    /* Try to parse a statement */
    node_t stmt = read_statement(ctx, tokens, &current, filename);

    /* Error node (CUBEC_NODE_ERROR or CUBEC_NODE_STATEMENT_ERROR) — push and
     * continue */
    if (node_is_error(stmt)) {
      vec_push(statements, stmt);
      if (current <= before)
        current = before + 1;
      continue;
    }

    /* NULL — no parser matched (unrecognized token) */
    if (!stmt) {
      /* Record diagnostic, skip the bad token, create StatementError */
      token_t bad = vec_get(tokens, current);
      if (bad) {
        location_t loc = *token_get_location(bad);
        loc.filename = filename;
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, loc,
                             "unexpected token");
        current++;
        stmt = create_statement_error(ctx, loc);
        vec_push(statements, stmt);
      } else {
        break; /* No more tokens */
      }
      continue;
    }

    vec_push(statements, stmt);
  }

  /* Build location spanning from '{' to current position */
  location_t *start_loc = token_get_location(lbrace);
  location_t loc = {
      .begin = start_loc->begin,
      .end = start_loc->begin, /* fallback */
      .filename = filename,
  };
  /* Try to get end from the '}' or last consumed token */
  if (current > 0) {
    token_t end_tok = vec_get(tokens, current - 1);
    if (end_tok) {
      loc.end = token_get_location(end_tok)->end;
    }
  }

  cubec_statement_block_init_t init = {
      .location = loc,
      .parent = NULL,
      .statements = statements,
  };
  cubec_statement_block_t node =
      allocator_create(allocator, &g_cubec_statement_block_type, &init);
  *position = current;
  return &node->super;
}

node_t create_statement_block(context_t ctx, location_t loc, vec_t statements) {
  allocator_t alloc = ctx->allocator;
  cubec_statement_block_init_t init = {.statements = statements};
  return (node_t)allocator_create(alloc, &g_cubec_statement_block_type, &init);
}

void write_statement_block(writer_t writer, node_t stmt) {
  cubec_statement_block_t block = (cubec_statement_block_t)stmt;
  writer_append(writer, "{");
  if (vec_get_size(block->statements)) {
    writer_newline(writer, 1);
    for (size_t i = 0; i < vec_get_size(block->statements); i++) {
      node_t stmt = vec_get(block->statements, i);
      write_statement(writer, stmt);
    }
    writer_newline(writer, -1);
  }
  writer_append(writer, "}");
  writer_newline(writer, 0);
}