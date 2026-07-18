#include "cubec/literal_undefined.h"
#include "cubec/ast_factory_internal.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/ast_factory.h"
#include "cubec/literal.h"
#include "cubec/token.h"

static void _cubec_literal_undefined_init(cubec_literal_undefined_t self,
                                          allocator_t allocator,
                                          cubec_literal_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  TRY_VOID_LOCAL(onerror, g_cubec_literal_type.init(&self->super, allocator, init));
onerror:
  return;
}

static void _cubec_literal_undefined_dispose(cubec_literal_undefined_t self,
                                             allocator_t allocator) {
  g_cubec_literal_type.dispose(&self->super, allocator);
}

static void _cubec_literal_undefined_clone(cubec_literal_undefined_t self,
                                           allocator_t allocator,
                                           cubec_literal_undefined_t another) {
  TRY_VOID_LOCAL(onerror, g_cubec_literal_type.clone(&self->super, allocator, &another->super));
onerror:
  return;
}

static void _cubec_literal_undefined_move(cubec_literal_undefined_t self,
                                          allocator_t allocator,
                                          cubec_literal_undefined_t another) {
  TRY_VOID_LOCAL(cleanup, g_cubec_literal_type.move(&self->super, allocator, &another->super));
  return;

cleanup:
  return;
}

type_t g_cubec_literal_undefined_type = {
    .name = "cubec.cubec.literal_undefined",
    .size = sizeof(struct _cubec_literal_undefined_t),
    .init = (type_init_fn_t)_cubec_literal_undefined_init,
    .dispose = (type_dispose_fn_t)_cubec_literal_undefined_dispose,
    .clone = (type_clone_fn_t)_cubec_literal_undefined_clone,
    .move = (type_move_fn_t)_cubec_literal_undefined_move,
};

node_t read_literal_undefined(allocator_t allocator, vec_t tokens,
                               size_t *position, const char *filename) {
  size_t current = *position;

  token_t token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(token, CUBEC_TOKEN_KEYWORD, "undefined")) {
    return NULL;
  }

  location_t loc = *token_get_location(token);
  loc.filename = filename;
  current++;

  cubec_literal_init_t init = {
      .kind = CUBEC_NODE_LITERAL_UNDEFINED,
      .parent = NULL,
  };
  init.location = loc;

  cubec_literal_undefined_t node =
      TRY_LOCAL(onerror, allocator_create(allocator, &g_cubec_literal_undefined_type, &init));
  *position = current;
  return (node_t)node;

onerror:
  return NULL;
}

node_t cubec_ast_create_undefined(allocator_t alloc, location_t loc) {
  cubec_literal_init_t init = {.kind = CUBEC_NODE_LITERAL_UNDEFINED,
                                .location = loc, .parent = NULL};
  return (node_t)allocator_create(alloc, &g_cubec_literal_undefined_type, &init);
}
