#include "core/emit_context.h"
#include "core/token_writer.h"
#include "cubec/expression_ternary.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <inttypes.h>

/* Forward declaration for read_expression_binary */
extern node_t read_expression_binary(vm_t vm, vec_t tokens,
                                     size_t *position, const char *filename);

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void
_cubec_expression_ternary_init(cubec_expression_ternary_t self,
                               allocator_t allocator,
                               cubec_expression_ternary_init_t *init) {
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_TERNARY,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;
  g_cubec_expression_class.init(&self->super, allocator, &super_init);

  self->condition = init->condition;
  self->consequent = init->consequent;
  self->alternate = init->alternate;
}

static void _cubec_expression_ternary_dispose(cubec_expression_ternary_t self,
                                              allocator_t allocator) {
  allocator_free(allocator, &self->condition);
  allocator_free(allocator, &self->consequent);
  allocator_free(allocator, &self->alternate);
  g_cubec_expression_class.dispose(&self->super, allocator);
}

static void
_cubec_expression_ternary_clone(cubec_expression_ternary_t self,
                                allocator_t allocator,
                                cubec_expression_ternary_t another) {
  g_cubec_expression_class.clone(&self->super, allocator, &another->super);
  self->condition = alloc_clone(allocator, another->condition);
  self->consequent = alloc_clone(allocator, another->consequent);
  self->alternate = alloc_clone(allocator, another->alternate);
  return;

cleanup:
  allocator_free(allocator, &self->alternate);
  allocator_free(allocator, &self->consequent);
  allocator_free(allocator, &self->condition);
}

static void _cubec_expression_ternary_move(cubec_expression_ternary_t self,
                                           allocator_t allocator,
                                           cubec_expression_ternary_t another) {
  g_cubec_expression_class.move(&self->super, allocator, &another->super);
  self->condition = alloc_move(allocator, another->condition);
  self->consequent = alloc_move(allocator, another->consequent);
  self->alternate = alloc_move(allocator, another->alternate);
  return;

cleanup:
  allocator_free(allocator, &self->alternate);
  allocator_free(allocator, &self->consequent);
  allocator_free(allocator, &self->condition);
}

class_t g_cubec_expression_ternary_class = {
    .name = "cubec.cubec.expression_ternary",
    .size = sizeof(struct _cubec_expression_ternary_t),
    .init = (class_init_fn_t)_cubec_expression_ternary_init,
    .dispose = (class_dispose_fn_t)_cubec_expression_ternary_dispose,
    .clone = (class_clone_fn_t)_cubec_expression_ternary_clone,
    .move = (class_move_fn_t)_cubec_expression_ternary_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_ternary
 * -------------------------------------------------------------------------- */

node_t read_expression_ternary(vm_t vm, vec_t tokens, size_t *position,
                               const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;
  node_t condition = NULL;
  cubec_expression_ternary_t node = NULL;
  node_t consequent = NULL;
  node_t alternate = NULL;

  /* Parse condition using read_expression_binary.
   * This handles all binary ops including ==, !=, extends.
   * If no '?' follows, returns the condition as-is. */
  condition = read_expression_binary(vm, tokens, &current, filename);
  if (node_is_error(condition))
    return condition;
  if (!condition) {
    return NULL;
  }

  /* Check if this is actually a ternary — expect '?' */
  skip_whitespace(tokens, &current);
  if (!token_is(vec_get(tokens, current), CUBEC_TOKEN_SYMBOL, "?")) {
    /* Not a ternary — return the condition as-is */
    *position = current;
    return condition;
  }
  current++; /* Consumed '?' — committed to parsing a ternary from here */
  location_t start_location = condition->location;
  start_location.filename = filename;

  /* Parse consequent expression (the true branch) */
  skip_whitespace(tokens, &current);
  consequent = read_expression(vm, tokens, &current, filename);
  if (node_is_error(consequent)) {
    allocator_free(allocator, &condition);
    return consequent;
  }
  if (!consequent) {
    goto onerror;
  }

  /* Expect ':' */
  skip_whitespace(tokens, &current);
  token_t colon = vec_get(tokens, current);
  if (!token_is(colon, CUBEC_TOKEN_SYMBOL, ":")) {
    goto onerror;
  }
  current++;

  /* Parse alternate expression (the false branch) via read_expression
   * to handle nested ternaries naturally */
  skip_whitespace(tokens, &current);
  alternate = read_expression(vm, tokens, &current, filename);
  if (node_is_error(alternate)) {
    allocator_free(allocator, &condition);
    allocator_free(allocator, &consequent);
    return alternate;
  }
  if (!alternate) {
    goto onerror;
  }

  node = allocator_create(allocator, &g_cubec_expression_ternary_class,
                          &(cubec_expression_ternary_init_t){
                              .condition = condition,
                              .consequent = consequent,
                              .alternate = alternate,
                          });

  /* Location spans from condition start to alternate end */
  {
    location_t loc = condition->location;
    loc.end = token_get_location(colon)->end;
    node->super.super.location = loc;
    node->super.super.location.filename = filename;
  }

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &condition);
  allocator_free(allocator, &alternate);
  allocator_free(allocator, &consequent);
  allocator_free(allocator, &node);
  return create_error(vm, start_location);
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_ternary
 * -------------------------------------------------------------------------- */

node_t create_expression_ternary(vm_t vm, location_t loc, node_t cond,
                                 node_t then_branch, node_t else_branch) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_expression_ternary_init_t init = {.location = loc,
                                          .parent = NULL,
                                          .condition = cond,
                                          .consequent = then_branch,
                                          .alternate = else_branch};
  return (node_t)allocator_create(alloc, &g_cubec_expression_ternary_class,
                                  &init);
}

/* --------------------------------------------------------------------------
 *  Writer: write_expression_ternary
 * -------------------------------------------------------------------------- */

void emit_expression_ternary(emit_context_t ctx, node_t node) {
  cubec_expression_ternary_t ternary = (cubec_expression_ternary_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_expression(ctx, ternary->condition);
  emit_space(ctx);
  emit_symbol(ctx, "?");
  emit_space(ctx);
  emit_expression(ctx, ternary->consequent);
  emit_space(ctx);
  emit_symbol(ctx, ":");
  emit_space(ctx);
  emit_expression(ctx, ternary->alternate);
}
