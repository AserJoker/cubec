#include "cubec/expression.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/type.h"
#include "cubec/expression_call.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_group.h"
#include "cubec/expression_member.h"
#include "cubec/expression_postfix_unary.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_ternary.h"
#include "cubec/literal_char.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"

static void _cubec_expression_init(cubec_expression_t self,
                                   allocator_t allocator,
                                   cubec_expression_init_t *init) {
  if (!init) {
    THROW_LOCAL(onerror, "init cannot be NULL");
  }
  node_init_t super_init = {
      .kind = init->kind,
      .parent = NULL,
  };
  super_init.location = init->location;
  g_node_type.init(&self->super, allocator, &super_init);
onerror:
  return;
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

  // Try grouped expression: ( expr )
  result = TRY(NULL,
               read_expression_group(allocator, tokens, &current, filename));
  if (result) {
    *position = current;
    return result;
  }

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
      /* Skip whitespace/comments before postfix operator */
      skip_whitespace(tokens, &current);

      /* Try postfix: function call <callee>(<args>) */
      node_t call_node =
          TRY_LOCAL(onerror,
                    read_expression_call(allocator, tokens, &current, filename,
                                         node));
      if (call_node) {
        node = call_node;
        *position = current;
        continue;
      }

      /* Try postfix: slice expression <host>[start:length] - MUST be before
       * generic instantiation because arr[0:10] would otherwise be incorrectly
       * parsed as generic with argument "0" followed by ":" error */
      node_t slice_node =
          TRY_LOCAL(onerror,
                    read_expression_slice(allocator, tokens, &current, filename,
                                          node));
      if (slice_node) {
        node = slice_node;
        *position = current;
        continue;
      }

      /* Try postfix: generic instantiation <callee>[<args>] */
      node_t generic_instantiation_node =
          TRY_LOCAL(onerror,
                    read_expression_generic_instantiation(
                        allocator, tokens, &current, filename, node));
      if (generic_instantiation_node) {
        node = generic_instantiation_node;
        *position = current;
        continue;
      }

      /* Try postfix: unary deref/addr <value>.+ or <value>.& (MUST be before
       * member access since .* and .& also start with '.') */
      node_t postfix_unary_node = TRY_LOCAL(
          onerror, read_expression_postfix_unary(allocator, tokens, &current,
                                                 filename, node));
      if (postfix_unary_node) {
        node = postfix_unary_node;
        *position = current;
        continue;
      }

      /* Try postfix: member access <host>.<field> */
      node_t member_node =
          TRY_LOCAL(onerror, read_expression_member(allocator, tokens, &current,
                                                    filename, node));
      if (!member_node) {
        break;
      }
      node = member_node;
      *position = current;
    }
  }
  return node;
onerror:
  allocator_free(allocator, &node);
  return NULL;
}
node_t read_expression_type(allocator_t allocator, vec_t tokens,
                            size_t *position, const char *filename) {
  /* Parse a type expression: identifier with optional member access and
   * generic instantiation. Handles patterns like:
   *   - identifier (e.g., "Vec", "i32")
   *   - member (e.g., "std::vec::Vec")
   *   - generic instantiation (e.g., "Vec[i32]", "Option[T]")
   *
   * This is a simplified version of read_value focused on type syntax. */

  node_t node = NULL;

  /* Try identifier as the base type */
  node = TRY_LOCAL(onerror,
                   read_literal_identifier(allocator, tokens, position, filename));
  if (!node) {
    return NULL;
  }

  /* Process postfix operators: member access and generic instantiation */
  size_t current = *position;
  while (true) {
    skip_whitespace(tokens, &current);

    /* Try postfix: generic instantiation <callee>[<args>] */
    node_t generic_node = TRY_LOCAL(
        onerror, read_expression_generic_instantiation(allocator, tokens,
                                                       &current, filename, node));
    if (generic_node) {
      node = generic_node;
      *position = current;
      continue;
    }

    /* Try postfix: member access <host>.<field> */
    node_t member_node = TRY_LOCAL(onerror,
                                   read_expression_member(allocator, tokens,
                                                          &current, filename,
                                                          node));
    if (!member_node) {
      break;
    }
    node = member_node;
    *position = current;
  }

  return node;

onerror:
  allocator_free(allocator, &node);
  return NULL;
}

node_t read_expression(allocator_t allocator, vec_t tokens, size_t *position,
                       const char *filename) {
  /* read_expression_ternary internally:
   * 1. Calls read_expression_binary to parse the condition
   * 2. If no '?' follows, returns the condition directly
   * 3. Otherwise parses consequent and alternate recursively */
  return read_expression_ternary(allocator, tokens, position, filename);
}