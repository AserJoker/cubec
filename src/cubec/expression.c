#include "cubec/expression.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/type.h"
#include "cubec/expression_member.h"
#include "cubec/literal_char.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"

static void _cubec_expression_init(cubec_expression_t self,
                                   allocator_t allocator,
                                   cubec_expression_init_t *init) {
  node_init_t super_init = {
      .kind = 0,
      .parent = NULL,
  };
  if (init) {
    super_init.location = init->location;
    super_init.kind = init->kind;
  }
  g_node_type.init(&self->super, allocator, &super_init);
}

static void _cubec_expression_dispose(cubec_expression_t self,
                                      allocator_t allocator) {
  g_node_type.dispose(&self->super, allocator);
}

static void _cubec_expression_clone(cubec_expression_t self,
                                    allocator_t allocator,
                                    cubec_expression_t another) {
  g_node_type.clone(&self->super, allocator, &another->super);
}

static void _cubec_expression_move(cubec_expression_t self,
                                   allocator_t allocator,
                                   cubec_expression_t another) {
  g_node_type.move(&self->super, allocator, &another->super);
}

type_t g_cubec_expression_type = {
    .name = "cubec.cubec.expression",
    .size = sizeof(struct _cubec_expression_t),
    .init = (type_init_fn_t)_cubec_expression_init,
    .dispose = (type_dispose_fn_t)_cubec_expression_dispose,
    .clone = (type_clone_fn_t)_cubec_expression_clone,
    .move = (type_move_fn_t)_cubec_expression_move,
};

node_t read_atom(allocator_t allocator, vec_t tokens, size_t *position,
                 const char *filename) {
  size_t current = *position;
  node_t result = NULL;

  // Try string literal
  result =
      TRY(NULL, read_literal_string(allocator, tokens, &current, filename));
  if (result) {
    *position = current;
    return result;
  }

  // Try numeric literal
  result =
      TRY(NULL, read_literal_numeric(allocator, tokens, &current, filename));
  if (result) {
    *position = current;
    return result;
  }

  // Try identifier
  result =
      TRY(NULL, read_literal_identifier(allocator, tokens, &current, filename));
  if (result) {
    *position = current;
    return result;
  }

  // Try char literal
  result = TRY(NULL, read_literal_char(allocator, tokens, &current, filename));
  if (result) {
    *position = current;
    return result;
  }

  return NULL;
}

node_t read_value(allocator_t allocator, vec_t tokens, size_t *position,
                  const char *filename) {
  node_t node = NULL;
  node = TRY_LOCAL(onerror, read_atom(allocator, tokens, position, filename));
  if (node) {
    size_t current = *position;
    while (true) {
      /* Try postfix: member access <host>.<field> */
      node_t member_node = TRY_LOCAL(onerror,
          read_expression_member(allocator, tokens, &current, filename, node));
      if (!member_node) {
        break;
      }
      node = member_node;
      *position = current;
    }
  }
  return node;
onerror:
  allocator_free(allocator, node);
  return NULL;
}