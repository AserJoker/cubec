#include "core/emit_context.h"
#include "core/token_writer.h"
#include "cubec/expression_wildcard.h"
#include "core/token.h"
#include "cubec/token.h"

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_expression_wildcard_init(cubec_expression_wildcard_t self,
                                            allocator_t allocator, void *arg) {
  (void)arg;
  cubec_expression_init_t super_init = {
      .kind = CUBEC_NODE_EXPRESSION_WILDCARD,
      .parent = NULL,
  };
  g_cubec_expression_class.init(&self->super, allocator, &super_init);
  self->is_tuple = false;
}

static void _cubec_expression_wildcard_dispose(cubec_expression_wildcard_t self,
                                                allocator_t allocator) {
  g_cubec_expression_class.dispose(&self->super, allocator);
}

static void _cubec_expression_wildcard_clone(cubec_expression_wildcard_t self,
                                              allocator_t allocator,
                                              cubec_expression_wildcard_t another) {
  g_cubec_expression_class.clone(&self->super, allocator, &another->super);
  self->is_tuple = another->is_tuple;
}

static void _cubec_expression_wildcard_move(cubec_expression_wildcard_t self,
                                             allocator_t allocator,
                                             cubec_expression_wildcard_t another) {
  g_cubec_expression_class.move(&self->super, allocator, &another->super);
  self->is_tuple = another->is_tuple;
}

class_t g_cubec_expression_wildcard_class = {
    .name = "cubec.cubec.expression_wildcard",
    .size = sizeof(struct _cubec_expression_wildcard_t),
    .init = (class_init_fn_t)_cubec_expression_wildcard_init,
    .dispose = (class_dispose_fn_t)_cubec_expression_wildcard_dispose,
    .clone = (class_clone_fn_t)_cubec_expression_wildcard_clone,
    .move = (class_move_fn_t)_cubec_expression_wildcard_move,
};

/* --------------------------------------------------------------------------
 *  Parser
 * -------------------------------------------------------------------------- */

node_t read_expression_wildcard(vm_t vm, vec_t tokens,
                                size_t *position, const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  size_t current = *position;

  skip_whitespace(tokens, &current);
  token_t tok = vec_get(tokens, current);
  if (!token_is(tok, CUBEC_TOKEN_SYMBOL, "?"))
    return NULL;

  current++; /* consume `?` */

  cubec_expression_wildcard_t node =
      (cubec_expression_wildcard_t)allocator_create(
          allocator, &g_cubec_expression_wildcard_class, NULL);
  if (!node) return NULL;

  location_t loc = *token_get_location(tok);
  loc.filename = filename;
  node->super.super.location = loc;
  node->is_tuple = false;

  *position = current;
  return (node_t)node;
}

/* --------------------------------------------------------------------------
 *  Factory: create_expression_wildcard
 * -------------------------------------------------------------------------- */

node_t create_expression_wildcard(vm_t vm, location_t loc,
                                   bool is_tuple) {
  allocator_t alloc = vm_get_allocator(vm);
  cubec_expression_wildcard_init_t init = {
      .location = loc,
      .parent = NULL,
      .is_tuple = is_tuple,
  };
  return (node_t)allocator_create(alloc, &g_cubec_expression_wildcard_class,
                                  &init);
}

/* --------------------------------------------------------------------------
 *  Writer: write_expression_wildcard
 * -------------------------------------------------------------------------- */

void emit_expression_wildcard(emit_context_t ctx, node_t node) {
  cubec_expression_wildcard_t w = (cubec_expression_wildcard_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  if (w->is_tuple) {
    emit_symbol(ctx, "<?>");
  } else {
    emit_symbol(ctx, "?");
  }
}
