#include "cubec/statement_empty.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/node_error.h"
#include "cubec/token.h"

static void _cubec_statement_empty_init(cubec_statement_empty_t self,
                                        allocator_t allocator,
                                        cubec_statement_empty_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_EMPTY,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_class.init(&self->super, allocator, &super_init);
}

static void _cubec_statement_empty_dispose(cubec_statement_empty_t self,
                                           allocator_t allocator) {
  g_node_class.dispose(&self->super, allocator);
}

static void _cubec_statement_empty_clone(cubec_statement_empty_t self,
                                         allocator_t allocator,
                                         cubec_statement_empty_t another) {
  g_node_class.clone(&self->super, allocator, &another->super);
}

static void _cubec_statement_empty_move(cubec_statement_empty_t self,
                                        allocator_t allocator,
                                        cubec_statement_empty_t another) {
  g_node_class.move(&self->super, allocator, &another->super);
}

class_t g_cubec_statement_empty_class = {
    .name = "cubec.cubec.statement_empty",
    .size = sizeof(struct _cubec_statement_empty_t),
    .init = (class_init_fn_t)_cubec_statement_empty_init,
    .dispose = (class_dispose_fn_t)_cubec_statement_empty_dispose,
    .clone = (class_clone_fn_t)_cubec_statement_empty_clone,
    .move = (class_move_fn_t)_cubec_statement_empty_move,
};

node_t read_statement_empty(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  location_t start_location = {0};
  token_t token = vec_get(tokens, current);
  if (!token_is(token, CUBEC_TOKEN_SYMBOL, ";")) {
    return NULL;
  }
  start_location = *token_get_location(token);
  start_location.filename = filename;
  location_t *location = token_get_location(token);
  cubec_statement_empty_init_t init = {
      .location = *location,
      .parent = NULL,
  };
  cubec_statement_empty_t node =
      allocator_create(allocator, &g_cubec_statement_empty_class, &init);
  node->super.location.filename = filename;
  current++;
  *position = current;
  return &node->super;
onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                       "expected ';'");
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

node_t create_statement_empty(context_t ctx, location_t loc) {
  allocator_t alloc = ctx->allocator;
  cubec_statement_empty_init_t init = {.location = loc, .parent = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_statement_empty_class, &init);
}

void emit_statement_empty(emit_context_t ctx, node_t node) {
  recover_comments_to(ctx, node->location.begin.offset);
  emit_symbol(ctx, ";");
}