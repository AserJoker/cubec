#include "cubec/literal_nil.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/token.h"

static void _cubec_literal_nil_init(cubec_literal_nil_t self,
                                    allocator_t allocator,
                                    cubec_literal_init_t *init) {
  if (!init)
    return;
  g_cubec_literal_type.init(&self->super, allocator, init);
}

static void _cubec_literal_nil_dispose(cubec_literal_nil_t self,
                                       allocator_t allocator) {
  g_cubec_literal_type.dispose(&self->super, allocator);
}

static void _cubec_literal_nil_clone(cubec_literal_nil_t self,
                                     allocator_t allocator,
                                     cubec_literal_nil_t another) {
  g_cubec_literal_type.clone(&self->super, allocator, &another->super);
}

static void _cubec_literal_nil_move(cubec_literal_nil_t self,
                                    allocator_t allocator,
                                    cubec_literal_nil_t another) {
  g_cubec_literal_type.move(&self->super, allocator, &another->super);
}

type_t g_cubec_literal_nil_type = {
    .name = "cubec.cubec.literal_nil",
    .size = sizeof(struct _cubec_literal_nil_t),
    .init = (type_init_fn_t)_cubec_literal_nil_init,
    .dispose = (type_dispose_fn_t)_cubec_literal_nil_dispose,
    .clone = (type_clone_fn_t)_cubec_literal_nil_clone,
    .move = (type_move_fn_t)_cubec_literal_nil_move,
};

node_t read_literal_nil(context_t ctx, vec_t tokens, size_t *position,
                        const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;

  token_t token = vec_get(tokens, current);
  if (!token_is(token, CUBEC_TOKEN_KEYWORD, "nil"))
    return NULL;

  location_t loc = *token_get_location(token);
  loc.filename = filename;
  current++;

  cubec_literal_init_t init = {
      .kind = CUBEC_NODE_LITERAL_NIL,
      .parent = NULL,
  };
  init.location = loc;

  cubec_literal_nil_t node =
      allocator_create(allocator, &g_cubec_literal_nil_type, &init);
  *position = current;
  return (node_t)node;
}

node_t create_literal_nil(context_t ctx, location_t loc) {
  allocator_t alloc = ctx->allocator;
  cubec_literal_init_t init = {
      .kind = CUBEC_NODE_LITERAL_NIL, .location = loc, .parent = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_literal_nil_type, &init);
}

void emit_literal_nil(emit_context_t ctx, node_t node) {
  recover_comments_to(ctx, node->location.begin.offset);
  emit_keyword(ctx, "nil");
}
