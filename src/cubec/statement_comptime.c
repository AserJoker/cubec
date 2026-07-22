#include "cubec/statement_comptime.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/declaration_variable.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
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
 *  comptime foreach: comptime foreach(var item [: type] of iter) { }
 * ========================================================================== */

static void _cubec_statement_comptime_foreach_init(
    cubec_statement_comptime_foreach_t self, allocator_t allocator,
    cubec_statement_comptime_foreach_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_COMPTIME_FOREACH,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->is_var_decl = init->is_var_decl;
  self->variable = init->variable;
  self->var_type = init->var_type;
  self->iterator = init->iterator;
  self->body = init->body;
onerror:
  return;
}

static void _cubec_statement_comptime_foreach_dispose(
    cubec_statement_comptime_foreach_t self, allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->iterator);
  allocator_free(allocator, &self->var_type);
  allocator_free(allocator, &self->variable);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_comptime_foreach_clone(
    cubec_statement_comptime_foreach_t self, allocator_t allocator,
    cubec_statement_comptime_foreach_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->is_var_decl = another->is_var_decl;
  self->variable = TRY_LOCAL(onerror, value_clone(allocator, another->variable));
  self->var_type = another->var_type ? TRY_LOCAL(onerror, value_clone(allocator, another->var_type)) : NULL;
  self->iterator = TRY_LOCAL(onerror, value_clone(allocator, another->iterator));
  self->body = TRY_LOCAL(onerror, value_clone(allocator, another->body));
  return;
onerror:
  return;
}

static void _cubec_statement_comptime_foreach_move(
    cubec_statement_comptime_foreach_t self, allocator_t allocator,
    cubec_statement_comptime_foreach_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->is_var_decl = another->is_var_decl;
  self->variable = TRY_LOCAL(onerror, value_move(allocator, another->variable));
  self->var_type = another->var_type ? TRY_LOCAL(onerror, value_move(allocator, another->var_type)) : NULL;
  self->iterator = TRY_LOCAL(onerror, value_move(allocator, another->iterator));
  self->body = TRY_LOCAL(onerror, value_move(allocator, another->body));
  return;
onerror:
  return;
}

type_t g_cubec_statement_comptime_foreach_type = {
    .name = "cubec.cubec.statement_comptime_foreach",
    .size = sizeof(struct _cubec_statement_comptime_foreach_t),
    .init = (type_init_fn_t)_cubec_statement_comptime_foreach_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_comptime_foreach_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_comptime_foreach_clone,
    .move = (type_move_fn_t)_cubec_statement_comptime_foreach_move,
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
 *  Parser: comptime foreach — comptime foreach(var item [: type] of iter) { }
 * ========================================================================== */

static node_t _read_comptime_foreach(allocator_t allocator, vec_t tokens,
                                      size_t *position, const char *filename,
                                      location_t start_location) {
  size_t current = *position;
  node_t variable = NULL;
  node_t var_type = NULL;
  node_t iterator = NULL;
  node_t body = NULL;
  cubec_statement_comptime_foreach_t node = NULL;
  bool is_var_decl = false;

  /* 1. Expect 'foreach' keyword */
  if (!_is_keyword(tokens, current, "foreach")) {
    return NULL;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    THROW_LOCAL(onerror, "expected '(' after 'comptime foreach'");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Determine mode: var or lvalue */
  if (_is_keyword(tokens, current, "var")) {
    is_var_decl = true;
    current++;
    skip_whitespace(tokens, &current);

    variable = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
    if (!variable) {
      THROW_LOCAL(cleanup, "expected identifier after 'var' in comptime foreach");
    }
    skip_whitespace(tokens, &current);

    /* Optional type annotation ': <type>' */
    if (_is_symbol(tokens, current, ":")) {
      current++;
      skip_whitespace(tokens, &current);
      var_type = TRY_LOCAL(cleanup, read_expression_type(allocator, tokens, &current, filename));
      if (!var_type) {
        THROW_LOCAL(cleanup, "expected type after ':' in comptime foreach");
      }
      skip_whitespace(tokens, &current);
    }
  } else {
    is_var_decl = false;
    variable = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
    if (!variable) {
      THROW_LOCAL(cleanup, "expected identifier in comptime foreach");
    }
    skip_whitespace(tokens, &current);
  }

  /* 4. Expect 'of' keyword */
  if (!_is_keyword(tokens, current, "of")) {
    THROW_LOCAL(cleanup, "expected 'of' in comptime foreach");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse iterator expression */
  iterator = TRY_LOCAL(cleanup, read_expression(allocator, tokens, &current, filename));
  if (!iterator) {
    THROW_LOCAL(cleanup, "expected iterator expression after 'of'");
  }
  skip_whitespace(tokens, &current);

  /* 6. Expect ')' */
  if (!_is_symbol(tokens, current, ")")) {
    THROW_LOCAL(cleanup, "expected ')' after iterator in comptime foreach");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 7. Parse body (block) */
  body = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
  if (!body) {
    THROW_LOCAL(cleanup, "expected block after comptime foreach");
  }

  /* 8. Build location */
  location_t loc = start_location;
  loc.end = body->location.end;

  cubec_statement_comptime_foreach_init_t finit = {
      .location = loc,
      .parent = NULL,
      .is_var_decl = is_var_decl,
      .variable = variable,
      .var_type = var_type,
      .iterator = iterator,
      .body = body,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_comptime_foreach_type, &finit));
  *position = current;
  return &node->super;

cleanup:
  allocator_free(allocator, &body);
  allocator_free(allocator, &iterator);
  allocator_free(allocator, &var_type);
  allocator_free(allocator, &variable);
  allocator_free(allocator, &node);
onerror:
  allocator_free(allocator, &body);
  allocator_free(allocator, &iterator);
  allocator_free(allocator, &var_type);
  allocator_free(allocator, &variable);
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
  if (!next) {
    THROW_LOCAL(onerror, "expected '{', 'if', or 'for' after 'comptime'");
  }

  /* If next token is a declaration/function modifier, this is 'comptime' as
   * a modifier (e.g. comptime var, comptime func), not a comptime block/if/for/foreach.
   * Return NULL so the caller can try read_statement_declaration / function. */
  if (token_get_kind(next) == CUBEC_TOKEN_KEYWORD) {
    if (location_is(token_get_location(next), "var") ||
        location_is(token_get_location(next), "func") ||
        location_is(token_get_location(next), "export") ||
        location_is(token_get_location(next), "inline") ||
        location_is(token_get_location(next), "extern") ||
        location_is(token_get_location(next), "builtin")) {
      return NULL;
    }
  }

  node_t result = NULL;

  /* comptime { ... } */
  if (token_is(next, CUBEC_TOKEN_SYMBOL, "{")) {
    result = TRY_LOCAL(onerror, _read_comptime_block(allocator, tokens, &current, filename, start_location));
  }
  /* comptime if(...) { } */
  else if (token_get_kind(next) == CUBEC_TOKEN_KEYWORD &&
      location_is(token_get_location(next), "if")) {
    result = TRY_LOCAL(onerror, _read_comptime_if(allocator, tokens, &current, filename, start_location));
  }
  /* comptime foreach(...) { } */
  else if (token_get_kind(next) == CUBEC_TOKEN_KEYWORD &&
      location_is(token_get_location(next), "foreach")) {
    result = TRY_LOCAL(onerror, _read_comptime_foreach(allocator, tokens, &current, filename, start_location));
  }
  else {
    THROW_LOCAL(onerror, "expected '{', 'if', or 'foreach' after 'comptime'");
  }

  *position = current;
  return result;

onerror:
  return NULL;
}
