#include "cubec/statement_empty.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/token.h"
#include "core/type.h"
#include "cubec/node.h"
#include "cubec/token.h"

static void _cubec_statement_empty_init(cubec_statement_empty_t self,
                                        allocator_t allocator,
                                        cubec_statement_empty_init_t *init) {
  node_init_t super_init = {
      .kind = CUBEC_NODE_STATEMENT_EMPTY,
      .parent = NULL,
  };
  if (init) {
    super_init.location = init->location;
  }
  g_node_type.init(&self->super, allocator, &super_init);
}

static void _cubec_statement_empty_dispose(cubec_statement_empty_t self,
                                           allocator_t allocator) {
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_statement_empty_clone(cubec_statement_empty_t self,
                                         allocator_t allocator,
                                         cubec_statement_empty_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
}

static void _cubec_statement_empty_move(cubec_statement_empty_t self,
                                        allocator_t allocator,
                                        cubec_statement_empty_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
}

type_t g_cubec_statement_empty_type = {
    .name = "cubec.cubec.statement_empty",
    .size = sizeof(struct _cubec_statement_empty_t),
    .init = (type_init_fn_t)_cubec_statement_empty_init,
    .dispose = (type_dispose_fn_t)_cubec_statement_empty_dispose,
    .clone = (type_clone_fn_t)_cubec_statement_empty_clone,
    .move = (type_move_fn_t)_cubec_statement_empty_move,
};

node_t read_statement_empty(allocator_t allocator, vec_t tokens, size_t *position,
                            const char *filename) {
  size_t current = *position;
  skip_whitespace(tokens, &current);
  token_t token = TRY_LOCAL(onerror, vec_get(tokens, current));
  if (!token_is(token, CUBEC_TOKEN_SYMBOL, ";")) {
    return NULL;
  }
  cubec_statement_empty_t node =
      allocator_create(allocator, &g_cubec_statement_empty_type, NULL);
  location_t *location = token_get_location(token);
  node->super.location = *location;
  node->super.location.filename = filename;
  current++;
  *position = current;
  return &node->super;
onerror:
  return NULL;
}
