#include "cubec/statement_if.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node_error.h"
#include "cubec/statement.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_statement_if_init(cubec_statement_if_t self,
                                     allocator_t allocator,
                                     cubec_statement_if_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_IF,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_class.init(&self->super, allocator, &super_init);
  self->condition = init->condition;
  self->then_branch = init->then_branch;
  self->else_branch = init->else_branch;
}

static void _cubec_statement_if_dispose(cubec_statement_if_t self,
                                        allocator_t allocator) {
  allocator_free(allocator, &self->else_branch);
  allocator_free(allocator, &self->then_branch);
  allocator_free(allocator, &self->condition);
  g_node_class.dispose(&self->super, allocator);
}

static void _cubec_statement_if_clone(cubec_statement_if_t self,
                                      allocator_t allocator,
                                      cubec_statement_if_t another) {
  g_node_class.clone(&self->super, allocator, &another->super);
  self->condition = alloc_clone(allocator, another->condition);
  self->then_branch = alloc_clone(allocator, another->then_branch);
  self->else_branch = another->else_branch
                          ? alloc_clone(allocator, another->else_branch)
                          : NULL;
  return;
}

static void _cubec_statement_if_move(cubec_statement_if_t self,
                                     allocator_t allocator,
                                     cubec_statement_if_t another) {
  g_node_class.move(&self->super, allocator, &another->super);
  self->condition = alloc_move(allocator, another->condition);
  self->then_branch = alloc_move(allocator, another->then_branch);
  self->else_branch =
      another->else_branch ? alloc_move(allocator, another->else_branch) : NULL;
  return;
}

class_t g_cubec_statement_if_class = {
    .name = "cubec.cubec.statement_if",
    .size = sizeof(struct _cubec_statement_if_t),
    .init = (class_init_fn_t)_cubec_statement_if_init,
    .dispose = (class_dispose_fn_t)_cubec_statement_if_dispose,
    .clone = (class_clone_fn_t)_cubec_statement_if_clone,
    .move = (class_move_fn_t)_cubec_statement_if_move,
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
 *  Parser: read_statement_if — if(condition) { } [else { } | else if(...)]
 * -------------------------------------------------------------------------- */

node_t read_statement_if(vm_t vm, vec_t tokens, size_t *position,
                         const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;
  node_t condition = NULL;
  node_t then_branch = NULL;
  node_t else_branch = NULL;
  cubec_statement_if_t node = NULL;

  /* 1. Expect 'if' keyword */
  if (!_is_keyword(tokens, current, "if")) {
    return NULL;
  }
  token_t if_token = vec_get(tokens, current);
  location_t start_location = *token_get_location(if_token);
  start_location.filename = filename;
  current++;
  skip_whitespace(tokens, &current);

  /* 2. Expect '(' */
  if (!_is_symbol(tokens, current, "(")) {
    goto onerror;
  }
  current++;
  skip_whitespace(tokens, &current);

  /* 3. Parse condition expression */
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

  /* 5. Parse then branch (any statement) */
  then_branch = read_statement(vm, tokens, &current, filename);
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
      /* else if — parse as nested if statement */
      else_branch = read_statement_if(vm, tokens, &current, filename);
      if (node_is_error(else_branch)) {
        allocator_free(allocator, &condition);
        allocator_free(allocator, &then_branch);
        return else_branch;
      }
      if (!else_branch)
        goto onerror;
    } else {
      /* else <statement> */
      else_branch = read_statement(vm, tokens, &current, filename);
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

  cubec_statement_if_init_t init = {
      .location = loc,
      .parent = NULL,
      .condition = condition,
      .then_branch = then_branch,
      .else_branch = else_branch,
  };
  node = allocator_create(allocator, &g_cubec_statement_if_class, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &else_branch);
  allocator_free(allocator, &then_branch);
  allocator_free(allocator, &condition);
  allocator_free(allocator, &node);
  return create_error(vm, start_location);
}

node_t create_statement_if(vm_t vm, location_t loc, node_t cond,
                           node_t then_branch, node_t else_branch) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_statement_if_init_t init = {.location = loc,
                                    .parent = NULL,
                                    .condition = cond,
                                    .then_branch = then_branch,
                                    .else_branch = else_branch};
  return (node_t)allocator_create(alloc, &g_cubec_statement_if_class, &init);
}

void emit_statement_if(emit_context_t ctx, node_t node) {
  cubec_statement_if_t stmt = (cubec_statement_if_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "if");
  emit_space(ctx);
  emit_symbol(ctx, "(");
  emit_expression(ctx, stmt->condition);
  emit_symbol(ctx, ")");
  emit_space(ctx);
  emit_statement(ctx, stmt->then_branch);
  if (stmt->else_branch) {
    emit_space(ctx);
    recover_comments_to(ctx, stmt->else_branch->location.begin.offset);
    emit_keyword(ctx, "else");
    emit_space(ctx);
    emit_statement(ctx, stmt->else_branch);
  }
}
