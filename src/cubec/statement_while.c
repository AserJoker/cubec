#include "cubec/statement_while.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node_error.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_while_init(cubec_statement_while_t self,
                                        allocator_t allocator,
                                        cubec_statement_while_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_WHILE,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_class.init(&self->super, allocator, &super_init);
  self->condition = init->condition;
  self->body = init->body;
}

static void _cubec_statement_while_dispose(cubec_statement_while_t self,
                                           allocator_t allocator) {
  allocator_free(allocator, &self->body);
  allocator_free(allocator, &self->condition);
  g_node_class.dispose(&self->super, allocator);
}

static void _cubec_statement_while_clone(cubec_statement_while_t self,
                                         allocator_t allocator,
                                         cubec_statement_while_t another) {
  g_node_class.clone(&self->super, allocator, &another->super);
  self->condition = alloc_clone(allocator, another->condition);
  self->body = alloc_clone(allocator, another->body);
  return;
}

static void _cubec_statement_while_move(cubec_statement_while_t self,
                                        allocator_t allocator,
                                        cubec_statement_while_t another) {
  g_node_class.move(&self->super, allocator, &another->super);
  self->condition = alloc_move(allocator, another->condition);
  self->body = alloc_move(allocator, another->body);
  return;
}

class_t g_cubec_statement_while_class = {
    .name = "cubec.cubec.statement_while",
    .size = sizeof(struct _cubec_statement_while_t),
    .init = (class_init_fn_t)_cubec_statement_while_init,
    .dispose = (class_dispose_fn_t)_cubec_statement_while_dispose,
    .clone = (class_clone_fn_t)_cubec_statement_while_clone,
    .move = (class_move_fn_t)_cubec_statement_while_move,
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
 *  Parser: read_statement_while — while(condition) { }
 * -------------------------------------------------------------------------- */

node_t read_statement_while(vm_t vm, vec_t tokens, size_t *position,
                            const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;
  node_t condition = NULL;
  node_t body = NULL;
  cubec_statement_while_t node = NULL;

  /* 1. Expect 'while' keyword */
  if (!_is_keyword(tokens, current, "while")) {
    return NULL;
  }
  token_t while_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(while_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse condition */
  condition = read_expression(vm, tokens, &current, filename);
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

  /* 5. Parse body (any statement) */
  body = read_statement(vm, tokens, &current, filename);
  if (node_is_error(body)) {
    allocator_free(allocator, &condition);
    return body;
  }
  if (!body)
    goto onerror;

  /* 6. Build location */
  location_t loc = start_location;
  loc.end = body->location.end;

  cubec_statement_while_init_t init = {
      .location = loc,
      .parent = NULL,
      .condition = condition,
      .body = body,
  };
  node = allocator_create(allocator, &g_cubec_statement_while_class, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &body);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &node);
  return create_error(vm, start_location);
}

node_t create_create_while(vm_t vm, location_t loc, node_t cond,
                           node_t body) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_statement_while_init_t init = {.condition = cond, .body = body};
  return (node_t)allocator_create(alloc, &g_cubec_statement_while_class, &init);
}

void emit_statement_while(emit_context_t ctx, node_t node) {
  cubec_statement_while_t stmt = (cubec_statement_while_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "while");
  emit_space(ctx);
  emit_symbol(ctx, "(");
  emit_expression(ctx, stmt->condition);
  emit_symbol(ctx, ")");
  emit_space(ctx);
  emit_statement(ctx, stmt->body);
}
