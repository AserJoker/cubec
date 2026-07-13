#include "cubec/expression.h"
#include "core/allocator.h"
#include "core/error.h"
#include "core/node.h"
#include "core/type.h"
#include "cubec/token.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/expression_call.h"
#include "cubec/expression_comma.h"
#include "cubec/expression_function.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_group.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_type_qualifier.h"
#include "cubec/expression_type_function.h"
#include "cubec/expression_type_interface.h"
#include "cubec/expression_type_struct.h"
#include "cubec/expression_type_enum.h"
#include "cubec/expression_type_union.h"
#include "cubec/expression_member.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_postfix_unary.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_alignof.h"
#include "cubec/literal_char.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include <inttypes.h>

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

  // Try function type: func(type_list) -> type — must be before function
  // expression because func(i32) -> i32 (type) and func(x: i32): i32 { ... }
  // (expression) both start with 'func('. The type form uses '->' for return
  // type and has no body; the expression form uses ':' and has a body.
  // type_function returns NULL (without THROW) when it detects the expression
  // form (named params or ':' instead of '->').
  result = read_expression_type_function(allocator, tokens, &current, filename);
  if (result) {
    *position = current;
    return result;
  }

  // Try interface type: interface[generic_params] { members }
  result = read_expression_type_interface(allocator, tokens, &current, filename);
  if (result) {
    *position = current;
    return result;
  }

  // Try struct type: struct[generic_params] { members }
  result = read_expression_type_struct(allocator, tokens, &current, filename);
  if (result) {
    *position = current;
    return result;
  }

  // Try enum type: enum { items }
  result = read_expression_type_enum(allocator, tokens, &current, filename);
  if (result) {
    *position = current;
    return result;
  }

  // Try union type: union[generic_params] { fields }
  result = read_expression_type_union(allocator, tokens, &current, filename);
  if (result) {
    *position = current;
    return result;
  }

  // Try anonymous function: func |captures| [generic_params] (params) -> type { body }
  result = read_expression_function(allocator, tokens, &current, filename);
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

  // Try type qualifier: const/volatile <type> — prefix type modifiers
  result = read_expression_type_qualifier(allocator, tokens, &current, filename);
  if (result) {
    *position = current;
    return result;
  }

  // Try pointer declaration: * [const] [volatile] <type>
  // Note: * in atom position is always a pointer (not deref — deref uses .*)
  result = read_declaration_pointer(allocator, tokens, &current, filename);
  if (result) {
    *position = current;
    return result;
  }

  // Try slice declaration: [] [const] [volatile] <type>
  result = read_declaration_slice(allocator, tokens, &current, filename);
  if (result) {
    *position = current;
    return result;
  }

  // Try array declaration: [ <expr> ] [const] [volatile] <type>
  result = read_declaration_array(allocator, tokens, &current, filename);
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
   *
   * Each sub-parser returns NULL without setting g_error when it doesn't
   * match (e.g. no '*' for pointer, no 'const'/'volatile' for qualifier).
   * This allows sequential trying without needing error_clear().
   */

  node_t node = NULL;
  size_t current = *position;

  /* Try grouped expression: ( expression ) */
  node = read_expression_group(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try typeof: typeof(<expression>) — compile-time type computation */
  node = read_expression_typeof(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try sizeof: sizeof(<expression>) — compile-time size computation */
  node = read_expression_sizeof(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try alignof: alignof(<expression>) — compile-time alignment computation */
  node = read_expression_alignof(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try function type expression: func(type_list) -> type */
  node = read_expression_type_function(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try interface type expression: interface[generic_params] { members } */
  node = read_expression_type_interface(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }
  /* Try struct type expression: struct[generic_params] { members } */
  node = read_expression_type_struct(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }
  /* Try enum type expression: enum { items } */
  node = read_expression_type_enum(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }
  /* Try union type expression: union[generic_params] { fields } */
  node = read_expression_type_union(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }
  /* Try type qualifier expression (prefix form: const/volatile <type>) */
  node = read_expression_type_qualifier(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try pointer declaration (prefix form: * [const] [volatile] <type>) */
  node = read_declaration_pointer(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try array declaration (prefix form: [ <expr> ] [const] [volatile] <type>) */
  node = read_declaration_array(allocator, tokens, &current, filename);
  if (node) {
    *position = current;
    return node;
  }

  /* Try slice declaration (prefix form: [] [const] [volatile] <type>) */
  node = read_declaration_slice(allocator, tokens, &current, filename);
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
    node_t namespace_node =
        read_expression_namespace_access(allocator, tokens,
                                         &current, filename, node);
    if (namespace_node) {
      node = namespace_node;
      *position = current;
      continue;
    }

    /* Try postfix: generic instantiation <callee>[<args>] */
    node_t generic_node =
        read_expression_generic_instantiation(allocator, tokens,
                                              &current, filename, node);
    if (generic_node) {
      node = generic_node;
      *position = current;
      continue;
    }

    /* Try postfix: pointer declaration (chained, e.g., i32 *) */
    node_t pointer_node =
        read_declaration_pointer(allocator, tokens, &current, filename);
    if (pointer_node) {
      node = pointer_node;
      *position = current;
      continue;
    }

    break;
  }

  return node;
}


node_t read_expression_type(allocator_t allocator, vec_t tokens,
                            size_t *position, const char *filename) {
  /* Unified type expression parser. Now delegates to read_expression() which
   * handles all type and value constructs since read_atom() includes:
   * - Type-only: pointer/slice/array/const/volatile/func type
   * - Value: typeof/sizeof/alignof/identifiers/literals/groups/functions
   * - Binary ops: extends/==/!= (in expression_binary)
   * - Ternary: condition ? type_a : type_b (in expression_ternary)
   */
  return read_expression(allocator, tokens, position, filename);
}

node_t read_expression(allocator_t allocator, vec_t tokens, size_t *position,
                       const char *filename) {
  /* read_expression_ternary internally:
   * 1. Calls read_expression_binary to parse the condition
   * 2. If no '?' follows, returns the condition directly
   * 3. Otherwise parses consequent and alternate recursively */
  return read_expression_ternary(allocator, tokens, position, filename);
}