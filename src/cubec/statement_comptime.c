#include "cubec/statement_comptime.h"
#include "core/token.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/statement_block.h"
#include "cubec/token.h"
#include <inttypes.h>


/* ==========================================================================
 *  comptime if: comptime if(condition) { } [else { }]
 * ========================================================================== */

static void
_cubec_statement_comptime_if_init(cubec_statement_comptime_if_t self,
                                  allocator_t allocator,
                                  cubec_statement_comptime_if_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_COMPTIME_IF,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->condition = init->condition;
  self->then_branch = init->then_branch;
  self->else_branch = init->else_branch;
}

static void
_cubec_statement_comptime_if_dispose(cubec_statement_comptime_if_t self,
                                     allocator_t allocator) {
  allocator_free(allocator, &self->else_branch);
  allocator_free(allocator, &self->then_branch);
  allocator_free(allocator, &self->condition);
  g_node_type.dispose(&self->super, allocator);
}

static void
_cubec_statement_comptime_if_clone(cubec_statement_comptime_if_t self,
                                   allocator_t allocator,
                                   cubec_statement_comptime_if_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->condition = value_clone(allocator, another->condition);
  self->then_branch = value_clone(allocator, another->then_branch);
  self->else_branch = another->else_branch
                          ? value_clone(allocator, another->else_branch)
                          : NULL;
  return;
}

static void
_cubec_statement_comptime_if_move(cubec_statement_comptime_if_t self,
                                  allocator_t allocator,
                                  cubec_statement_comptime_if_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->condition = value_move(allocator, another->condition);
  self->then_branch = value_move(allocator, another->then_branch);
  self->else_branch =
      another->else_branch ? value_move(allocator, another->else_branch) : NULL;
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
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_COMPTIME_FOREACH,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->is_var_decl = init->is_var_decl;
  self->variable = init->variable;
  self->var_type = init->var_type;
  self->iterator = init->iterator;
  self->body = init->body;
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
  g_node_type.clone(&self->super, allocator, &another->super);
  self->is_var_decl = another->is_var_decl;
  self->variable = value_clone(allocator, another->variable);
  self->var_type =
      another->var_type ? value_clone(allocator, another->var_type) : NULL;
  self->iterator = value_clone(allocator, another->iterator);
  self->body = value_clone(allocator, another->body);
  return;
}

static void _cubec_statement_comptime_foreach_move(
    cubec_statement_comptime_foreach_t self, allocator_t allocator,
    cubec_statement_comptime_foreach_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->is_var_decl = another->is_var_decl;
  self->variable = value_move(allocator, another->variable);
  self->var_type =
      another->var_type ? value_move(allocator, another->var_type) : NULL;
  self->iterator = value_move(allocator, another->iterator);
  self->body = value_move(allocator, another->body);
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
  if (!token)
    return false;
  if (token_get_kind(token) != CUBEC_TOKEN_KEYWORD)
    return false;
  return location_is(token_get_location(token), keyword);
}

static bool _is_symbol(vec_t tokens, size_t position, const char *symbol) {
  token_t token = vec_get(tokens, position);
  if (!token)
    return false;
  return token_is(token, CUBEC_TOKEN_SYMBOL, symbol);
}

/* ==========================================================================
 *  Parser: comptime if — comptime if(condition) { } [else { }]
 * ========================================================================== */

