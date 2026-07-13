#include "cubec/statement_comptime.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/declaration_variable.h"
#include "cubec/expression.h"
#include "cubec/expression_comma.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/statement_declaration.h"
#include "cubec/token.h"
#include <inttypes.h>

/* ==========================================================================
 *  comptime block: comptime { <body> }
 * ========================================================================== */

static void _cubec_statement_comptime_block_init(
    cubec_statement_comptime_block_t self, allocator_t allocator,
    cubec_statement_comptime_block_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_COMPTIME_BLOCK,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->body = init->body;
onerror:
  return;
}

static void _cubec_statement_comptime_block_dispose(
    cubec_statement_comptime_block_t self, allocator_t allocator) {
  allocator_free(allocator, &self->body);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_comptime_block_clone(
    cubec_statement_comptime_block_t self, allocator_t allocator,
    cubec_statement_comptime_block_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->body = TRY_LOCAL(onerror, value_clone(allocator, another->body));
  return;
onerror:
  return;
}

static void _cubec_statement_comptime_block_move(
    cubec_statement_comptime_block_t self, allocator_t allocator,
    cubec_statement_comptime_block_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->body = TRY_LOCAL(onerror, value_move(allocator, another->body));
  return;
onerror:
  return;
}

type_t g_cubec_statement_comptime_block_type = {
    .name = "cubec.cubec.statement_comptime_block",
    .size = sizeof(struct _cubec_statement_comptime_block_t),
    .init = (type_init_fn_t)_cubec_statement_comptime_block_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_comptime_block_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_comptime_block_clone,
    .move = (type_move_fn_t)_cubec_statement_comptime_block_move,
};

/* ==========================================================================
 *  comptime if: comptime if(condition) { } [else { }]
 * ========================================================================== */

static void _cubec_statement_comptime_if_init(
    cubec_statement_comptime_if_t self, allocator_t allocator,
    cubec_statement_comptime_if_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_COMPTIME_IF,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->condition = init->condition;
  self->then_branch = init->then_branch;
  self->else_branch = init->else_branch;
onerror:
  return;
}

static void _cubec_statement_comptime_if_dispose(
    cubec_statement_comptime_if_t self, allocator_t allocator) {
  allocator_free(allocator, &self->else_branch);
  allocator_free(allocator, &self->then_branch);
  allocator_free(allocator, &self->condition);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_comptime_if_clone(
    cubec_statement_comptime_if_t self, allocator_t allocator,
    cubec_statement_comptime_if_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->condition = TRY_LOCAL(onerror, value_clone(allocator, another->condition));
  self->then_branch = TRY_LOCAL(onerror, value_clone(allocator, another->then_branch));
  self->else_branch = another->else_branch ? TRY_LOCAL(onerror, value_clone(allocator, another->else_branch)) : NULL;
  return;
onerror:
  return;
}

static void _cubec_statement_comptime_if_move(
    cubec_statement_comptime_if_t self, allocator_t allocator,
    cubec_statement_comptime_if_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->condition = TRY_LOCAL(onerror, value_move(allocator, another->condition));
  self->then_branch = TRY_LOCAL(onerror, value_move(allocator, another->then_branch));
  self->else_branch = another->else_branch ? TRY_LOCAL(onerror, value_move(allocator, another->else_branch)) : NULL;
  return;
onerror:
  return;
}

type_t g_cubec_statement_comptime_if_type = {
    .name = "cubec.cubec.statement_comptime_if",
    .size = sizeof(struct _cubec_statement_comptime_if_t),
    .init = (type_init_fn_t)_cubec_statement_comptime_if_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_comptime_if_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_comptime_if_clone,
    .move = (type_move_fn_t)_cubec_statement_comptime_if_move,
};

/* ==========================================================================
 *  comptime for: comptime for(init; cond; incr) { }
 * ========================================================================== */

static void _cubec_statement_comptime_for_init(
    cubec_statement_comptime_for_t self, allocator_t allocator,
    cubec_statement_comptime_for_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_COMPTIME_FOR,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->init = init->init;
  self->condition = init->condition;
  self->increment = init->increment;
  self->body = init->body;
onerror:
  return;
}

static void _cubec_statement_comptime_for_dispose(
    cubec_statement_comptime_for_t self, allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->increment);
  allocator_free(allocator, &self->condition);
  allocator_free(allocator, &self->init);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_comptime_for_clone(
    cubec_statement_comptime_for_t self, allocator_t allocator,
    cubec_statement_comptime_for_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->init = another->init ? TRY_LOCAL(onerror, value_clone(allocator, another->init)) : NULL;
  self->condition = another->condition ? TRY_LOCAL(onerror, value_clone(allocator, another->condition)) : NULL;
  self->increment = another->increment ? TRY_LOCAL(onerror, value_clone(allocator, another->increment)) : NULL;
  self->body = TRY_LOCAL(onerror, value_clone(allocator, another->body));
  return;
onerror:
  return;
}

static void _cubec_statement_comptime_for_move(
    cubec_statement_comptime_for_t self, allocator_t allocator,
    cubec_statement_comptime_for_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->init = another->init ? TRY_LOCAL(onerror, value_move(allocator, another->init)) : NULL;
  self->condition = another->condition ? TRY_LOCAL(onerror, value_move(allocator, another->condition)) : NULL;
  self->increment = another->increment ? TRY_LOCAL(onerror, value_move(allocator, another->increment)) : NULL;
  self->body = TRY_LOCAL(onerror, value_move(allocator, another->body));
  return;
onerror:
  return;
}

type_t g_cubec_statement_comptime_for_type = {
    .name = "cubec.cubec.statement_comptime_for",
    .size = sizeof(struct _cubec_statement_comptime_for_t),
    .init = (type_init_fn_t)_cubec_statement_comptime_for_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_comptime_for_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_comptime_for_clone,
    .move = (type_move_fn_t)_cubec_statement_comptime_for_move,
};

/* ==========================================================================
 *  Helper: check keyword / symbol
 * ========================================================================== */

static bool _is_keyword(vec_t tokens, size_t position, const char *keyword) {
  token_t token = vec_get(tokens, position);
  if (!token) return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD) return false;
  return location_is(token_get_location(token), keyword);
}

static bool _is_symbol(vec_t tokens, size_t position, const char *symbol) {
  token_t token = vec_get(tokens, position);
  if (!token) return false;
  return token_is(token, CUBEC_TOKEN_SYMBOL, symbol);
}

/* ==========================================================================
 *  Parser: comptime block — comptime { <body> }
 * ========================================================================== */

static node_t _read_comptime_block(allocator_t allocator, vec_t tokens,
                                    size_t *position, const char *filename,
                                    location_t start_location) {
  size_t current = *position;
  node_t body = NULL;
  cubec_statement_comptime_block_t node = NULL;

  /* 1. Parse body (block) */
  body = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
  if (!body) {
    THROW_LOCAL(cleanup, "expected block after 'comptime'");
  }

  /* 2. Build location */
  location_t loc = start_location;
  loc.end = body->location.end;

  cubec_statement_comptime_block_init_t init = {
      .location = loc,
      .parent = NULL,
      .body = body,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_comptime_block_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &body);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &body);
  allocator_free(allocator, &node);
  return NULL;
}

