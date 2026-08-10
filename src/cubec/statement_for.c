#include "cubec/statement_for.h"
#include "core/token.h"
#include "cubec/declaration_variable.h"
#include "cubec/expression.h"
#include "cubec/expression_comma.h"
#include "cubec/node_error.h"
#include "cubec/statement.h"
#include "cubec/statement_declaration.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_for_init(cubec_statement_for_t self,
                                      allocator_t allocator,
                                      cubec_statement_for_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_FOR,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_class.init(&self->super, allocator, &super_init);
  self->init = init->init;
  self->condition = init->condition;
  self->increment = init->increment;
  self->body = init->body;
}

static void _cubec_statement_for_dispose(cubec_statement_for_t self,
                                         allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->increment);
  allocator_free(allocator, &self->condition);
  allocator_free(allocator, &self->init);
  g_node_class.dispose(&self->super, allocator);
}

static void _cubec_statement_for_clone(cubec_statement_for_t self,
                                       allocator_t allocator,
                                       cubec_statement_for_t another) {
  g_node_class.clone(&self->super, allocator, &another->super);
  self->init = another->init ? alloc_clone(allocator, another->init) : NULL;
  self->condition =
      another->condition ? alloc_clone(allocator, another->condition) : NULL;
  self->increment =
      another->increment ? alloc_clone(allocator, another->increment) : NULL;
  self->body = alloc_clone(allocator, another->body);
  return;
}

static void _cubec_statement_for_move(cubec_statement_for_t self,
                                      allocator_t allocator,
                                      cubec_statement_for_t another) {
  g_node_class.move(&self->super, allocator, &another->super);
  self->init = another->init ? alloc_move(allocator, another->init) : NULL;
  self->condition =
      another->condition ? alloc_move(allocator, another->condition) : NULL;
  self->increment =
      another->increment ? alloc_move(allocator, another->increment) : NULL;
  self->body = alloc_move(allocator, another->body);
  return;
}

class_t g_cubec_statement_for_class = {
    .name = "cubec.cubec.statement_for",
    .size = sizeof(struct _cubec_statement_for_t),
    .init = (class_init_fn_t)_cubec_statement_for_init,
    .dispose = (class_dispose_fn_t)_cubec_statement_for_dispose,
    .clone = (class_clone_fn_t)_cubec_statement_for_clone,
    .move = (class_move_fn_t)_cubec_statement_for_move,
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
 *  Parser: read_statement_for — for(init; cond; incr) { }
 * -------------------------------------------------------------------------- */

node_t read_statement_for(context_t ctx, vec_t tokens, size_t *position,
                          const char *filename) {
  allocator_t allocator = ctx->allocator;
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
  token_t for_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(for_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse init (optional, ends at ';') */
  if (!_is_symbol(tokens, current, ";")) {
    if (_is_keyword(tokens, current, "var")) {
      /* Parse var declaration without consuming ';' */
      token_t var_token = vec_get(tokens, current);
      location_t var_loc = *token_get_location(var_token);
      var_loc.filename = filename;
      current++;
      skip_whitespace(tokens, &current);
      node_t declarator =
          read_declaration_variable(ctx, tokens, &current, filename);
      if (node_is_error(declarator))
        return declarator;
      if (!declarator)
        goto onerror;
      /* Wrap in statement_expression-like node: use statement_declaration
       * pattern */
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
      init = allocator_create(allocator, &g_cubec_statement_declaration_class,
                              &decl_init);
      if (!init) {
        allocator_free(allocator, &declarator);
      }
    } else {
      /* Parse as expression (including assignment) */
      init = read_expression_comma(ctx, tokens, &current, filename);
      if (node_is_error(init))
        return init;
      if (!init)
        goto onerror;
    }
  }
  skip_whitespace(tokens, &current);

  /* 4. Expect first ';' */
  if (!_is_symbol(tokens, current, ";")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse condition (optional, ends at ';') */
  if (!_is_symbol(tokens, current, ";")) {
    condition = read_expression_comma(ctx, tokens, &current, filename);
    if (node_is_error(condition)) {
      allocator_free(allocator, &init);
      return condition;
    }
    if (!condition)
      goto onerror;
  }
  skip_whitespace(tokens, &current);

  /* 6. Expect second ';' */
  if (!_is_symbol(tokens, current, ";")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 7. Parse increment (optional, ends at ')') */
  if (!_is_symbol(tokens, current, ")")) {
    increment = read_expression_comma(ctx, tokens, &current, filename);
    if (node_is_error(increment)) {
      allocator_free(allocator, &condition);
      allocator_free(allocator, &init);
      return increment;
    }
    if (!increment)
      goto onerror;
  }
  skip_whitespace(tokens, &current);

  /* 8. Expect ')' */
  if (!_is_symbol(tokens, current, ")")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 9. Parse body (any statement) */
  body = read_statement(ctx, tokens, &current, filename);
  if (node_is_error(body)) {
    allocator_free(allocator, &increment);
    allocator_free(allocator, &condition);
    allocator_free(allocator, &init);
    return body;
  }
  if (!body)
    goto onerror;

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
  node = allocator_create(allocator, &g_cubec_statement_for_class, &finit);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &body);
  allocator_free(allocator, &increment);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &init);
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

node_t create_statement_for(context_t ctx, location_t loc, node_t init_node,
                            node_t cond, node_t incr, node_t body) {
  allocator_t alloc = ctx->allocator;
  cubec_statement_for_init_t init = {.location = loc,
                                     .parent = NULL,
                                     .init = init_node,
                                     .condition = cond,
                                     .increment = incr,
                                     .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_for_class, &init);
}

void emit_statement_for(emit_context_t ctx, node_t node) {
  cubec_statement_for_t stmt = (cubec_statement_for_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "for");
  emit_space(ctx);
  emit_symbol(ctx, "(");
  if (stmt->init) {
    if (stmt->init->kind == CUBEC_NODE_STATEMENT_DECLARATION) {
      cubec_statement_declaration_t init =
          (cubec_statement_declaration_t)stmt->init;
      if (init->is_using)
        emit_keyword(ctx, "using");
      else
        emit_keyword(ctx, "var");
      emit_space(ctx);
      emit_declaration_variable(ctx, init->declarator);
    } else {
      emit_expression(ctx, stmt->init);
    }
  }
  emit_symbol(ctx, ";");
  emit_space(ctx);
  if (stmt->condition) emit_expression(ctx, stmt->condition);
  emit_symbol(ctx, ";");
  emit_space(ctx);
  if (stmt->increment) emit_expression(ctx, stmt->increment);
  emit_symbol(ctx, ")");
  emit_space(ctx);
  emit_statement(ctx, stmt->body);
}
