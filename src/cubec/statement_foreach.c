#include "cubec/statement_foreach.h"
#include "cubec/ast_factory_internal.h"
#include "cubec/ast_factory.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/statement.h"
#include "cubec/statement_block.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_foreach_init(
    cubec_statement_foreach_t self, allocator_t allocator,
    cubec_statement_foreach_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_FOREACH,
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

static void _cubec_statement_foreach_dispose(
    cubec_statement_foreach_t self, allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->iterator);
  allocator_free(allocator, &self->var_type);
  allocator_free(allocator, &self->variable);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_foreach_clone(
    cubec_statement_foreach_t self, allocator_t allocator,
    cubec_statement_foreach_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->is_var_decl = another->is_var_decl;
  self->variable = TRY_LOCAL(onerror, value_clone(allocator, another->variable));
  self->var_type = TRY_LOCAL(onerror, value_clone(allocator, another->var_type));
  self->iterator = TRY_LOCAL(onerror, value_clone(allocator, another->iterator));
  self->body = TRY_LOCAL(onerror, value_clone(allocator, another->body));
  return;
onerror:
  return;
}

static void _cubec_statement_foreach_move(
    cubec_statement_foreach_t self, allocator_t allocator,
    cubec_statement_foreach_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->is_var_decl = another->is_var_decl;
  self->variable = TRY_LOCAL(onerror, value_move(allocator, another->variable));
  self->var_type = TRY_LOCAL(onerror, value_move(allocator, another->var_type));
  self->iterator = TRY_LOCAL(onerror, value_move(allocator, another->iterator));
  self->body = TRY_LOCAL(onerror, value_move(allocator, another->body));
  return;
onerror:
  return;
}

type_t g_cubec_statement_foreach_type = {
    .name = "cubec.cubec.statement_foreach",
    .size = sizeof(struct _cubec_statement_foreach_t),
    .init = (type_init_fn_t)_cubec_statement_foreach_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_foreach_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_foreach_clone,
    .move = (type_move_fn_t)_cubec_statement_foreach_move,
};

/* --------------------------------------------------------------------------
 *  Helper: check keyword / symbol
 * -------------------------------------------------------------------------- */

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

/* --------------------------------------------------------------------------
 *  Parser: read_statement_foreach
 *    foreach(<identifier> of <expr>) <stmt>
 *    foreach(var <identifier>[:<type>] of <expr>) <stmt>
 * -------------------------------------------------------------------------- */

node_t read_statement_foreach(allocator_t allocator, vec_t tokens,
                               size_t *position, const char *filename) {
  size_t current = *position;
  node_t variable = NULL;
  node_t var_type = NULL;
  node_t iterator = NULL;
  node_t body = NULL;
  cubec_statement_foreach_t node = NULL;
  bool is_var_decl = false;

  /* 1. Expect 'foreach' keyword */
  if (!_is_keyword(tokens, current, "foreach")) {
    return NULL;
  }
  token_t foreach_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  location_t start_location = *token_get_location(foreach_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    THROW_LOCAL(onerror, "expected '(' after 'foreach'");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Determine mode: var or lvalue */
  if (_is_keyword(tokens, current, "var")) {
    /* var mode: foreach(var <identifier>[:<type>] of <expr>) */
    is_var_decl = true;
    current++;
    skip_whitespace(tokens, &current);

    /* Parse identifier */
    variable = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
    if (!variable) {
      THROW_LOCAL(cleanup, "expected identifier after 'var' in foreach");
    }
    skip_whitespace(tokens, &current);

    /* Optional type annotation ': <type>' */
    if (_is_symbol(tokens, current, ":")) {
      current++;
      skip_whitespace(tokens, &current);
      var_type = TRY_LOCAL(cleanup, read_expression_type(allocator, tokens, &current, filename));
      if (!var_type) {
        THROW_LOCAL(cleanup, "expected type after ':' in foreach");
      }
      skip_whitespace(tokens, &current);
    }
  } else {
    /* lvalue mode: foreach(<identifier> of <expr>) */
    is_var_decl = false;
    variable = TRY_LOCAL(cleanup, read_literal_identifier(allocator, tokens, &current, filename));
    if (!variable) {
      THROW_LOCAL(cleanup, "expected identifier in foreach");
    }
    skip_whitespace(tokens, &current);
  }

  /* 4. Expect 'of' keyword */
  if (!_is_keyword(tokens, current, "of")) {
    THROW_LOCAL(cleanup, "expected 'of' in foreach");
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
    token_t tok = TRY_LOCAL(cleanup, vec_get(tokens, current));
    location_t *loc = token_get_location(tok);
    THROW_LOCAL(cleanup, "%s:%" PRIuPTR ":%" PRIuPTR " expected ')' after iterator",
                filename, loc->begin.line + 1, loc->begin.column);
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 7. Parse body (any statement, not just block) */
  body = TRY_LOCAL(cleanup, read_statement(allocator, tokens, &current, filename));
  if (!body) {
    THROW_LOCAL(cleanup, "expected statement after foreach");
  }

  /* 8. Build location */
  location_t loc = start_location;
  loc.end = body->location.end;

  cubec_statement_foreach_init_t finit = {
      .location = loc,
      .parent = NULL,
      .is_var_decl = is_var_decl,
      .variable = variable,
      .var_type = var_type,
      .iterator = iterator,
      .body = body,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_foreach_type, &finit));
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

node_t cubec_ast_create_foreach_stmt(allocator_t alloc, location_t loc,
                                     bool is_var_decl, node_t variable,
                                     node_t var_type, node_t iterator,
                                     node_t body) {
  cubec_statement_foreach_init_t init = {
      .location = loc, .parent = NULL, .is_var_decl = is_var_decl,
      .variable = variable, .var_type = var_type,
      .iterator = iterator, .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_foreach_type,
                                  &init);
}
