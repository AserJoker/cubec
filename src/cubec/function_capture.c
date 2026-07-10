#include "cubec/function_capture.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/literal_identifier.h"
#include "cubec/node.h"
#include "cubec/token.h"
#include <inttypes.h>

/* --------------------------------------------------------------------------
 *  Lifecycle: init / dispose / clone / move
 * -------------------------------------------------------------------------- */

static void _cubec_function_capture_init(
    cubec_function_capture_t self, allocator_t allocator,
    cubec_function_capture_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = CUBEC_NODE_FUNCTION_BINDING,
      .parent = NULL,
  };
  super_init.location = init->location;
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
  self->identifier = init->identifier;
onerror:
  return;
}

static void _cubec_function_capture_dispose(
    cubec_function_capture_t self, allocator_t allocator) {
  allocator_free(allocator, &self->identifier);
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_function_capture_clone(
    cubec_function_capture_t self, allocator_t allocator,
    cubec_function_capture_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
  self->identifier = TRY_LOCAL(onerror, value_clone(allocator, another->identifier));
onerror:
  return;
}

static void _cubec_function_capture_move(
    cubec_function_capture_t self, allocator_t allocator,
    cubec_function_capture_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
  self->identifier = TRY_LOCAL(onerror, value_move(allocator, another->identifier));
onerror:
  return;
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

node_t read_function_capture(allocator_t allocator, vec_t tokens,
                              size_t *position, const char *filename) {
  size_t current = *position;
  node_t identifier = NULL;

  /* 1. Check for identifier */
  token_t first = vec_get(tokens, current);
  if (!first || token_get_kind(first) != CUBEC_TOKEN_IDENTIFIER) {
    return NULL;
  }

  /* 2. Parse identifier */
  identifier = TRY_LOCAL(fail, read_literal_identifier(allocator, tokens, &current, filename));
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
  cap = TRY_LOCAL(fail, allocator_create(allocator, &g_cubec_function_capture_type,
      &(cubec_function_capture_init_t){
          .location = loc,
          .identifier = identifier,
      }));

  *position = current;
  return (node_t)&cap->super;

fail:
  allocator_free(allocator, &identifier);
  return NULL;
}
