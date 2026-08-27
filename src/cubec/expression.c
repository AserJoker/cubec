#include "cubec/expression.h"
#include "core/emit_context.h"
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
#include "cubec/declaration_function.h"
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
#include "cubec/literal_nil.h"
#include "cubec/literal_bool.h"
#include "cubec/literal_undefined.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "core/token.h"
#include "cubec/token.h"
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
  g_node_class.init(&self->super, allocator, &super_init);
}

static void _cubec_expression_dispose(cubec_expression_t self,
                                      allocator_t allocator) {
  g_node_class.dispose(&self->super, allocator);
}

static void _cubec_expression_clone(cubec_expression_t self,
                                    allocator_t allocator,
                                    cubec_expression_t another) {
  g_node_class.clone(&self->super, allocator, &another->super);
}

static void _cubec_expression_move(cubec_expression_t self,
                                   allocator_t allocator,
                                   cubec_expression_t another) {
  g_node_class.move(&self->super, allocator, &another->super);
}

class_t g_cubec_expression_class = {
    .name = "cubec.cubec.expression",
    .size = sizeof(struct _cubec_expression_t),
    .init = (class_init_fn_t)_cubec_expression_init,
    .dispose = (class_dispose_fn_t)_cubec_expression_dispose,
    .clone = (class_clone_fn_t)_cubec_expression_clone,
    .move = (class_move_fn_t)_cubec_expression_move,
};

