#include "c/lower.h"
#include "c/c_ir.h"
#include "c/c_type.h"
#include "c/mangle.h"
#include "c/c_ir_expr_binary.h"
#include "c/c_ir_expr_unary.h"
#include "c/c_ir_expr_call.h"
#include "c/c_ir_expr_member.h"
#include "c/c_ir_expr_subscript.h"
#include "c/c_ir_expr_cast.h"
#include "c/c_ir_expr_ternary.h"
#include "c/c_ir_expr_compound.h"
#include "c/c_ir_expr_sizeof.h"
#include "c/c_ir_expr_alignof.h"
#include "c/c_ir_expr_literal.h"
#include "c/c_ir_expr_initializer.h"
#include "c/c_ir_stmt_stmt_expr.h"
#include "cubec/node.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_call.h"
#include "cubec/expression_member.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_initialize_field.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_group.h"
#include "cubec/expression_type_qualifier.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_alignof.h"
/* expression_try.h and expression_assert.h don't exist as standalone headers.
   .? and .! are handled via expression_postfix_unary (reuses expression_binary_t). */
#include "cubec/expression_postfix_unary.h"
#include "cubec/expression_comma.h"
#include "cubec/expression_spread.h"
#include "cubec/expression_postfix_unary.h"
#include "cubec/expression_function.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_char.h"
#include <stdio.h>
#include "engine/semantic_type.h"
#include "engine/symbol.h"
#include "engine/scope.h"
#include "engine/context.h"
#include "core/node.h"
#include <string.h>

/* Forward declarations */
c_type_t lower_type(allocator_t allocator, semantic_type_t type,
                     const char *module_hash);
c_ir_node_t lower_stmt(allocator_t allocator, context_t ctx, node_t node,
                        const char *module_hash);

/**
 * @brief Lower a cubec expression AST node to a C IR expression node.
 */