/* ==========================================================================
 *  Parser: comptime if — comptime if(condition) { } [else { }]
 * ========================================================================== */

static node_t _read_comptime_if(allocator_t allocator, vec_t tokens,
                                 size_t *position, const char *filename,
                                 location_t start_location) {
  size_t current = *position;
  node_t condition = NULL;
  node_t then_branch = NULL;
  node_t else_branch = NULL;
  cubec_statement_comptime_if_t node = NULL;

  /* 1. Expect 'if' keyword */
  if (!_is_keyword(tokens, current, "if")) {
    return NULL;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    THROW_LOCAL(onerror, "expected '(' after 'comptime if'");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse condition expression */
  condition = TRY_LOCAL(cleanup, read_expression(allocator, tokens, &current, filename));
  if (!condition) {
    THROW_LOCAL(cleanup, "expected condition expression in comptime if");
  }
  skip_whitespace(tokens, &current);

  /* 4. Expect ')' */
  if (!_is_symbol(tokens, current, ")")) {
    THROW_LOCAL(cleanup, "expected ')' after comptime if condition");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse then branch (block) */
  then_branch = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
  if (!then_branch) {
    THROW_LOCAL(cleanup, "expected block after comptime if condition");
  }
  skip_whitespace(tokens, &current);

  /* 6. Optional else clause */
  if (_is_keyword(tokens, current, "else")) {
    current++;
    skip_whitespace(tokens, &current);

    if (_is_keyword(tokens, current, "if")) {
      /* else if — parse as nested comptime if */
      else_branch = TRY_LOCAL(cleanup, _read_comptime_if(allocator, tokens, &current, filename, start_location));
      if (!else_branch) {
        THROW_LOCAL(cleanup, "expected comptime if after else");
      }
    } else {
      /* else { } */
      else_branch = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
      if (!else_branch) {
        THROW_LOCAL(cleanup, "expected block after else in comptime if");
      }
    }
  }

  /* 7. Build location */
  location_t loc = start_location;
  if (else_branch) {
    loc.end = else_branch->location.end;
  } else {
    loc.end = then_branch->location.end;
  }

  cubec_statement_comptime_if_init_t init = {
      .location = loc,
      .parent = NULL,
      .condition = condition,
      .then_branch = then_branch,
      .else_branch = else_branch,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_comptime_if_type, &init));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &else_branch);
  allocator_free(allocator, &then_branch);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &else_branch);
  allocator_free(allocator, &then_branch);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &node);
  return NULL;
}

