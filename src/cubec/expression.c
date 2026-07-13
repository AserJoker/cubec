#include "cubec/expression.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/type.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/expression_call.h"
#include "cubec/expression_comma.h"
#include "cubec/expression_function.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_group.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_type_const.h"
#include "cubec/expression_type_group.h"
#include "cubec/expression_type_volatile.h"
#include "cubec/expression_member.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_postfix_unary.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_type_ternary.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_alignof.h"
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
  TRY_VOID_LOCAL(onerror, g_node_type.init(&self->super, allocator, &super_init));
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
  TRY_VOID_LOCAL(onerror, g_node_type.clone(&self->super, allocator, &another->super));
onerror:
  return;
}

static void _cubec_expression_move(cubec_expression_t self,
                                   allocator_t allocator,
                                   cubec_expression_t another) {
  TRY_VOID_LOCAL(onerror, g_node_type.move(&self->super, allocator, &another->super));
onerror:
  return;
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

  // Try initialize list: .<type>{<items>} or .{<items>}
  result = TRY(NULL,
               read_expression_initialize_list(allocator, tokens, &current, filename));
  if (result) {
    *position = current;
    return result;
  }

  // Try typeof: typeof(<expression>) — compile-time type computation
  result = TRY(NULL,
               read_expression_typeof(allocator, tokens, &current, filename));
  if (result) {
    *position = current;
    return result;
  }

  // Try sizeof: sizeof(<expression>) — compile-time size computation
  result = TRY(NULL,
               read_expression_sizeof(allocator, tokens, &current, filename));
  if (result) {
    *position = current;
    return result;
  }

  // Try alignof: alignof(<expression>) — compile-time alignment computation
  result = TRY(NULL,
               read_expression_alignof(allocator, tokens, &current, filename));
  if (result) {
    *position = current;
    return result;
  }

  // Try anonymous function: func |captures| [generic_params] (params) -> type { body }
  result = TRY(NULL,
               read_expression_function(allocator, tokens, &current, filename));
  if (result) {
    *position = current;
    return result;
  }

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
      if (member_node) {
        node = member_node;
        *position = current;
        continue;
      }

      /* Try postfix: namespace access <host>::<field> */
      node_t namespace_node = TRY_LOCAL(
          onerror, read_expression_namespace_access(allocator, tokens, &current,
                                                    filename, node));
      if (namespace_node) {
        node = namespace_node;
        *position = current;
        continue;
      }

      break;
      *position = current;
    }
  }
  return node;
onerror:
  allocator_free(allocator, &node);
  return NULL;
}
node_t read_type_expression_primary(allocator_t allocator, vec_t tokens,
                                    size_t *position, const char *filename) {
  /* Parse a primary type expression: identifier with optional namespace
   * access (::), generic instantiation, pointer/slice/array declaration,
   * and grouping. Uses :: for namespace navigation (e.g. std::vec::Vec)
   * and . is NOT used in type expressions (only in normal expressions for
   * member access). This ensures:
   *   *std::vec::Vec → *(std::vec::Vec) — pointer to namespaced type
   *   []i32 → slice of i32
   *   Vec[i32]::Element → nested type in generic instantiation
   */

  node_t node = NULL;
  size_t current = *position;

  /* Try grouped type expression: ( type_expression ) */
  node = TRY_LOCAL(onerror,
                   read_expression_type_group(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try const type expression (prefix form: const <type>) */
  node = TRY_LOCAL(onerror,
                   read_expression_type_const(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try volatile type expression (prefix form: volatile <type>) */
  node = TRY_LOCAL(onerror,
                   read_expression_type_volatile(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try pointer declaration (prefix form: * [const] [volatile] <type>) */
  node = TRY_LOCAL(onerror,
                   read_declaration_pointer(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try array declaration (prefix form: [ <expr> ] [const] [volatile] <type>) */
  node = TRY_LOCAL(onerror,
                   read_declaration_array(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try slice declaration (prefix form: [] [const] [volatile] <type>) */
  node = TRY_LOCAL(onerror,
                   read_declaration_slice(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    return node;
  }

  /* Try identifier as the base type */
  if (!node) {
    node = read_literal_identifier(allocator, tokens, &current, filename);
  }

  if (!node) {
    return NULL;
  }

  *position = current;
  /* Process postfix operators: namespace access (::), generic instantiation,
   * and pointer declaration */
  while (true) {
    skip_whitespace(tokens, &current);

    /* Try postfix: namespace access <host>::<field> */
    node_t namespace_node = TRY_LOCAL(
        onerror, read_expression_namespace_access(allocator, tokens,
                                                   &current, filename, node));
    if (namespace_node) {
      node = namespace_node;
      *position = current;
      continue;
    }

    /* Try postfix: generic instantiation <callee>[<args>] */
    node_t generic_node = TRY_LOCAL(
        onerror, read_expression_generic_instantiation(allocator, tokens,
                                                       &current, filename, node));
    if (generic_node) {
      node = generic_node;
      *position = current;
      continue;
    }

    /* Try postfix: pointer declaration (chained, e.g., i32 *) */
    node_t pointer_node = TRY_LOCAL(
        onerror, read_declaration_pointer(allocator, tokens, &current, filename));
    if (pointer_node) {
      node = pointer_node;
      *position = current;
      continue;
    }

    break;
  }

  return node;

onerror:
  allocator_free(allocator, &node);
  return NULL;
}

node_t read_expression_type(allocator_t allocator, vec_t tokens,
                            size_t *position, const char *filename) {
  /* Try top-level type expressions first: typeof, ternary.
   * typeof is NOT a sub-type of pointer/slice/array declarations —
   * it is a top-level type expression like ternary conditionals. */
  node_t node = NULL;
  size_t current = *position;

  /* Try typeof type expression: typeof(<expression>) */
  node = TRY_LOCAL(onerror,
                   read_expression_typeof(allocator, tokens, &current, filename));
  if (node) {
    *position = current;
    /* Process postfix operators for typeof: namespace access, generic
     * instantiation. Pointer/slice/array cannot wrap typeof. */
    while (true) {
      skip_whitespace(tokens, &current);

      /* Try postfix: namespace access <host>::<field> */
      node_t namespace_node = TRY_LOCAL(
          onerror, read_expression_namespace_access(allocator, tokens,
                                                     &current, filename, node));
      if (namespace_node) {
        node = namespace_node;
        *position = current;
        continue;
      }

      /* Try postfix: generic instantiation <callee>[<args>] */
      node_t generic_node = TRY_LOCAL(
          onerror, read_expression_generic_instantiation(allocator, tokens,
                                                         &current, filename, node));
      if (generic_node) {
        node = generic_node;
        *position = current;
        continue;
      }

      break;
    }
    return node;
  }

  /* Try ternary type expression: cond ? type_a : type_b.
   * If read_expression_type_ternary returns NULL (no type expression at all),
   * fall back to read_type_expression_primary. */
  current = *position;
  node = read_expression_type_ternary(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }
  return read_type_expression_primary(allocator, tokens, position, filename);

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