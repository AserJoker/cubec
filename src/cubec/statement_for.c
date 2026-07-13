#include "cubec/statement_for.h"
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
#include "cubec/statement_expression.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_for_init(
    cubec_statement_for_t self, allocator_t allocator,
    cubec_statement_for_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_FOR,
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

static void _cubec_statement_for_dispose(
    cubec_statement_for_t self, allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->increment);
  allocator_free(allocator, &self->condition);
  allocator_free(allocator, &self->init);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_for_clone(
    cubec_statement_for_t self, allocator_t allocator,
    cubec_statement_for_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->init = another->init ? TRY_LOCAL(onerror, value_clone(allocator, another->init)) : NULL;
  self->condition = another->condition ? TRY_LOCAL(onerror, value_clone(allocator, another->condition)) : NULL;
  self->increment = another->increment ? TRY_LOCAL(onerror, value_clone(allocator, another->increment)) : NULL;
  self->body = TRY_LOCAL(onerror, value_clone(allocator, another->body));
  return;
onerror:
  return;
}

static void _cubec_statement_for_move(
    cubec_statement_for_t self, allocator_t allocator,
    cubec_statement_for_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->init = another->init ? TRY_LOCAL(onerror, value_move(allocator, another->init)) : NULL;
  self->condition = another->condition ? TRY_LOCAL(onerror, value_move(allocator, another->condition)) : NULL;
  self->increment = another->increment ? TRY_LOCAL(onerror, value_move(allocator, another->increment)) : NULL;
  self->body = TRY_LOCAL(onerror, value_move(allocator, another->body));
  return;
onerror:
  return;
}

type_t g_cubec_statement_for_type = {
    .name = "cubec.cubec.statement_for",
    .size = sizeof(struct _cubec_statement_for_t),
    .init = (type_init_fn_t)_cubec_statement_for_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_for_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_for_clone,
    .move = (type_move_fn_t)_cubec_statement_for_move,
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
 *  Parser: read_statement_for — for(init; cond; incr) { }
 * -------------------------------------------------------------------------- */

node_t read_statement_for(allocator_t allocator, vec_t tokens,
                           size_t *position, const char *filename) {
  size_t current = *position;
  node_t init = NULL;
  node_t condition = NULL;
  node_t increment = NULL;
  node_t body = NULL;
  cubec_statement_for_t node = NULL;

  /* 1. Expect 'for' keyword */
  if (!_is_keyword(tokens, current, "for")) {
    return NULL;
  }
  token_t for_token = TRY_LOCAL(onerror, vec_get(tokens, current));
  location_t start_location = *token_get_location(for_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    THROW_LOCAL(onerror, "expected '(' after 'for'");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse init (optional, ends at ';') */
  if (!_is_symbol(tokens, current, ";")) {
    if (_is_keyword(tokens, current, "var")) {
      /* Parse var declaration without consuming ';' */
      token_t var_token = TRY_LOCAL(cleanup, vec_get(tokens, current));
      location_t var_loc = *token_get_location(var_token);
      var_loc.filename = filename;
      current++;
      skip_whitespace(tokens, &current);
      node_t declarator = TRY_LOCAL(cleanup, read_declaration_variable(allocator, tokens, &current, filename));
      if (!declarator) {
        THROW_LOCAL(cleanup, "expected variable declarator after 'var' in for init");
      }
      /* Wrap in statement_expression-like node: use statement_declaration pattern */
      /* Actually, just create a statement_declaration node without the ';' */
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
      /* Parse as expression (including assignment) */
      init = TRY_LOCAL(cleanup, read_expression_comma(allocator, tokens, &current, filename));
    }
  }
  skip_whitespace(tokens, &current);

  /* 4. Expect first ';' */
  if (!_is_symbol(tokens, current, ";")) {
    THROW_LOCAL(cleanup, "expected ';' after for init");
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
    THROW_LOCAL(cleanup, "expected ';' after for condition");
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
    THROW_LOCAL(cleanup, "expected ')' after for increment");
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 9. Parse body (block) */
  body = TRY_LOCAL(cleanup, read_statement_block(allocator, tokens, &current, filename));
  if (!body) {
    THROW_LOCAL(cleanup, "expected block after for");
  }

  /* 10. Build location */
  location_t loc = start_location;
  loc.end = body->location.end;

  cubec_statement_for_init_t finit = {
      .location = loc,
      .parent = NULL,
      .init = init,
      .condition = condition,
      .increment = increment,
      .body = body,
  };
  node = TRY_LOCAL(cleanup, allocator_create(allocator, &g_cubec_statement_for_type, &finit));
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