/* ==========================================================================
 *  Parser: comptime for — comptime for(init; cond; incr) { }
 * ========================================================================== */

static node_t _read_comptime_for(allocator_t allocator, vec_t tokens,
                                  size_t *position, const char *filename,
                                  location_t start_location) {
  size_t current = *position;
  node_t init = NULL;
  node_t condition = NULL;
  node_t increment = NULL;
  node_t body = NULL;
  cubec_statement_comptime_for_t node = NULL;

  /* 1. Expect 'for' keyword */
  if (!_is_keyword(tokens, current, "for")) {
    return NULL;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    THROW_LOCAL(onerror, "expected '(' after 'comptime for'");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse init (optional, ends at ';') */
  if (!_is_symbol(tokens, current, ";")) {
    if (_is_keyword(tokens, current, "var")) {
      token_t var_token = TRY_LOCAL(cleanup, vec_get(tokens, current));
      location_t var_loc = *token_get_location(var_token);
      var_loc.filename = filename;
      current++;
      skip_whitespace(tokens, &current);
      node_t declarator = TRY_LOCAL(cleanup, read_declaration_variable(allocator, tokens, &current, filename));
      if (!declarator) {
        THROW_LOCAL(cleanup, "expected variable declarator after 'var' in comptime for init");
      }
      cubec_statement_declaration_init_t decl_init = {
          .location = var_loc,
          .parent = NULL,
          .is_export = false,
          .is_extern = false,
          .is_builtin = false,
          .is_comptime = false,
          .declarator = declarator,
      };
      init = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_declaration_type, &decl_init));
      if (!init) {
        allocator_free(allocator, &declarator);
      }
    } else {
      init = TRY_LOCAL(cleanup, read_expression_comma(allocator, tokens, &current, filename));
    }
  }
  skip_whitespace(tokens, &current);

  /* 4. Expect first ';' */
  if (!_is_symbol(tokens, current, ";")) {
    THROW_LOCAL(cleanup, "expected ';' after comptime for init");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse condition (optional, ends at ';') */
  if (!_is_symbol(tokens, current, ";")) {
    condition = TRY_LOCAL(cleanup, read_expression_comma(allocator, tokens, &current, filename));
  }
  skip_whitespace(tokens, &current);

  /* 6. Expect second ';' */
  if (!_is_symbol(tokens, current, ";")) {
    THROW_LOCAL(cleanup, "expected ';' after comptime for condition");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 7. Parse increment (optional, ends at ')') */
  if (!_is_symbol(tokens, current, ")")) {
    increment = TRY_LOCAL(cleanup, read_expression_comma(allocator, tokens, &current, filename));
  }
  skip_whitespace(tokens, &current);

  /* 8. Expect ')' */
  if (!_is_symbol(tokens, current, ")")) {
    THROW_LOCAL(cleanup, "expected ')' after comptime for increment");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 9. Parse body (block) */
  body = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
  if (!body) {
    THROW_LOCAL(cleanup, "expected block after comptime for");
  }

  /* 10. Build location */
  location_t loc = start_location;
  loc.end = body->location.end;

  cubec_statement_comptime_for_init_t finit = {
      .location = loc,
      .parent = NULL,
      .init = init,
      .condition = condition,
      .increment = increment,
      .body = body,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_comptime_for_type, &finit));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &body);
  allocator_free(allocator, &increment);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &init);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &body);
  allocator_free(allocator, &increment);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &init);
  allocator_free(allocator, &node);
  return NULL;
}

/* ==========================================================================
 *  Public dispatcher: read_statement_comptime
 * ========================================================================== */

node_t read_statement_comptime(allocator_t allocator, vec_t tokens,
                                size_t *position, const char *filename) {
  size_t current = *position;

  /* 1. Expect 'comptime' keyword */
  if (!_is_keyword(tokens, current, "comptime")) {
    return NULL;
  }
  token_t comptime_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  location_t start_location = *token_get_location(comptime_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Peek at next token to determine which comptime form */
  token_t next = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!next) return NULL;

  /* comptime { ... } */
  if (token_is(next, CUBEC_TOKEN_SYMBOL, "{")) {
    return _read_comptime_block(allocator, tokens, &current, filename, start_location);
  }

  /* comptime if(...) { } */
  if (token_get_kind(next) == CUBEC_TOKEN_KEYWORD &&
      location_is(token_get_location(next), "if")) {
    return _read_comptime_if(allocator, tokens, &current, filename, start_location);
  }

  /* comptime for(...) { } */
  if (token_get_kind(next) == CUBEC_TOKEN_KEYWORD &&
      location_is(token_get_location(next), "for")) {
    return _read_comptime_for(allocator, tokens, &current, filename, start_location);
  }

  /* Not a comptime statement — let declaration/function parsers handle it */
  return NULL;

onerror:
  return NULL;
}
