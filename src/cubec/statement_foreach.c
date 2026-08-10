#include "cubec/statement_foreach.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#include "cubec/node_error.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void
_cubec_statement_foreach_init(cubec_statement_foreach_t self,
                              allocator_t allocator,
                              cubec_statement_foreach_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_FOREACH,
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

static void _cubec_statement_foreach_dispose(cubec_statement_foreach_t self,
                                             allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->iterator);
  allocator_free(allocator, &self->var_type);
  allocator_free(allocator, &self->variable);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_foreach_clone(cubec_statement_foreach_t self,
                                           allocator_t allocator,
                                           cubec_statement_foreach_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->is_var_decl = another->is_var_decl;
  self->variable = alloc_clone(allocator, another->variable);
  self->var_type = alloc_clone(allocator, another->var_type);
  self->iterator = alloc_clone(allocator, another->iterator);
  self->body = alloc_clone(allocator, another->body);
  return;
}

static void _cubec_statement_foreach_move(cubec_statement_foreach_t self,
                                          allocator_t allocator,
                                          cubec_statement_foreach_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->is_var_decl = another->is_var_decl;
  self->variable = alloc_move(allocator, another->variable);
  self->var_type = alloc_move(allocator, another->var_type);
  self->iterator = alloc_move(allocator, another->iterator);
  self->body = alloc_move(allocator, another->body);
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

/* --------------------------------------------------------------------------
 *  Parser: read_statement_foreach
 *    foreach(<identifier> of <expr>) <stmt>
 *    foreach(var <identifier>[:<type>] of <expr>) <stmt>
 * -------------------------------------------------------------------------- */

node_t read_statement_foreach(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename) {
  allocator_t allocator = ctx->allocator;
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
  token_t foreach_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(foreach_token);
  start_location.filename = filename;
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
    /* var mode: foreach(var <identifier>[:<type>] of <expr>) */
    is_var_decl = true;
    current++;
    skip_whitespace(tokens, &current);

    /* Parse identifier */
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
    /* lvalue mode: foreach(<identifier> of <expr>) */
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

  /* 7. Parse body (any statement, not just block) */
  body = read_statement(ctx, tokens, &current, filename);
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

  cubec_statement_foreach_init_t finit = {
      .location = loc,
      .parent = NULL,
      .is_var_decl = is_var_decl,
      .variable = variable,
      .var_type = var_type,
      .iterator = iterator,
      .body = body,
  };
  node = allocator_create(allocator, &g_cubec_statement_foreach_type, &finit);
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

node_t create_statement_foreach(context_t ctx, location_t loc, bool is_var_decl,
                                node_t variable, node_t var_type,
                                node_t iterator, node_t body) {
  allocator_t alloc = ctx->allocator;
  cubec_statement_foreach_init_t init = {.location = loc,
                                         .parent = NULL,
                                         .is_var_decl = is_var_decl,
                                         .variable = variable,
                                         .var_type = var_type,
                                         .iterator = iterator,
                                         .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_foreach_type,
                                  &init);
}

void emit_statement_foreach(emit_context_t ctx, node_t node) {
  cubec_statement_foreach_t stmt = (cubec_statement_foreach_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "foreach");
  emit_space(ctx);
  emit_symbol(ctx, "(");
  if (stmt->is_var_decl) {
    emit_keyword(ctx, "var");
    emit_space(ctx);
  }
  emit_expression(ctx, stmt->variable);
  if (stmt->var_type) {
    emit_symbol(ctx, ":");
    emit_space(ctx);
    emit_expression(ctx, stmt->var_type);
  }
  emit_space(ctx);
  emit_keyword(ctx, "of");
  emit_space(ctx);
  emit_expression(ctx, stmt->iterator);
  emit_symbol(ctx, ")");
  emit_space(ctx);
  emit_statement(ctx, stmt->body);
}
