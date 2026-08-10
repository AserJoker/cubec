#include "cubec/function_capture.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "core/token_writer.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_function_capture_init(cubec_function_capture_t self,
                                         allocator_t allocator,
                                         cubec_function_capture_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = CUBEC_NODE_FUNCTION_CAPTURE,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
  self->identifier = init->identifier;
}

static void _cubec_function_capture_dispose(cubec_function_capture_t self,
                                            allocator_t allocator) {
  allocator_free(allocator, &self->identifier);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_function_capture_clone(cubec_function_capture_t self,
                                          allocator_t allocator,
                                          cubec_function_capture_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
  self->identifier = alloc_clone(allocator, another->identifier);
}

static void _cubec_function_capture_move(cubec_function_capture_t self,
                                         allocator_t allocator,
                                         cubec_function_capture_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
  self->identifier = alloc_move(allocator, another->identifier);
}

type_t g_cubec_function_capture_type = {
    .name = "cubec.cubec.function_capture",
    .size = sizeof(struct _cubec_function_capture_t),
    .init = (type_init_fn_t)_cubec_function_capture_init,
    .dispose = (type_dispose_fn_t)_cubec_function_capture_dispose,
    .clone = (type_clone_fn_t)_cubec_function_capture_clone,
    .move = (type_move_fn_t)_cubec_function_capture_move,
};

/* --------------------------------------------------------------------------
 *  Parser: read_function_capture
 * -------------------------------------------------------------------------- */

node_t read_function_capture(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;
  node_t identifier = NULL;

  /* 1. Check for identifier */
  token_t first = vec_get(tokens, current);
  if (!first || token_get_kind(first) != CUBEC_TOKEN_IDENTIFIER) {
    return NULL;
  }

  /* 2. Parse identifier */
  identifier = read_literal_identifier(ctx, tokens, &current, filename);
  if (!identifier) {
    return NULL;
  }

  /* 3. Build location */
  location_t *start_loc = token_get_location(first);
  location_t loc = {
      .begin = start_loc->begin,
      .end = identifier->location.end,
      .filename = filename,
  };

  /* 4. Create node */
  cubec_function_capture_t cap = NULL;
  cap = allocator_create(allocator, &g_cubec_function_capture_type,
                         &(cubec_function_capture_init_t){
                             .location = loc,
                             .identifier = identifier,
                         });
  if (!cap)
    goto fail;

  *position = current;
  return (node_t)&cap->super;

fail:
  allocator_free(allocator, &identifier);
  return NULL;
}

/* --------------------------------------------------------------------------
 *  Factory: create_function_capture
 * -------------------------------------------------------------------------- */

node_t create_function_capture(context_t ctx, location_t loc,
                               const char *name) {
  allocator_t alloc = ctx->allocator;
  node_t name_node = create_literal_identifier(ctx, loc, name);
  cubec_function_capture_init_t init = {
      .location = loc,
      .identifier = name_node,
  };
  return (node_t)allocator_create(alloc, &g_cubec_function_capture_type, &init);
}

void emit_function_capture(emit_context_t ctx, node_t node) {
  cubec_function_capture_t cap = (cubec_function_capture_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_expression(ctx, cap->identifier);
}
