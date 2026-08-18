#include "cubec/statement_do_while.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node_error.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void
_cubec_statement_do_while_init(cubec_statement_do_while_t self,
                               allocator_t allocator,
                               cubec_statement_do_while_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_DO_WHILE,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_class.init(&self->super, allocator, &super_init);
  self->body = init->body;
  self->condition = init->condition;
}

static void _cubec_statement_do_while_dispose(cubec_statement_do_while_t self,
                                              allocator_t allocator) {
  allocator_free(allocator, &self->condition);
  allocator_free(allocator, &self->body);
  g_node_class.dispose(&self->super, allocator);
}

static void
_cubec_statement_do_while_clone(cubec_statement_do_while_t self,
                                allocator_t allocator,
                                cubec_statement_do_while_t another) {
  g_node_class.clone(&self->super, allocator, &another->super);
  self->body = alloc_clone(allocator, another->body);
  self->condition = alloc_clone(allocator, another->condition);
  return;
}

static void _cubec_statement_do_while_move(cubec_statement_do_while_t self,
                                           allocator_t allocator,
                                           cubec_statement_do_while_t another) {
  g_node_class.move(&self->super, allocator, &another->super);
  self->body = alloc_move(allocator, another->body);
  self->condition = alloc_move(allocator, another->condition);
  return;
}

class_t g_cubec_statement_do_while_class = {
    .name = "cubec.cubec.statement_do_while",
    .size = sizeof(struct _cubec_statement_do_while_t),
    .init = (class_init_fn_t)_cubec_statement_do_while_init,
    .dispose = (class_dispose_fn_t)_cubec_statement_do_while_dispose,
    .clone = (class_clone_fn_t)_cubec_statement_do_while_clone,
    .move = (class_move_fn_t)_cubec_statement_do_while_move,
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
 *  Parser: read_statement_do_while — do { } while(condition);
 * -------------------------------------------------------------------------- */

node_t read_statement_do_while(vm_t vm, vec_t tokens, size_t *position,
                               const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;
  node_t body = NULL;
  node_t condition = NULL;
  cubec_statement_do_while_t node = NULL;

  /* 1. Expect 'do' keyword */
  if (!_is_keyword(tokens, current, "do")) {
    return NULL;
  }
  token_t do_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(do_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Parse body (any statement) */
  body = read_statement(vm, tokens, &current, filename);
  if (node_is_error(body))
    return body;
  if (!body)
    goto onerror;
  skip_whitespace(tokens, &current);

  /* 3. Expect 'while' keyword */
  if (!_is_keyword(tokens, current, "while")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 4. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 5. Parse condition */
  condition = read_expression(vm, tokens, &current, filename);
  if (node_is_error(condition)) {
    allocator_free(allocator, &body);
    return condition;
  }
  if (!condition)
    goto onerror;
  skip_whitespace(tokens, &current);

  /* 6. Expect ')' */
  if (!_is_symbol(tokens, current, ")")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 7. Expect ';' */
  if (!_is_symbol(tokens, current, ";")) {
    goto onerror;
  }
  token_t semi = vec_get(tokens, current);
  current++;

  /* 8. Build location */
  location_t loc = start_location;
  loc.end = token_get_location(semi)->end;

  cubec_statement_do_while_init_t init = {
      .location = loc,
      .parent = NULL,
      .body = body,
      .condition = condition,
  };
  node = allocator_create(allocator, &g_cubec_statement_do_while_class, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &condition);
  allocator_free(allocator, &body);
  allocator_free(allocator, &node);
  return create_error(vm, start_location);
}

node_t create_statement_do_while(vm_t vm, location_t loc, node_t body,
                                 node_t cond) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_statement_do_while_init_t init = {.body = body, .condition = cond};
  return (node_t)allocator_create(alloc, &g_cubec_statement_do_while_class,
                                  &init);
}

void emit_statement_do_while(emit_context_t ctx, node_t node) {
  cubec_statement_do_while_t stmt = (cubec_statement_do_while_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "do");
  emit_space(ctx);
  emit_statement(ctx, stmt->body);
  emit_space(ctx);
  emit_keyword(ctx, "while");
  emit_space(ctx);
  emit_symbol(ctx, "(");
  emit_expression(ctx, stmt->condition);
  emit_symbol(ctx, ")");
  emit_symbol(ctx, ";");
}
