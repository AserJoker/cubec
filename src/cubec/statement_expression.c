#include "cubec/statement_expression.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

static void
_cubec_statement_expression_init(cubec_statement_expression_t self,
                                 allocator_t allocator,
                                 cubec_statement_expression_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_EXPRESSION,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_class.init(&self->super, allocator, &super_init);
  self->expression = init->expression;
}

static void
_cubec_statement_expression_dispose(cubec_statement_expression_t self,
                                    allocator_t allocator) {
  allocator_free(allocator, &self->expression);
  g_node_class.dispose(&self->super, allocator);
}

static void
_cubec_statement_expression_clone(cubec_statement_expression_t self,
                                  allocator_t allocator,
                                  cubec_statement_expression_t another) {
  g_node_class.clone(&self->super, allocator, &another->super);
  self->expression = alloc_clone(allocator, another->expression);
  return;
}

static void
_cubec_statement_expression_move(cubec_statement_expression_t self,
                                 allocator_t allocator,
                                 cubec_statement_expression_t another) {
  g_node_class.move(&self->super, allocator, &another->super);
  self->expression = alloc_move(allocator, another->expression);
  return;
}

class_t g_cubec_statement_expression_class = {
    .name = "cubec.cubec.statement_expression",
    .size = sizeof(struct _cubec_statement_expression_t),
    .init = (class_init_fn_t)_cubec_statement_expression_init,
    .dispose = (class_dispose_fn_t)_cubec_statement_expression_dispose,
    .clone = (class_clone_fn_t)_cubec_statement_expression_clone,
    .move = (class_move_fn_t)_cubec_statement_expression_move,
};

node_t read_statement_expression(vm_t vm, vec_t tokens, size_t *position,
                                 const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;

  /* Try to parse the expression */
  node_t expr = read_expression(vm, tokens, &current, filename);
  if (node_is_error(expr))
    return expr;
  if (!expr) {
    return NULL;
  }
  location_t start_location = expr->location;

  /* Expect semicolon after the expression */
  skip_whitespace(tokens, &current);
  token_t semi = vec_get(tokens, current);
  if (!token_is(semi, CUBEC_TOKEN_SYMBOL, ";")) {
    goto onerror;
  }

  /* Build location spanning from expression start to semicolon */
  location_t start_loc = expr->location;
  location_t *end_loc = token_get_location(semi);
  location_t loc = {
      .begin = start_loc.begin,
      .end = end_loc->end,
      .filename = filename,
  };

  /* Consume the semicolon */
  current++;

  cubec_statement_expression_init_t init = {
      .location = loc,
      .parent = NULL,
      .expression = expr,
  };
  cubec_statement_expression_t node =
      allocator_create(allocator, &g_cubec_statement_expression_class, &init);
  *position = current;
  return &node->super;

onerror:
  allocator_free(allocator, &expr);
  return create_error(vm, start_location);
}

node_t create_statement_expression(vm_t vm, location_t loc, node_t expr) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_statement_expression_init_t init = {.expression = expr};
  return (node_t)allocator_create(alloc, &g_cubec_statement_expression_class,
                                  &init);
}

void emit_statement_expression(emit_context_t ctx, node_t node) {
  cubec_statement_expression_t expr = (cubec_statement_expression_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_expression(ctx, expr->expression);
  emit_symbol(ctx, ";");
}