node_t read_atom(vm_t vm, vec_t tokens, size_t *position,
                 const char *filename) {
  size_t current = *position;
  node_t result = NULL;

  // Try initialize list: .<type>{<items>} or .{<items>}
  result = read_expression_initialize_list(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try typeof: typeof(<expression>) — compile-time type computation
  result = read_expression_typeof(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try sizeof: sizeof(<expression>) — compile-time size computation
  result = read_expression_sizeof(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try alignof: alignof(<expression>) — compile-time alignment computation
  result = read_expression_alignof(vm, tokens, &current, filename);
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
  result = read_declaration_callable(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try interface type: interface[generic_params] { members }
  result = read_declaration_interface(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try struct type: struct[generic_params] { members }
  result = read_declaration_struct(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try enum type: enum { items }
  result = read_declaration_enum(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try union type: union[generic_params] { fields }
  result = read_declaration_union(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try anonymous function: func |captures| [generic_params] (params) -> type {
  // body }
  result = read_declaration_function(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try grouped expression: ( expr )
  result = read_expression_group(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try type qualifier: const/volatile <type> — prefix type modifications
  result = read_declaration_qualifier(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try tuple type: <type1, type2, ...> — prefix type expression
  result = read_declaration_tuple(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try pointer declaration: * [const] [volatile] <type>
  // Note: * in atom position is always a pointer (not deref — deref uses .*)
  result = read_declaration_pointer(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try slice declaration: [] [const] [volatile] <type>
  result = read_declaration_slice(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try array declaration: [ <expr> ] [const] [volatile] <type>
  result = read_declaration_array(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try string literal
  result = read_literal_string(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try numeric literal
  result = read_literal_numeric(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try nil literal
  result = read_literal_nil(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try bool literal (true/false)
  result = read_literal_bool(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try undefined literal
  result = read_literal_undefined(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try prefix namespace access: ::identifier (current-module scope)
  {
    size_t saved = current;
    skip_whitespace(tokens, &saved);
    token_t tok = vec_get(tokens, saved);
    if (token_is(tok, CUBEC_TOKEN_SYMBOL, "::")) {
      /* Prefix :: — create namespace_access with host=NULL */
      size_t ns_pos = saved;
      ns_pos++; /* skip '::' */
      skip_whitespace(tokens, &ns_pos);
      node_t field_node =
          read_literal_identifier(vm, tokens, &ns_pos, filename);
      if (field_node) {
        cubec_expression_namespace_access_t ns =
            allocator_create(vm_get_allocator(vm),
                             &g_cubec_expression_namespace_access_class,
                             &(cubec_expression_namespace_access_init_t){
                                 .host = NULL,
                                 .field = (cubec_literal_identifier_t)field_node,
                             });
        location_t *loc = token_get_location(tok);
        ns->super.super.location = *loc;
        ns->super.super.location.filename = filename;
        *position = ns_pos;
        return (node_t)ns;
      }
      /* '::' without identifier — not ours, restore */
    }
  }

  // Try identifier
  result = read_literal_identifier(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try char literal
  result = read_literal_char(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  // Try wildcard type: ? (in type context, e.g. []? or *?)
  result = read_expression_wildcard(vm, tokens, &current, filename);
  if (node_is_error(result))
    return result;
  if (result) {
    *position = current;
    return result;
  }

  return NULL;
}

node_t read_value(vm_t vm, vec_t tokens, size_t *position,
                  const char *filename) {
  allocator_t allocator = vm_get_allocator(vm);
  node_t node = NULL;

  node = read_atom(vm, tokens, position, filename);
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
          read_expression_call(vm, tokens, &current, filename, node);
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
       * subscript because arr[0:10] would otherwise be incorrectly parsed
       * as subscript with argument "0" followed by ":" error */
      node_t slice_node =
          read_expression_slice(vm, tokens, &current, filename, node);
      if (node_is_error(slice_node)) {
        allocator_free(allocator, &node);
        return slice_node;
      }
      if (slice_node) {
        node = slice_node;
        *position = current;
        continue;
      }

      /* Try postfix: subscript <host>[<args>] — unified bracket syntax for
       * subscript access and generic instantiation (callee[Type]); the two
       * are disambiguated later during semantic analysis. */
      node_t subscript_node =
          read_expression_subscript(vm, tokens, &current, filename, node);
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
          read_expression_addr(vm, tokens, &current, filename, node);
      if (!postfix_unary_node)
        postfix_unary_node =
            read_expression_deref(vm, tokens, &current, filename, node);
      if (!postfix_unary_node)
        postfix_unary_node =
            read_expression_try(vm, tokens, &current, filename, node);
      if (!postfix_unary_node)
        postfix_unary_node =
            read_expression_assert(vm, tokens, &current, filename, node);
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
          read_expression_member(vm, tokens, &current, filename, node);
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
          vm, tokens, &current, filename, node);
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
node_t read_type_expression_primary(vm_t vm, vec_t tokens,
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
  node = read_expression_group(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try typeof: typeof(<expression>) — compile-time type computation */
  node = read_expression_typeof(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try sizeof: sizeof(<expression>) — compile-time size computation */
  node = read_expression_sizeof(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try alignof: alignof(<expression>) — compile-time alignment computation */
  node = read_expression_alignof(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try function type expression: func(type_list) -> type */
  node = read_declaration_callable(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try interface type expression: interface[generic_params] { members } */
  node = read_declaration_interface(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }
  /* Try struct type expression: struct[generic_params] { members } */
  node = read_declaration_struct(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }
  /* Try enum type expression: enum { items } */
  node = read_declaration_enum(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }
  /* Try union type expression: union[generic_params] { fields } */
  node = read_declaration_union(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }
  /* Try type qualifier expression (prefix form: const/volatile <type>) */
  node = read_declaration_qualifier(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try tuple type expression (prefix form: <type1, type2, ...>) */
  node = read_declaration_tuple(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try pointer declaration (prefix form: * [const] [volatile] <type>) */
  node = read_declaration_pointer(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try array declaration (prefix form: [ <expr> ] [const] [volatile] <type>)
   */
  node = read_declaration_array(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try slice declaration (prefix form: [] [const] [volatile] <type>) */
  node = read_declaration_slice(vm, tokens, &current, filename);
  if (node_is_error(node))
    return node;
  if (node) {
    *position = current;
    return node;
  }

  /* Try identifier as the base type */
  if (!node) {
    node = read_literal_identifier(vm, tokens, &current, filename);
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
        read_expression_namespace_access(vm, tokens, &current, filename, node);
    if (node_is_error(namespace_node)) {
      allocator_free(vm_get_allocator(vm), &node);
      return namespace_node;
    }
    if (namespace_node) {
      node = namespace_node;
      *position = current;
      continue;
    }

    /* Try postfix: subscript <host>[<args>] — unified bracket syntax for
     * subscript access and generic instantiation (callee[Type]); disambiguated
     * later during semantic analysis. */
    node_t subscript_node = read_expression_subscript(
        vm, tokens, &current, filename, node);
    if (node_is_error(subscript_node)) {
      allocator_free(vm_get_allocator(vm), &node);
      return subscript_node;
    }
    if (subscript_node) {
      node = subscript_node;
      *position = current;
      continue;
    }

    /* Try postfix: pointer declaration (chained, e.g., i32 *) */
    node_t pointer_node =
        read_declaration_pointer(vm, tokens, &current, filename);
    if (node_is_error(pointer_node)) {
      allocator_free(vm_get_allocator(vm), &node);
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

node_t read_expression_type(vm_t vm, vec_t tokens, size_t *position,
                            const char *filename) {
  /* handles all type and value constructs since read_atom() includes:
   * - Type-only: pointer/slice/array/const/volatile/func type
   * - Value: typeof/sizeof/alignof/identifiers/literals/groups/functions
   * - Binary ops: extends/==/!= (in expression_binary)
   * - Ternary: condition ? type_a : type_b (in expression_ternary)
   */
  return read_expression(vm, tokens, position, filename);
}

node_t read_expression_base(vm_t vm, vec_t tokens, size_t *position,
                            const char *filename) {
  /* Used as the "inner" parser by read_expression_assignment for rvalue
   * and by read_expression_type for type contexts. */
  return read_expression_ternary(vm, tokens, position, filename);
}

node_t read_expression(vm_t vm, vec_t tokens, size_t *position,
                       const char *filename) {
  /* read_expression_comma internally tries assignment then ternary,
   * so this single call covers the entire expression grammar. */
  return read_expression_comma(vm, tokens, position, filename);
}

/* --------------------------------------------------------------------------
 *  Emit dispatcher: emit_expression
 * -------------------------------------------------------------------------- */

void emit_expression(emit_context_t ctx, node_t expr) {
  switch (expr->kind) {
  case CUBEC_NODE_LITERAL_IDENTIFIER:
    emit_literal_identifier(ctx, expr);
    break;
  case CUBEC_NODE_LITERAL_NUMERIC:
    emit_literal_numeric(ctx, expr);
    break;
  case CUBEC_NODE_LITERAL_STRING:
    emit_literal_string(ctx, expr);
    break;
  case CUBEC_NODE_LITERAL_CHAR:
    emit_literal_char(ctx, expr);
    break;
  case CUBEC_NODE_LITERAL_NIL:
    emit_literal_nil(ctx, expr);
    break;
  case CUBEC_NODE_LITERAL_UNDEFINED:
    emit_literal_undefined(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_COMMA:
    emit_expression_comma(ctx, expr);
    break;
  case CUBEC_NODE_DECLARATION_ARRAY:
    emit_declaration_array(ctx, expr);
    break;
  case CUBEC_NODE_DECLARATION_POINTER:
    emit_declaration_pointer(ctx, expr);
    break;
  case CUBEC_NODE_DECLARATION_SLICE:
    emit_declaration_slice(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_ADDR:
    emit_expression_addr(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_ALIGNOF:
    emit_expression_alignof(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_ASSERT:
    emit_expression_assert(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT:
    emit_expression_assignment(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_BINARY:
    emit_expression_binary(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_CALL:
    emit_expression_call(ctx, expr);
    break;
  case CUBEC_NODE_DECLARATION_CALLABLE:
    emit_declaration_callable(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_DEREF:
    emit_expression_deref(ctx, expr);
    break;
  case CUBEC_NODE_DECLARATION_FUNCTION:
    emit_declaration_function(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS:
    emit_expression_namespace_access(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_SIZEOF:
    emit_expression_sizeof(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_SLICE:
    emit_expression_slice(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_SPREAD:
    emit_expression_spread(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_SUBSCRIPT:
    emit_expression_subscript(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_TERNARY:
    emit_expression_ternary(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_TRY:
    emit_expression_try(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_TYPEOF:
    emit_expression_typeof(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_WILDCARD:
    emit_expression_wildcard(ctx, expr);
    break;
  case CUBEC_NODE_DECLARATION_QUALIFIER:
    emit_declaration_qualifier(ctx, expr);
    break;
  case CUBEC_NODE_DECLARATION_ENUM:
    emit_declaration_enum(ctx, expr);
    break;
  case CUBEC_NODE_DECLARATION_UNION:
    emit_declaration_union(ctx, expr);
    break;
  case CUBEC_NODE_DECLARATION_INTERFACE:
    emit_declaration_interface(ctx, expr);
    break;
  case CUBEC_NODE_DECLARATION_STRUCT:
    emit_declaration_struct(ctx, expr);
    break;
  case CUBEC_NODE_DECLARATION_TUPLE:
    emit_declaration_tuple(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_GROUP:
    emit_expression_group(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_MEMBER:
    emit_expression_member(ctx, expr);
    break;
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST:
    emit_expression_initialize_list(ctx, expr);
    break;
  case CUBEC_NODE_ERROR:
    /* TODO: emit_node_error */
    break;
  }
}