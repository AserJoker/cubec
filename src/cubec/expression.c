#include "cubec/expression.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/expression_addr.h"
#include "cubec/expression_alignof.h"
#include "cubec/expression_assert.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_call.h"
#include "cubec/declaration_callable.h"
#include "cubec/expression_comma.h"
#include "cubec/declaration_enum.h"
#include "cubec/declaration_interface.h"
#include "cubec/declaration_qualifier.h"
#include "cubec/declaration_struct.h"
#include "cubec/declaration_tuple.h"
#include "cubec/declaration_union.h"
#include "cubec/expression_deref.h"
#include "cubec/expression_function.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_group.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_member.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_spread.h"
#include "cubec/expression_subscript.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_try.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_wildcard.h"
#include "cubec/literal_char.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_undefined.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include <inttypes.h>

static void _cubec_expression_init(cubec_expression_t self,
                                   allocator_t allocator,
                                   cubec_expression_init_t *init) {
  if (!init)
    return;
  node_init_t super_init = {
      .kind = init->kind,
      .parent = NULL,
  };
  super_init.location = init->location;
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

node_t read_atom(context_t ctx, vec_t tokens, size_t *position,
                 const char *filename) {
  size_t current = *position;
  node_t result = NULL;

  // Try initialize list: .<type>{<items>} or .{<items>}
  result = read_expression_initialize_list(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try typeof: typeof(<expression>) — compile-time type computation
  result = read_expression_typeof(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try sizeof: sizeof(<expression>) — compile-time size computation
  result = read_expression_sizeof(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try alignof: alignof(<expression>) — compile-time alignment computation
  result = read_expression_alignof(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
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
  result = read_declaration_callable(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try interface type: interface[generic_params] { members }
  result = read_declaration_interface(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try struct type: struct[generic_params] { members }
  result = read_declaration_struct(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try enum type: enum { items }
  result = read_declaration_enum(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try union type: union[generic_params] { fields }
  result = read_declaration_union(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try anonymous function: func |captures| [generic_params] (params) -> type {
  // body }
  result = read_expression_function(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try grouped expression: ( expr )
  result = read_expression_group(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try type qualifier: const/volatile <type> — prefix type modifications
  result = read_declaration_qualifier(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try tuple type: <type1, type2, ...> — prefix type expression
  result = read_declaration_tuple(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try pointer declaration: * [const] [volatile] <type>
  // Note: * in atom position is always a pointer (not deref — deref uses .*)
  result = read_declaration_pointer(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try slice declaration: [] [const] [volatile] <type>
  result = read_declaration_slice(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try array declaration: [ <expr> ] [const] [volatile] <type>
  result = read_declaration_array(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try string literal
  result = read_literal_string(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try numeric literal
  result = read_literal_numeric(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try undefined literal
  result = read_literal_undefined(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try identifier
  result = read_literal_identifier(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try char literal
  result = read_literal_char(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try wildcard type: ? (in type context, e.g. []? or *?)
  result = read_expression_wildcard(ctx, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  return NULL;
}

node_t read_value(context_t ctx, vec_t tokens, size_t *position,
                  const char *filename) {
  allocator_t allocator = ctx->allocator;
  node_t node = NULL;

  node = read_atom(ctx, tokens, position, filename);
  if (!node)
    return NULL;
  if (node_is_error(node))
    return node;

  if (node) {
    size_t current = *position;
    while (true) {
      /* Skip whitespace/comments before postfix operator */
      skip_whitespace(tokens, &current);

      /* Try postfix: function call <callee>(<args>) */
      node_t call_node =
          read_expression_call(ctx, tokens, &current, filename, node);
      if (node_is_error(call_node)) {
        allocator_free(allocator, &node);
        return call_node;
      }
      if (call_node) {
        node = call_node;
        *position = current;
        continue;
      }

      /* Try postfix: slice expression <host>[start:length] - MUST be before
       * generic instantiation because arr[0:10] would otherwise be incorrectly
       * parsed as generic with argument "0" followed by ":" error */
      node_t slice_node =
          read_expression_slice(ctx, tokens, &current, filename, node);
      if (node_is_error(slice_node)) {
        allocator_free(allocator, &node);
        return slice_node;
      }
      if (slice_node) {
        node = slice_node;
        *position = current;
        continue;
      }

      /* Try postfix: generic instantiation <callee>[<args>] */
      node_t generic_instantiation_node = read_expression_generic_instantiation(
          ctx, tokens, &current, filename, node);
      if (node_is_error(generic_instantiation_node)) {
        allocator_free(allocator, &node);
        return generic_instantiation_node;
      }
      if (generic_instantiation_node) {
        node = generic_instantiation_node;
        *position = current;
        continue;
      }

      /* Try postfix: subscript <host>[<index>] — after generic instantiation
       * (identifier[Type] is generic, expr[index] is subscript) */
      node_t subscript_node =
          read_expression_subscript(ctx, tokens, &current, filename, node);
      if (node_is_error(subscript_node)) {
        allocator_free(allocator, &node);
        return subscript_node;
      }
      if (subscript_node) {
        node = subscript_node;
        *position = current;
        continue;
      }

      /* Try postfix: unary deref/addr/try/assert (MUST be before
       * member access since dot-asterisk/dot-amp/dot-qmark/dot-bang also start
       * with '.') */
      node_t postfix_unary_node =
          read_expression_addr(ctx, tokens, &current, filename, node);
      if (!postfix_unary_node)
        postfix_unary_node =
            read_expression_deref(ctx, tokens, &current, filename, node);
      if (!postfix_unary_node)
        postfix_unary_node =
            read_expression_try(ctx, tokens, &current, filename, node);
      if (!postfix_unary_node)
        postfix_unary_node =
            read_expression_assert(ctx, tokens, &current, filename, node);
      if (node_is_error(postfix_unary_node)) {
        allocator_free(allocator, &node);
        return postfix_unary_node;
      }
      if (postfix_unary_node) {
        node = postfix_unary_node;
        *position = current;
        continue;
      }

      /* Try postfix: member access <host>.<field> */
      node_t member_node =
          read_expression_member(ctx, tokens, &current, filename, node);
      if (node_is_error(member_node)) {
        allocator_free(allocator, &node);
        return member_node;
      }
      if (member_node) {
        node = member_node;
        *position = current;
        continue;
      }

      /* Try postfix: namespace access <host>::<field> */
      node_t namespace_node = read_expression_namespace_access(
          ctx, tokens, &current, filename, node);
      if (node_is_error(namespace_node)) {
        allocator_free(allocator, &node);
        return namespace_node;
      }
      if (namespace_node) {
        node = namespace_node;
        *position = current;
        continue;
      }

      break;
    }
  }
  return node;
}
node_t read_type_expression_primary(context_t ctx, vec_t tokens,
                                    size_t *position, const char *filename) {
  node_t node = NULL;
  size_t current = *position;

  /* Try namespace access (::), generic instantiation, pointer/slice/array
   * declaration, and grouping. Uses :: for namespace navigation (e.g.
   * std::vec::Vec) and . is NOT used in type expressions (only in normal
   * expressions for member access). This ensures: *std::vec::Vec →
   * *(std::vec::Vec) — pointer to namespaced type
   *   []i32 → slice of i32
   *   Vec[i32]::Element → nested type in generic instantiation
   */

  /* Try grouped expression: ( expression ) */
  node = read_expression_group(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try typeof: typeof(<expression>) — compile-time type computation */
  node = read_expression_typeof(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try sizeof: sizeof(<expression>) — compile-time size computation */
  node = read_expression_sizeof(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try alignof: alignof(<expression>) — compile-time alignment computation */
  node = read_expression_alignof(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try function type expression: func(type_list) -> type */
  node = read_declaration_callable(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try interface type expression: interface[generic_params] { members } */
  node = read_declaration_interface(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }
  /* Try struct type expression: struct[generic_params] { members } */
  node = read_declaration_struct(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }
  /* Try enum type expression: enum { items } */
  node = read_declaration_enum(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }
  /* Try union type expression: union[generic_params] { fields } */
  node = read_declaration_union(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }
  /* Try type qualifier expression (prefix form: const/volatile <type>) */
  node = read_declaration_qualifier(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try tuple type expression (prefix form: <type1, type2, ...>) */
  node = read_declaration_tuple(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try pointer declaration (prefix form: * [const] [volatile] <type>) */
  node = read_declaration_pointer(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try array declaration (prefix form: [ <expr> ] [const] [volatile] <type>)
   */
  node = read_declaration_array(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try slice declaration (prefix form: [] [const] [volatile] <type>) */
  node = read_declaration_slice(ctx, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try identifier as the base type */
  if (!node) {
    node = read_literal_identifier(ctx, tokens, &current, filename);
  }
  if (node_is_error(node))
    return node;

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
        read_expression_namespace_access(ctx, tokens, &current, filename, node);
    if (node_is_error(namespace_node)) {
      allocator_free(ctx->allocator, &node);
      return namespace_node;
    }
    if (namespace_node) {
      node = namespace_node;
      *position = current;
      continue;
    }

    /* Try postfix: generic instantiation <callee>[<args>] */
    node_t generic_node = read_expression_generic_instantiation(
        ctx, tokens, &current, filename, node);
    if (node_is_error(generic_node)) {
      allocator_free(ctx->allocator, &node);
      return generic_node;
    }
    if (generic_node) {
      node = generic_node;
      *position = current;
      continue;
    }

    /* Try postfix: pointer declaration (chained, e.g., i32 *) */
    node_t pointer_node =
        read_declaration_pointer(ctx, tokens, &current, filename);
    if (node_is_error(pointer_node)) {
      allocator_free(ctx->allocator, &node);
      return pointer_node;
    }
    if (pointer_node) {
      node = pointer_node;
      *position = current;
      continue;
    }

    break;
  }

  return node;
}

node_t read_expression_type(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename) {
  /* handles all type and value constructs since read_atom() includes:
   * - Type-only: pointer/slice/array/const/volatile/func type
   * - Value: typeof/sizeof/alignof/identifiers/literals/groups/functions
   * - Binary ops: extends/==/!= (in expression_binary)
   * - Ternary: condition ? type_a : type_b (in expression_ternary)
   */
  return read_expression(ctx, tokens, position, filename);
}

node_t read_expression_base(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename) {
  /* Used as the "inner" parser by read_expression_assignment for rvalue
   * and by read_expression_type for type contexts. */
  return read_expression_ternary(ctx, tokens, position, filename);
}

node_t read_expression(context_t ctx, vec_t tokens, size_t *position,
                       const char *filename) {
  /* read_expression_comma internally tries assignment then ternary,
   * so this single call covers the entire expression grammar. */
  return read_expression_comma(ctx, tokens, position, filename);
}

void write_expression(writer_t writer, node_t expr) {
  switch (expr->kind) {
  case CUBEC_NODE_EXPRESSION_COMMA:
    write_expression_comma(writer, expr);
    break;
  case CUBEC_NODE_DECLARATION_ARRAY:
    write_declaration_array(writer, expr);
    break;
  case CUBEC_NODE_DECLARATION_POINTER:
    write_declaration_pointer(writer, expr);
    break;
  case CUBEC_NODE_DECLARATION_SLICE:
    write_declaration_slice(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_ADDR:
    write_expression_addr(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_ALIGNOF:
    write_expression_alignof(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_ASSERT:
    write_expression_assert(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT:
    write_expression_assigment(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_BINARY:
    write_expression_binary(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_CALL:
    write_expression_call(writer, expr);
    break;
  case CUBEC_NODE_DECLARATION_CALLABLE:
    write_declaration_callable(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_DEREF:
    write_expression_deref(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_FUNCTION:
    write_expression_function(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION:
    write_expression_generic_instantiation(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS:
    write_expression_namespace_access(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_SIZEOF:
    write_expression_sizeof(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_SLICE:
    write_expression_slice(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_SPREAD:
    write_expression_spread(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_SUBSCRIPT:
    write_expression_subscript(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_TERNARY:
    write_expression_ternary(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_TRY:
    write_expression_try(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_TYPEOF:
    write_expression_typeof(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_WILDCARD:
    write_expression_wildcard(writer, expr);
    break;
  case CUBEC_NODE_LITERAL_CHAR:
    write_literal_char(writer, expr);
    break;
  case CUBEC_NODE_LITERAL_IDENTIFIER:
    write_literal_identifier(writer, expr);
    break;
  case CUBEC_NODE_LITERAL_NUMERIC:
    write_literal_numeric(writer, expr);
    break;
  case CUBEC_NODE_LITERAL_STRING:
    write_literal_string(writer, expr);
    break;
  case CUBEC_NODE_LITERAL_UNDEFINED:
    write_literal_undefined(writer, expr);
    break;
  case CUBEC_NODE_DECLARATION_QUALIFIER:
    write_declaration_qualifier(writer, expr);
    break;
  case CUBEC_NODE_DECLARATION_ENUM:
    write_declaration_enum(writer, expr);
    break;
  case CUBEC_NODE_DECLARATION_UNION:
    write_declaration_union(writer, expr);
    break;
  case CUBEC_NODE_DECLARATION_INTERFACE:
    write_declaration_interface(writer, expr);
    break;
  case CUBEC_NODE_DECLARATION_STRUCT:
    write_declaration_struct(writer, expr);
    break;
  case CUBEC_NODE_DECLARATION_TUPLE:
    write_declaration_tuple(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_GROUP:
    write_expression_group(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_MEMBER:
    write_expression_member(writer, expr);
    break;
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST:
    write_expression_initialize_list(writer, expr);
    break;
  case CUBEC_NODE_ERROR:
    write_node_error(writer, expr);
    break;
  }
}