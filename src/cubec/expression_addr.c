#include "core/emit_context.h"
#include "core/token_writer.h"
#include "cubec/expression_addr.h"
#include "core/token.h"
#include "cubec/expression.h"
#include "cubec/token.h"
#include <string.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_addr_init(cubec_expression_addr_t self,
                                        allocator_t allocator,
                                        cubec_expression_addr_init_t *init) {
  if (!init)
    return;

  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_ADDR,
      .parent = NULL,
  };
  super_init.location = init->location;
  super_init.parent = init->parent;

  g_cubec_expression_class.init(&self->super, allocator, &super_init);
  self->host = init->host;
}

static void _cubec_expression_addr_dispose(cubec_expression_addr_t self,
                                           allocator_t allocator) {
  allocator_free(allocator, &self->host);
  g_cubec_expression_class.dispose(&self->super, allocator);
}

static void _cubec_expression_addr_clone(cubec_expression_addr_t self,
                                         allocator_t allocator,
                                         cubec_expression_addr_t another) {
  g_cubec_expression_class.clone(&self->super, allocator, &another->super);
  self->host = alloc_clone(allocator, another->host);
}

static void _cubec_expression_addr_move(cubec_expression_addr_t self,
                                        allocator_t allocator,
                                        cubec_expression_addr_t another) {
  g_cubec_expression_class.move(&self->super, allocator, &another->super);
  self->host = alloc_move(allocator, another->host);
}

class_t g_cubec_expression_addr_class = {
    .name = "cubec.cubec.expression_addr",
    .size = sizeof(struct _cubec_expression_addr_t),
    .init = (class_init_fn_t)_cubec_expression_addr_init,
    .dispose = (class_dispose_fn_t)_cubec_expression_addr_dispose,
    .clone = (class_clone_fn_t)_cubec_expression_addr_clone,
    .move = (class_move_fn_t)_cubec_expression_addr_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_expression_addr
 * -------------------------------------------------------------------------- */

/**
 * Try to parse postfix address-of: <host>.&
 * Expects '.' token followed by '&' token.
 * Returns NULL if tokens don't match.
 */
node_t read_expression_addr(vm_t vm, vec_t tokens, size_t *position,
                            const char *filename, node_t host) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;
  cubec_expression_addr_t node = NULL;

  /* Expect '.' token first */
  token_t dot_token = vec_get(tokens, current);
  if (!token_is(dot_token, CUBEC_TOKEN_SYMBOL, ".")) {
    return NULL;
  }
  current++;

  /* Expect '&' after '.' */
  skip_whitespace(tokens, &current);
  token_t second_token = vec_get(tokens, current);
  if (!token_is(second_token, CUBEC_TOKEN_SYMBOL, "&")) {
    return NULL;
  }
  location_t start_location = *token_get_location(dot_token);
  start_location.filename = filename;
  current++;

  node = allocator_create(allocator, &g_cubec_expression_addr_class,
                          &(cubec_expression_addr_init_t){
                              .host = host,
                          });
  location_t *loc = token_get_location(dot_token);
  node->super.super.location = *loc;
  node->super.super.location.filename = filename;

  *position = current;
  return (node_t)node;

onerror:
  allocator_free(allocator, &node);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_addr
 * -------------------------------------------------------------------------- */

node_t create_expression_addr(vm_t vm, location_t loc, node_t host) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_expression_addr_init_t init = {
      .location = loc,
      .parent = NULL,
      .host = host,
  };
  return (node_t)allocator_create(alloc, &g_cubec_expression_addr_class, &init);
}

void emit_expression_addr(emit_context_t ctx, node_t node) {
  cubec_expression_addr_t addr = (cubec_expression_addr_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_expression(ctx, addr->host);
  emit_symbol(ctx, ".&");
}