c_ir_node_t lower_expr(allocator_t allocator, context_t ctx, node_t node,
                        const char *module_hash) {
  if (!node) return NULL;

  /* Error nodes — skip */
  if (node->kind == CUBEC_NODE_ERROR || node->kind == CUBEC_NODE_STATEMENT_ERROR) {
    return NULL;
  }

  location_t loc = node->location;

  switch (node->kind) {

  /* ===== Literals ===== */
  case CUBEC_NODE_LITERAL_NUMERIC: {
    cubec_literal_numeric_t n = (cubec_literal_numeric_t)node;
    return (c_ir_node_t)c_ir_expr_numeric_create(allocator,
                                                    string_get(n->value), loc);
  }

  case CUBEC_NODE_LITERAL_STRING: {
    cubec_literal_string_t n = (cubec_literal_string_t)node;
    return (c_ir_node_t)c_ir_expr_string_create(allocator,
                                                   string_get(n->value), loc);
  }

  case CUBEC_NODE_LITERAL_IDENTIFIER: {
    cubec_literal_identifier_t n = (cubec_literal_identifier_t)node;
    /* TODO: name mangling for identifiers — for now use raw name */
    return (c_ir_node_t)c_ir_expr_ident_create(allocator,
                                                  string_get(n->value), loc);
  }

  case CUBEC_NODE_LITERAL_CHAR: {
    cubec_literal_char_t n = (cubec_literal_char_t)node;
    char buf[4];
    snprintf(buf, sizeof(buf), "'%c'", n->value);
    return (c_ir_node_t)c_ir_expr_char_create(allocator, buf, loc);
  }

  case CUBEC_NODE_LITERAL_UNDEFINED: {
    /* undefined → no initializer, emit a zero/null as placeholder */
    return (c_ir_node_t)c_ir_expr_null_create(allocator, loc);
  }

  /* ===== Binary expressions ===== */
  case CUBEC_NODE_EXPRESSION_BINARY: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    if (!n->left) {
      /* Prefix unary: !x, -x, ~x, &x, *x */
      c_ir_node_t operand = lower_expr(allocator, ctx, n->right, module_hash);
      const char *op = string_get(n->opt);
      /* Map cubec deref .* to C * and address .& to C & */
      if (strcmp(op, ".*") == 0) op = "*";
      else if (strcmp(op, ".&") == 0) op = "&";
      return (c_ir_node_t)c_ir_expr_unary_create(allocator, op, operand, true, loc);
    }
    c_ir_node_t left = lower_expr(allocator, ctx, n->left, module_hash);
    c_ir_node_t right = lower_expr(allocator, ctx, n->right, module_hash);
    return (c_ir_node_t)c_ir_expr_binary_create(allocator, string_get(n->opt),
                                                   left, right, loc);
  }

  /* ===== Postfix unary (x++, x--) ===== */
  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER: {
    /* Type qualifier expression — strip and lower the inner */
    /* This shouldn't appear at runtime, but handle gracefully */
    return NULL;
  }

  /* ===== Assignment ===== */
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT: {
    /* Assignment is a binary expression with =, +=, etc. */
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    c_ir_node_t left = lower_expr(allocator, ctx, n->left, module_hash);
    c_ir_node_t right = lower_expr(allocator, ctx, n->right, module_hash);
    return (c_ir_node_t)c_ir_expr_binary_create(allocator, string_get(n->opt),
                                                   left, right, loc);
  }

  /* ===== Call ===== */
  case CUBEC_NODE_EXPRESSION_CALL: {
    cubec_expression_call_t n = (cubec_expression_call_t)node;
    c_ir_node_t callee = lower_expr(allocator, ctx, n->callee, module_hash);
    vec_t args = allocator_create(allocator, &g_vec_type,
                                   &(vec_init_t){.auto_dispose = false});
    size_t count = n->arguments ? vec_get_size(n->arguments) : 0;
    for (size_t i = 0; i < count; i++) {
      node_t arg = vec_get(n->arguments, i);
      c_ir_node_t c_arg = lower_expr(allocator, ctx, arg, module_hash);
      if (c_arg) vec_push(args, c_arg);
    }
    return (c_ir_node_t)c_ir_expr_call_create(allocator, callee, args, loc);
  }

  /* ===== Member access ===== */
  case CUBEC_NODE_EXPRESSION_MEMBER: {
    cubec_expression_member_t n = (cubec_expression_member_t)node;
    c_ir_node_t obj = lower_expr(allocator, ctx, n->host, module_hash);
    const char *field = string_get(n->field->value);
    /* TODO: determine is_arrow based on semantic type of host */
    bool is_arrow = false;
    return (c_ir_node_t)c_ir_expr_member_create(allocator, obj, field,
                                                   is_arrow, loc);
  }

  /* ===== Namespace access (Type::member) ===== */
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS: {
    cubec_expression_namespace_access_t n = (cubec_expression_namespace_access_t)node;
    const char *field = string_get(n->field->value);
    /* Namespace access becomes a mangled identifier */
    string_t mangled = mangle_name(allocator, module_hash, field);
    c_ir_node_t ident = (c_ir_node_t)c_ir_expr_ident_create(allocator,
                                                               string_get(mangled), loc);
    allocator_free(allocator, &mangled);
    return ident;
  }

  /* ===== Ternary ===== */
  case CUBEC_NODE_EXPRESSION_TERNARY: {
    cubec_expression_ternary_t n = (cubec_expression_ternary_t)node;
    c_ir_node_t cond = lower_expr(allocator, ctx, n->condition, module_hash);
    c_ir_node_t cons = lower_expr(allocator, ctx, n->consequent, module_hash);
    c_ir_node_t alt = lower_expr(allocator, ctx, n->alternate, module_hash);
    return (c_ir_node_t)c_ir_expr_ternary_create(allocator, cond, cons, alt, loc);
  }

  /* ===== Initialize list (struct literal) ===== */
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST: {
    cubec_expression_initialize_list_t n = (cubec_expression_initialize_list_t)node;
    /* TODO: resolve type from n->type for compound literal */
    c_type_t type = c_type_primitive(allocator, "void"); /* placeholder */
    vec_t fields = allocator_create(allocator, &g_vec_type,
                                      &(vec_init_t){.auto_dispose = false});
    size_t count = n->items ? vec_get_size(n->items) : 0;
    for (size_t i = 0; i < count; i++) {
      node_t item = vec_get(n->items, i);
      if (item->kind == CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD) {
        cubec_expression_initialize_field_t f = (cubec_expression_initialize_field_t)item;
        const char *name = f->field ? string_get(f->field->value) : NULL;
        c_ir_node_t value = lower_expr(allocator, ctx, f->value, module_hash);
        c_ir_node_t init = (c_ir_node_t)c_ir_expr_initializer_create(allocator, name, value, item->location);
        vec_push(fields, init);
      } else {
        c_ir_node_t value = lower_expr(allocator, ctx, item, module_hash);
        c_ir_node_t init = (c_ir_node_t)c_ir_expr_initializer_create_indexed(allocator, i, value, item->location);
        vec_push(fields, init);
      }
    }
    return (c_ir_node_t)c_ir_expr_compound_create(allocator, type, fields, loc);
  }

  /* ===== Group (parenthesized) ===== */
  case CUBEC_NODE_EXPRESSION_GROUP: {
    /* Groups don't exist in C IR — just lower the inner expression */
    /* The group node wraps a single expression; need to find it */
    /* Groups are stored as a vec_t of one expression */
    return NULL; /* TODO: implement when group structure is clear */
  }

  /* ===== Sizeof / Alignof ===== */
  case CUBEC_NODE_EXPRESSION_SIZEOF: {
    /* TODO: resolve type from AST */
    c_type_t type = c_type_primitive(allocator, "void");
    return (c_ir_node_t)c_ir_expr_sizeof_create(allocator, type, loc);
  }

  case CUBEC_NODE_EXPRESSION_ALIGNOF: {
    c_type_t type = c_type_primitive(allocator, "void");
    return (c_ir_node_t)c_ir_expr_alignof_create(allocator, type, loc);
  }

  /* ===== Try (.? error propagation) ===== */
  case CUBEC_NODE_EXPRESSION_TRY: {
    /* .? maps to GCC statement expression with runtime check */
    /* TODO: implement full .? lowering */
    return NULL;
  }

  /* ===== Assert ===== */
  case CUBEC_NODE_EXPRESSION_ASSERT: {
    /* Assert maps to __builtin_expect + if + abort */
    /* TODO: implement */
    return NULL;
  }

  /* ===== Comma expression ===== */
  case CUBEC_NODE_EXPRESSION_COMMA: {
    /* Comma is a binary op */
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    c_ir_node_t left = lower_expr(allocator, ctx, n->left, module_hash);
    c_ir_node_t right = lower_expr(allocator, ctx, n->right, module_hash);
    return (c_ir_node_t)c_ir_expr_binary_create(allocator, ",", left, right, loc);
  }

  /* ===== Slice expression ===== */
  case CUBEC_NODE_EXPRESSION_SLICE: {
    /* TODO: slice lowering */
    return NULL;
  }

  /* ===== Generic instantiation ===== */
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION: {
    /* Monomorphized — should be resolved to concrete function */
    /* TODO: implement */
    return NULL;
  }

  /* ===== Anonymous function ===== */
  case CUBEC_NODE_EXPRESSION_FUNCTION: {
    /* TODO: lower to static function + function pointer */
    return NULL;
  }

  /* ===== Spread ===== */
  case CUBEC_NODE_EXPRESSION_SPREAD: {
    return NULL;
  }

  /* ===== Wildcard ===== */
  case CUBEC_NODE_EXPRESSION_WILDCARD: {
    return NULL;
  }

  /* ===== Type expressions — no runtime representation ===== */
  case CUBEC_NODE_EXPRESSION_TYPE_ENUM:
  case CUBEC_NODE_EXPRESSION_TYPE_UNION:
  case CUBEC_NODE_EXPRESSION_TYPE_FUNCTION:
  case CUBEC_NODE_EXPRESSION_TYPE_INTERFACE:
  case CUBEC_NODE_EXPRESSION_TYPE_STRUCT:
  case CUBEC_NODE_EXPRESSION_TYPE_TUPLE:
  case CUBEC_NODE_EXPRESSION_TYPEOF:
    return NULL;

  default:
    return NULL;
  }
}