static node_t _read_comptime_if(context_t ctx, vec_t tokens, size_t *position,
                                const char *filename,
                                location_t start_location) {
  allocator_t allocator = ctx->allocator;
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
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse condition expression */
  condition = read_expression(ctx, tokens, &current, filename);
  if (node_is_error(condition))
    return condition;
  if (!condition)
    goto onerror;
  skip_whitespace(tokens, &current);

  /* 4. Expect ')' */
  if (!_is_symbol(tokens, current, ")")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse then branch (block) */
  then_branch = read_statement_block(ctx, tokens, &current, filename);
  if (node_is_error(then_branch)) {
    allocator_free(allocator, &condition);
    return then_branch;
  }
  if (!then_branch)
    goto onerror;
  skip_whitespace(tokens, &current);

  /* 6. Optional else clause */
  if (_is_keyword(tokens, current, "else")) {
    current++;
    skip_whitespace(tokens, &current);

    if (_is_keyword(tokens, current, "if")) {
      /* else if — parse as nested comptime if */
      else_branch =
          _read_comptime_if(ctx, tokens, &current, filename, start_location);
      if (node_is_error(else_branch)) {
        allocator_free(allocator, &condition);
        allocator_free(allocator, &then_branch);
        return else_branch;
      }
      if (!else_branch)
        goto onerror;
    } else {
      /* else { } */
      else_branch = read_statement_block(ctx, tokens, &current, filename);
      if (node_is_error(else_branch)) {
        allocator_free(allocator, &condition);
        allocator_free(allocator, &then_branch);
        return else_branch;
      }
      if (!else_branch)
        goto onerror;
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
  node =
      allocator_create(allocator, &g_cubec_statement_comptime_if_type, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &else_branch);
  allocator_free(allocator, &then_branch);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

/* ==========================================================================
 *  Parser: comptime foreach — comptime foreach(var item [: type] of iter) { }
 * ========================================================================== */

static node_t _read_comptime_foreach(context_t ctx, vec_t tokens,
                                     size_t *position, const char *filename,
                                     location_t start_location) {
  allocator_t allocator = ctx->allocator;
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
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Determine mode: var or lvalue */
  if (_is_keyword(tokens, current, "var")) {
    is_var_decl = true;
    current++;
    skip_whitespace(tokens, &current);

    variable = read_literal_identifier(ctx, tokens, &current, filename);
    if (node_is_error(variable))
      return variable;
    if (!variable)
      goto onerror;
    skip_whitespace(tokens, &current);

    /* Optional type annotation ': <type>' */
    if (_is_symbol(tokens, current, ":")) {
      current++;
      skip_whitespace(tokens, &current);
      var_type = read_expression_type(ctx, tokens, &current, filename);
      if (node_is_error(var_type)) {
        allocator_free(allocator, &variable);
        return var_type;
      }
      if (!var_type)
        goto onerror;
      skip_whitespace(tokens, &current);
    }
  } else {
    is_var_decl = false;
    variable = read_literal_identifier(ctx, tokens, &current, filename);
    if (node_is_error(variable))
      return variable;
    if (!variable)
      goto onerror;
    skip_whitespace(tokens, &current);
  }

  /* 4. Expect 'of' keyword */
  if (!_is_keyword(tokens, current, "of")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse iterator expression */
  iterator = read_expression(ctx, tokens, &current, filename);
  if (node_is_error(iterator)) {
    allocator_free(allocator, &var_type);
    allocator_free(allocator, &variable);
    return iterator;
  }
  if (!iterator)
    goto onerror;
  skip_whitespace(tokens, &current);

  /* 6. Expect ')' */
  if (!_is_symbol(tokens, current, ")")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 7. Parse body (block) */
  body = read_statement_block(ctx, tokens, &current, filename);
  if (node_is_error(body)) {
    allocator_free(allocator, &iterator);
    allocator_free(allocator, &var_type);
    allocator_free(allocator, &variable);
    return body;
  }
  if (!body)
    goto onerror;

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
  node = allocator_create(allocator, &g_cubec_statement_comptime_foreach_type,
                          &finit);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &body);
  allocator_free(allocator, &iterator);
  allocator_free(allocator, &var_type);
  allocator_free(allocator, &variable);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

/* ==========================================================================
 *  Public dispatcher: read_statement_comptime
 * ========================================================================== */

node_t read_statement_comptime(context_t ctx, vec_t tokens, size_t *position,
                               const char *filename) {
  allocator_t allocator = ctx->allocator;
  (void)allocator;
  size_t current = *position;

  /* 1. Expect 'comptime' keyword */
  if (!_is_keyword(tokens, current, "comptime")) {
    return NULL;
  }
  token_t comptime_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(comptime_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Peek at next token to determine which comptime form */
  token_t next = vec_get(tokens, current);
  if (!next) {
    goto onerror;
  }

  /* If next token is a declaration/function modifier, this is 'comptime' as
   * a modifier (e.g. comptime var, comptime func), not a comptime
   * block/if/for/foreach. Return NULL so the caller can try
   * read_statement_declaration / function. */
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

  /* comptime if(...) { } */
  if (token_get_kind(next) == CUBEC_TOKEN_KEYWORD &&
      location_is(token_get_location(next), "if")) {
    result = _read_comptime_if(ctx, tokens, &current, filename, start_location);
  }
  /* comptime foreach(...) { } */
  else if (token_get_kind(next) == CUBEC_TOKEN_KEYWORD &&
           location_is(token_get_location(next), "foreach")) {
    result =
        _read_comptime_foreach(ctx, tokens, &current, filename, start_location);
  } else {
    goto onerror;
  }

  /* Error propagation: sub-parser returned an Error node */
  if (node_is_error(result))
    return result;
  /* NULL means sub-parser didn't recognize the pattern */
  if (!result)
    goto onerror;

  *position = current;
  return result;

onerror:
  return create_error(ctx, start_location);
}
