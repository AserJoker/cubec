#include "c/lower.h"
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
#include "c/c_ir_stmt_local_decl.h"
#include "c/c_ir_stmt_return.h"
#include "c/c_ir_stmt_if.h"
#include "c/c_ir_stmt_expr.h"
#include "c/c_ir_stmt_block.h"
#include "c/c_ir_function.h"
#include "cubec/node.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_call.h"
#include "cubec/expression_member.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_initialize_field.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_subscript.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_group.h"
#include "cubec/expression_type_qualifier.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_alignof.h"
#include "cubec/expression_postfix_unary.h"
#include "cubec/expression_comma.h"
#include "cubec/expression_spread.h"
#include "cubec/expression_function.h"
#include "cubec/statement_function.h"
#include "cubec/function_argument.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_char.h"
#include "cubec/literal_identifier.h"
#include "engine/resolver.h"
#include <string.h>
#include <inttypes.h>

/* Forward declarations */
c_type_t lower_type(allocator_t allocator, semantic_type_t type,
                     const char *module_hash, c_ir_unit_t unit);
c_ir_node_t lower_stmt(allocator_t allocator, context_t ctx, node_t node,
                        const char *module_hash, vec_t defer_stack, c_ir_unit_t unit);

/* ===== Type inference for lower ===== */

/**
 * @brief Infer the semantic type of an expression node using lower's own scope chain.
 *
 * This replaces all context_check_expression calls in lower. It uses:
 * - lower's scope chain for variable/function/type lookups
 * - resolver_resolve_type for type expression nodes
 * - recursive inference for compound expressions
 */
semantic_type_t lower_infer_type(allocator_t allocator, context_t ctx,
                                   node_t node, const char *module_hash,
                                   c_ir_unit_t unit) {
  if (!node) return NULL;

  switch (node->kind) {

  /* Identifier → look up in lower's scope chain */
  case CUBEC_NODE_LITERAL_IDENTIFIER: {
    const char *name = string_get(((cubec_literal_identifier_t)node)->value);
    struct symbol *sym = scope_lookup(ctx->current_scope, name);
    if (!sym) return NULL;
    switch (sym->kind) {
    case SYMBOL_VARIABLE: return sym->variable.type;
    case SYMBOL_FUNCTION: return sym->function.type;
    case SYMBOL_TYPE:     return sym->type.type;
    case SYMBOL_ENUM_ITEM: {
      /* Enum item's type is the owning enum type */
      return sym->enum_item.owning_type;
    }
    default: return NULL;
    }
  }

  /* Member access (obj.field) → get host type, find field */
  case CUBEC_NODE_EXPRESSION_MEMBER: {
    cubec_expression_member_t n = (cubec_expression_member_t)node;
    semantic_type_t host_type = lower_infer_type(allocator, ctx, n->host, module_hash, unit);
    if (!host_type) return NULL;

    /* Dereference pointer/qualifier wrapping */
    enum type_kind k = semantic_type_get_kind(host_type);
    while (k == TYPE_QUALIFIER) {
      type_impl_t impl = semantic_type_get_impl(host_type);
      host_type = impl->qualifier.base;
      k = semantic_type_get_kind(host_type);
    }
    if (k == TYPE_POINTER) {
      type_impl_t impl = semantic_type_get_impl(host_type);
      host_type = impl->pointer.pointee;
      k = semantic_type_get_kind(host_type);
    }

    /* Find field in struct type */
    const char *field_name = string_get(n->field->value);
    vec_t fields = NULL;
    if (k == TYPE_STRUCT || k == TYPE_UNION || k == TYPE_CUNION) {
      type_impl_t impl = semantic_type_get_impl(host_type);
      fields = impl->struct_type.fields;
    } else if (k == TYPE_GENERIC_INSTANCE) {
      type_impl_t impl = semantic_type_get_impl(host_type);
      fields = impl->generic_instance.fields;
    } else if (k == TYPE_TUPLE) {
      type_impl_t impl = semantic_type_get_impl(host_type);
      fields = impl->tuple.fields;
    }

    if (fields) {
      size_t count = vec_get_size(fields);
      for (size_t i = 0; i < count; i++) {
        struct symbol *f = vec_get(fields, i);
        if (strcmp(f->name, field_name) == 0 && f->field.type) {
          return f->field.type;
        }
      }
    }
    return NULL;
  }

  /* Dereference (.*) → get pointee type */
  case CUBEC_NODE_EXPRESSION_DEREF: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    semantic_type_t host_type = lower_infer_type(allocator, ctx, n->right, module_hash, unit);
    if (!host_type) return NULL;
    enum type_kind dk = semantic_type_get_kind(host_type);
    if (dk == TYPE_POINTER) {
      type_impl_t impl = semantic_type_get_impl(host_type);
      return impl->pointer.pointee;
    }
    if (dk == TYPE_QUALIFIER) {
      type_impl_t impl = semantic_type_get_impl(host_type);
      semantic_type_t base = impl->qualifier.base;
      if (base && semantic_type_get_kind(base) == TYPE_POINTER)
        return base->impl->pointer.pointee;
    }
    return NULL;
  }

  /* Address-of (.&) → pointer to host type */
  case CUBEC_NODE_EXPRESSION_ADDR: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    semantic_type_t host_type = lower_infer_type(allocator, ctx, n->right, module_hash, unit);
    if (!host_type) return NULL;
    return semantic_type_create_pointer(allocator, host_type);
  }

  /* Try (.?) and Assert (.!) → unwrap union/Result to get inner value type */
  case CUBEC_NODE_EXPRESSION_TRY:
  case CUBEC_NODE_EXPRESSION_ASSERT: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    return lower_infer_type(allocator, ctx, n->right, module_hash, unit);
  }

  /* Subscript (tuple[index]) → field type at the given index */
  case CUBEC_NODE_EXPRESSION_SUBSCRIPT: {
    cubec_expression_subscript_t n = (cubec_expression_subscript_t)node;
    semantic_type_t host_type = lower_infer_type(allocator, ctx, n->host, module_hash, unit);
    if (!host_type) return NULL;

    /* Strip qualifier/pointer */
    enum type_kind k = semantic_type_get_kind(host_type);
    while (k == TYPE_QUALIFIER) {
      type_impl_t impl = semantic_type_get_impl(host_type);
      host_type = impl->qualifier.base;
      k = semantic_type_get_kind(host_type);
    }
    if (k == TYPE_POINTER) {
      type_impl_t impl = semantic_type_get_impl(host_type);
      host_type = impl->pointer.pointee;
      k = semantic_type_get_kind(host_type);
    }

    if (k != TYPE_TUPLE) return NULL;

    /* Get the index as a comptime integer */
    uint64_t idx = 0;
    if (n->index->kind == CUBEC_NODE_LITERAL_NUMERIC) {
      cubec_literal_numeric_t num = (cubec_literal_numeric_t)n->index;
      idx = strtoull(string_get(num->value), NULL, 10);
    } else {
      return NULL;
    }

    type_impl_t impl = semantic_type_get_impl(host_type);
    vec_t fields = impl->tuple.fields;
    if (idx < vec_get_size(fields)) {
      struct symbol *f = (struct symbol *)vec_get(fields, (size_t)idx);
      if (f && f->field.type) return f->field.type;
    }
    return NULL;
  }

  /* Initialize list → type from n->type expression */
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST: {
    cubec_expression_initialize_list_t n = (cubec_expression_initialize_list_t)node;
    if (n->type) return resolver_resolve_type(ctx, n->type);
    return NULL;
  }

  /* Sizeof → type of the inner expression */
  case CUBEC_NODE_EXPRESSION_SIZEOF: {
    cubec_expression_sizeof_t n = (cubec_expression_sizeof_t)node;
    /* sizeof can take a type expression or a value expression */
    if (n->expression) {
      enum _cubec_node_kind_t ek = n->expression->kind;
      /* Type expression nodes: use resolver */
      if (ek == CUBEC_NODE_EXPRESSION_TYPE_STRUCT || ek == CUBEC_NODE_EXPRESSION_TYPE_UNION ||
          ek == CUBEC_NODE_EXPRESSION_TYPE_ENUM || ek == CUBEC_NODE_EXPRESSION_TYPE_TUPLE ||
          ek == CUBEC_NODE_EXPRESSION_TYPE_FUNCTION || ek == CUBEC_NODE_EXPRESSION_TYPE_INTERFACE ||
          ek == CUBEC_NODE_DECLARATION_POINTER || ek == CUBEC_NODE_DECLARATION_SLICE ||
          ek == CUBEC_NODE_DECLARATION_ARRAY || ek == CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER ||
          ek == CUBEC_NODE_EXPRESSION_TYPEOF || ek == CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS ||
          ek == CUBEC_NODE_LITERAL_IDENTIFIER) {
        return resolver_resolve_type(ctx, n->expression);
      }
      /* Value expression: infer its type */
      return lower_infer_type(allocator, ctx, n->expression, module_hash, unit);
    }
    return NULL;
  }

  /* Alignof → same as sizeof for type inference */
  case CUBEC_NODE_EXPRESSION_ALIGNOF: {
    cubec_expression_alignof_t n = (cubec_expression_alignof_t)node;
    if (n->expression) {
      enum _cubec_node_kind_t ek = n->expression->kind;
      if (ek == CUBEC_NODE_EXPRESSION_TYPE_STRUCT || ek == CUBEC_NODE_EXPRESSION_TYPE_UNION ||
          ek == CUBEC_NODE_EXPRESSION_TYPE_ENUM || ek == CUBEC_NODE_EXPRESSION_TYPE_TUPLE ||
          ek == CUBEC_NODE_EXPRESSION_TYPE_FUNCTION || ek == CUBEC_NODE_EXPRESSION_TYPE_INTERFACE ||
          ek == CUBEC_NODE_DECLARATION_POINTER || ek == CUBEC_NODE_DECLARATION_SLICE ||
          ek == CUBEC_NODE_DECLARATION_ARRAY || ek == CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER ||
          ek == CUBEC_NODE_EXPRESSION_TYPEOF || ek == CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS ||
          ek == CUBEC_NODE_LITERAL_IDENTIFIER) {
        return resolver_resolve_type(ctx, n->expression);
      }
      return lower_infer_type(allocator, ctx, n->expression, module_hash, unit);
    }
    return NULL;
  }

  /* Call → callee's return type */
  case CUBEC_NODE_EXPRESSION_CALL: {
    cubec_expression_call_t n = (cubec_expression_call_t)node;
    /* Check if callee is a builtin that needs special handling */
    if (n->callee && n->callee->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
      const char *fn_name = string_get(((cubec_literal_identifier_t)n->callee)->value);
      struct symbol *sym = scope_lookup(ctx->global_scope, fn_name);

      if (sym && sym->is_builtin && sym->kind == SYMBOL_FUNCTION) {
        /* makeSlice → construct Slice<elem> type */
        if (strcmp(fn_name, "makeSlice") == 0 && n->arguments &&
            vec_get_size(n->arguments) >= 1) {
          semantic_type_t elem_type = resolver_resolve_type(ctx, vec_get(n->arguments, 0));
          if (elem_type) {
            return semantic_type_create_slice(allocator, elem_type);
          }
        }
        /* cast → first argument type */
        if (strcmp(fn_name, "cast") == 0 && n->arguments &&
            vec_get_size(n->arguments) >= 1) {
          return resolver_resolve_type(ctx, vec_get(n->arguments, 0));
        }
      }

      /* Regular function call → get return type from function symbol */
      if (sym && sym->kind == SYMBOL_FUNCTION && sym->function.type) {
        type_impl_t impl = semantic_type_get_impl(sym->function.type);
        return impl->function.return_type;
      }
    }

    /* Generic instantiation as callee */
    if (n->callee && n->callee->kind == CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION) {
      semantic_type_t callee_type = resolver_resolve_type(ctx, n->callee);
      if (callee_type && semantic_type_get_kind(callee_type) == TYPE_FUNCTION) {
        type_impl_t impl = semantic_type_get_impl(callee_type);
        return impl->function.return_type;
      }
    }

    /* Fallback: try to infer callee type */
    semantic_type_t callee_type = lower_infer_type(allocator, ctx, n->callee, module_hash, unit);
    if (callee_type && semantic_type_get_kind(callee_type) == TYPE_FUNCTION) {
      type_impl_t impl = semantic_type_get_impl(callee_type);
      return impl->function.return_type;
    }
    return NULL;
  }

  /* Slice expression → same type as host (should be a slice) */
  case CUBEC_NODE_EXPRESSION_SLICE: {
    cubec_expression_slice_t n = (cubec_expression_slice_t)node;
    semantic_type_t host_type = lower_infer_type(allocator, ctx, n->host, module_hash, unit);
    if (host_type) {
      enum type_kind k = semantic_type_get_kind(host_type);
      if (k == TYPE_SLICE) return host_type;
      /* If host is a qualifier wrapping a slice */
      if (k == TYPE_QUALIFIER) {
        type_impl_t impl = semantic_type_get_impl(host_type);
        if (impl->qualifier.base && semantic_type_get_kind(impl->qualifier.base) == TYPE_SLICE)
          return impl->qualifier.base;
      }
    }
    return NULL;
  }

  /* Generic instantiation → resolve via resolver, or subscript inference */
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION: {
    semantic_type_t rt = resolver_resolve_type(ctx, node);
    if (rt) return rt;
    /* If resolver can't resolve (callee is a variable), treat as subscript */
    {
      cubec_expression_generic_instantiation_t gi =
          (cubec_expression_generic_instantiation_t)node;
      if (gi->callee && gi->callee->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
        const char *name =
            string_get(((cubec_literal_identifier_t)gi->callee)->value);
        struct symbol *sym = name ? scope_lookup(ctx->current_scope, name) : NULL;
        if (sym && sym->kind == SYMBOL_VARIABLE) {
          semantic_type_t host_type = sym->variable.type;
          if (host_type) {
            semantic_type_t host_unq = semantic_type_strip_qualifier(host_type);
            if (host_unq && (host_unq->impl->kind == TYPE_TUPLE ||
                (host_unq->impl->kind == TYPE_GENERIC_INSTANCE &&
                 host_unq->impl->generic_instance.fields))) {
              /* Infer the field type from the tuple */
              if (gi->arguments && vec_get_size(gi->arguments) >= 1) {
                node_t idx_node = (node_t)vec_get(gi->arguments, 0);
                if (idx_node->kind == CUBEC_NODE_LITERAL_NUMERIC) {
                  cubec_literal_numeric_t num = (cubec_literal_numeric_t)idx_node;
                  uint64_t idx = strtoull(string_get(num->value), NULL, 10);
                  vec_t fields = host_unq->impl->kind == TYPE_TUPLE
                      ? host_unq->impl->tuple.fields
                      : host_unq->impl->generic_instance.fields;
                  size_t fcount = fields ? vec_get_size(fields) : 0;
                  if (idx < fcount) {
                    struct symbol *f = (struct symbol *)vec_get(fields, (size_t)idx);
                    return f ? f->field.type : NULL;
                  }
                }
              }
            }
          }
        }
      }
    }
    return NULL;
  }

  /* Anonymous function → resolve via resolver */
  case CUBEC_NODE_EXPRESSION_FUNCTION: {
    return resolver_resolve_type(ctx, node);
  }

  /* Type expression nodes → use resolver */
  case CUBEC_NODE_EXPRESSION_TYPE_STRUCT:
  case CUBEC_NODE_EXPRESSION_TYPE_UNION:
  case CUBEC_NODE_EXPRESSION_TYPE_ENUM:
  case CUBEC_NODE_EXPRESSION_TYPE_TUPLE:
  case CUBEC_NODE_EXPRESSION_TYPE_FUNCTION:
  case CUBEC_NODE_EXPRESSION_TYPE_INTERFACE:
  case CUBEC_NODE_EXPRESSION_TYPEOF:
  case CUBEC_NODE_DECLARATION_POINTER:
  case CUBEC_NODE_DECLARATION_SLICE:
  case CUBEC_NODE_DECLARATION_ARRAY:
  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER:
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS:
    return resolver_resolve_type(ctx, node);

  /* Binary / ternary / group → infer from structure */
  case CUBEC_NODE_EXPRESSION_BINARY: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    if (!n->left) {
      /* Prefix unary: result type depends on operator */
      const char *op = string_get(n->opt);
      if (strcmp(op, ".*") == 0) {
        /* Dereference */
        semantic_type_t host = lower_infer_type(allocator, ctx, n->right, module_hash, unit);
        if (host && semantic_type_get_kind(host) == TYPE_POINTER) {
          type_impl_t impl = semantic_type_get_impl(host);
          return impl->pointer.pointee;
        }
        return NULL;
      }
      if (strcmp(op, ".&") == 0) {
        semantic_type_t host = lower_infer_type(allocator, ctx, n->right, module_hash, unit);
        return host ? semantic_type_create_pointer(allocator, host) : NULL;
      }
      /* !, -, ~ → same type or bool */
      return lower_infer_type(allocator, ctx, n->right, module_hash, unit);
    }
    /* Regular binary → right operand type (simplified; C rules are more nuanced) */
    const char *op = string_get(n->opt);
    if (strcmp(op, "is") == 0) {
      /* is operator always returns bool */
      return ctx->builtin_bool;
    }
    if (strcmp(op, "extends") == 0) {
      return ctx->builtin_bool;
    }
    return lower_infer_type(allocator, ctx, n->right, module_hash, unit);
  }

  case CUBEC_NODE_EXPRESSION_TERNARY: {
    cubec_expression_ternary_t n = (cubec_expression_ternary_t)node;
    return lower_infer_type(allocator, ctx, n->consequent, module_hash, unit);
  }

  case CUBEC_NODE_EXPRESSION_GROUP: {
    cubec_expression_group_t n = (cubec_expression_group_t)node;
    return lower_infer_type(allocator, ctx, n->inner, module_hash, unit);
  }

  /* Assignment → type of left operand */
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    return lower_infer_type(allocator, ctx, n->left, module_hash, unit);
  }

  /* Comma → type of right operand */
  case CUBEC_NODE_EXPRESSION_COMMA: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    return lower_infer_type(allocator, ctx, n->right, module_hash, unit);
  }

  default:
    return NULL;
  }
}

/* Counter for anonymous function names */
static int _anon_func_counter = 0;

/**
 * @brief Lower a cubec expression AST node to a C IR expression node.
 * @param unit  Compilation unit for adding lifted definitions (e.g., anonymous functions)
 */
c_ir_node_t lower_expr(allocator_t allocator, context_t ctx, node_t node,
                        const char *module_hash, c_ir_unit_t unit) {
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
    const char *name = string_get(n->value);

    /* Resolve identifier from lower's own scope chain */
    struct symbol *sym = scope_lookup(ctx->current_scope, name);

    if (sym && (sym->kind == SYMBOL_FUNCTION || sym->kind == SYMBOL_VARIABLE ||
                sym->kind == SYMBOL_TYPE || sym->kind == SYMBOL_ENUM_ITEM ||
                sym->kind == SYMBOL_FIELD)) {
      /* Extern functions and builtin functions keep their raw name */
      if (sym->kind == SYMBOL_FUNCTION) {
        cubec_statement_function_t fn_node =
            (cubec_statement_function_t)sym->function.ast_node;
        if (fn_node && (fn_node->is_extern || fn_node->is_builtin)) {
          return (c_ir_node_t)c_ir_expr_ident_create(allocator, name, loc);
        }
        if (sym->is_builtin) {
          return (c_ir_node_t)c_ir_expr_ident_create(allocator, name, loc);
        }
      }
      /* Builtin variables also keep raw name */
      if (sym->is_builtin) {
        return (c_ir_node_t)c_ir_expr_ident_create(allocator, name, loc);
      }
      /* Local variables (not in global scope) keep their raw name */
      if (sym->kind == SYMBOL_VARIABLE) {
        struct symbol *global_sym = scope_lookup_local(ctx->global_scope, name);
        if (global_sym != sym) {
          return (c_ir_node_t)c_ir_expr_ident_create(allocator, name, loc);
        }
      }
      /* Global symbols get mangled names */
      string_t mangled = mangle_name(allocator, module_hash, name);
      c_ir_node_t result = (c_ir_node_t)c_ir_expr_ident_create(
          allocator, string_get(mangled), loc);
      allocator_free(allocator, &mangled);
      return result;
    }
    /* Unresolved — use raw name */
    return (c_ir_node_t)c_ir_expr_ident_create(allocator, name, loc);
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
      c_ir_node_t operand = lower_expr(allocator, ctx, n->right, module_hash, unit);
      const char *op = string_get(n->opt);
      /* Map cubec deref .* to C * and address .& to C & */
      if (strcmp(op, ".*") == 0) op = "*";
      else if (strcmp(op, ".&") == 0) op = "&";
      return (c_ir_node_t)c_ir_expr_unary_create(allocator, op, operand, true, loc);
    }
    const char *op = string_get(n->opt);

    /* is: union variant check → obj._tag == Type_tag */
    if (strcmp(op, "is") == 0) {
      c_ir_node_t obj = lower_expr(allocator, ctx, n->left, module_hash, unit);
      c_ir_node_t tag_access = (c_ir_node_t)c_ir_expr_member_create(
          allocator, obj, "_tag", false, loc);

      /* Resolve the target type from the right operand */
      semantic_type_t check_type = resolver_resolve_type(ctx, n->right);
      const char *tag_name = check_type
          ? semantic_type_get_name(check_type) : "0";
      string_t mangled_tag = mangle_name(allocator, module_hash, tag_name);
      char tag_const_buf[128];
      snprintf(tag_const_buf, sizeof(tag_const_buf), "%s_tag",
               string_get(mangled_tag));
      allocator_free(allocator, &mangled_tag);

      c_ir_node_t tag_val = (c_ir_node_t)c_ir_expr_ident_create(
          allocator, tag_const_buf, loc);
      return (c_ir_node_t)c_ir_expr_binary_create(
          allocator, "==", tag_access, tag_val, loc);
    }

    c_ir_node_t left = lower_expr(allocator, ctx, n->left, module_hash, unit);
    c_ir_node_t right = lower_expr(allocator, ctx, n->right, module_hash, unit);
    return (c_ir_node_t)c_ir_expr_binary_create(allocator, op,
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
    /* Wildcard assignment: _ = expr → just the expression (discard value) */
    if (n->left && n->left->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
      cubec_literal_identifier_t id = (cubec_literal_identifier_t)n->left;
      if (strcmp(string_get(id->value), "_") == 0 &&
          strcmp(string_get(n->opt), "=") == 0) {
        return lower_expr(allocator, ctx, n->right, module_hash, unit);
      }
    }
    c_ir_node_t left = lower_expr(allocator, ctx, n->left, module_hash, unit);
    c_ir_node_t right = lower_expr(allocator, ctx, n->right, module_hash, unit);
    return (c_ir_node_t)c_ir_expr_binary_create(allocator, string_get(n->opt),
                                                   left, right, loc);
  }

  /* ===== Call ===== */
  case CUBEC_NODE_EXPRESSION_CALL: {
    cubec_expression_call_t n = (cubec_expression_call_t)node;
    size_t count = n->arguments ? vec_get_size(n->arguments) : 0;

    /* Check if callee is a builtin function */
    if (n->callee && n->callee->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
      cubec_literal_identifier_t id = (cubec_literal_identifier_t)n->callee;
      const char *fn_name = string_get(id->value);
      struct symbol *sym = scope_lookup(ctx->global_scope, fn_name);

      if (sym && sym->is_builtin && sym->kind == SYMBOL_FUNCTION) {
        /* Builtin function lowering */

        /* assert — no runtime code */
        if (strcmp(fn_name, "assert") == 0) {
          return NULL;
        }

        /* panic → call panic() (declared as static inline in .h) */
        if (strcmp(fn_name, "panic") == 0) {
          c_ir_node_t callee = (c_ir_node_t)c_ir_expr_ident_create(
              allocator, "panic", loc);
          vec_t args = allocator_create(allocator, &g_vec_type,
                                         &(vec_init_t){.auto_dispose = false});
          for (size_t i = 0; i < count; i++) {
            c_ir_node_t c_arg = lower_expr(allocator, ctx,
                                             vec_get(n->arguments, i),
                                             module_hash, unit);
            if (c_arg) vec_push(args, c_arg);
          }
          return (c_ir_node_t)c_ir_expr_call_create(allocator, callee, args, loc);
        }

        /* makeSlice → compound literal */
        if (strcmp(fn_name, "makeSlice") == 0 && count >= 3) {
          semantic_type_t slice_type = lower_infer_type(allocator, ctx, (node_t)n, module_hash, unit);
          c_type_t c_type = slice_type
              ? lower_type(allocator, slice_type, module_hash, unit)
              : c_type_primitive(allocator, "void");

          vec_t fields = allocator_create(allocator, &g_vec_type,
                                            &(vec_init_t){.auto_dispose = false});

          /* .ptr = pointer */
          c_ir_node_t ptr_arg = lower_expr(allocator, ctx,
                                             vec_get(n->arguments, 0),
                                             module_hash, unit);
          c_ir_node_t init_ptr = (c_ir_node_t)c_ir_expr_initializer_create(
              allocator, "ptr", ptr_arg, loc);
          vec_push(fields, init_ptr);

          /* .start = start */
          c_ir_node_t start_arg = lower_expr(allocator, ctx,
                                                vec_get(n->arguments, 1),
                                                module_hash, unit);
          c_ir_node_t init_start = (c_ir_node_t)c_ir_expr_initializer_create(
              allocator, "start", start_arg, loc);
          vec_push(fields, init_start);

          /* .length = len */
          c_ir_node_t len_arg = lower_expr(allocator, ctx,
                                             vec_get(n->arguments, 2),
                                             module_hash, unit);
          c_ir_node_t init_len = (c_ir_node_t)c_ir_expr_initializer_create(
              allocator, "length", len_arg, loc);
          vec_push(fields, init_len);

          return (c_ir_node_t)c_ir_expr_compound_create(allocator, c_type,
                                                          fields, loc);
        }

        /* cast → C cast expression */
        if (strcmp(fn_name, "cast") == 0 && count >= 2) {
          semantic_type_t result_type = lower_infer_type(allocator, ctx, (node_t)n, module_hash, unit);
          c_type_t c_type = result_type
              ? lower_type(allocator, result_type, module_hash, unit)
              : c_type_primitive(allocator, "void");
          c_ir_node_t expr = lower_expr(allocator, ctx,
                                          vec_get(n->arguments, 1),
                                          module_hash, unit);
          return (c_ir_node_t)c_ir_expr_cast_create(allocator, c_type, expr, loc);
        }

        /* length/toString/typename — comptime, should be inlined by checker */
        if (strcmp(fn_name, "length") == 0 ||
            strcmp(fn_name, "toString") == 0 ||
            strcmp(fn_name, "typename") == 0) {
          /* These should be resolved at comptime; if not, fall through */
        }
      }
    }

    /* Default call lowering */
    c_ir_node_t callee = lower_expr(allocator, ctx, n->callee, module_hash, unit);
    vec_t args = allocator_create(allocator, &g_vec_type,
                                   &(vec_init_t){.auto_dispose = false});
    for (size_t i = 0; i < count; i++) {
      node_t arg = vec_get(n->arguments, i);
      c_ir_node_t c_arg = lower_expr(allocator, ctx, arg, module_hash, unit);
      if (c_arg) vec_push(args, c_arg);
    }
    return (c_ir_node_t)c_ir_expr_call_create(allocator, callee, args, loc);
  }

  /* ===== Member access ===== */
  case CUBEC_NODE_EXPRESSION_MEMBER: {
    cubec_expression_member_t n = (cubec_expression_member_t)node;
    const char *field = string_get(n->field->value);

    /* If the host is a dereference (.*), combine into arrow (->) access:
     * pp.*.x → pp->x  instead of (*pp).x */
    if (n->host && n->host->kind == CUBEC_NODE_EXPRESSION_DEREF) {
      cubec_expression_binary_t deref_node = (cubec_expression_binary_t)n->host;
      /* postfix unary deref: left=NULL, right=host */
      c_ir_node_t obj = lower_expr(allocator, ctx, deref_node->right, module_hash, unit);
      return (c_ir_node_t)c_ir_expr_member_create(allocator, obj, field, true, loc);
    }

    c_ir_node_t obj = lower_expr(allocator, ctx, n->host, module_hash, unit);

    /* Determine is_arrow from inferred type of host expression */
    bool is_arrow = false;
    semantic_type_t host_type = lower_infer_type(allocator, ctx, n->host, module_hash, unit);

    if (host_type) {
      enum type_kind k = semantic_type_get_kind(host_type);
      if (k == TYPE_POINTER) {
        is_arrow = true;
      } else if (k == TYPE_QUALIFIER) {
        type_impl_t impl = semantic_type_get_impl(host_type);
        semantic_type_t base = impl->qualifier.base;
        if (base && semantic_type_get_kind(base) == TYPE_POINTER)
          is_arrow = true;
      }
    }

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
    c_ir_node_t cond = lower_expr(allocator, ctx, n->condition, module_hash, unit);
    c_ir_node_t cons = lower_expr(allocator, ctx, n->consequent, module_hash, unit);
    c_ir_node_t alt = lower_expr(allocator, ctx, n->alternate, module_hash, unit);
    return (c_ir_node_t)c_ir_expr_ternary_create(allocator, cond, cons, alt, loc);
  }

  /* ===== Initialize list (struct literal) ===== */
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST: {
    cubec_expression_initialize_list_t n = (cubec_expression_initialize_list_t)node;
    /* Resolve type via lower's own type inference */
    semantic_type_t init_type = lower_infer_type(allocator, ctx, (node_t)n, module_hash, unit);
    c_type_t type = init_type
        ? lower_type(allocator, init_type, module_hash, unit)
        : c_type_primitive(allocator, "void");
    vec_t fields = allocator_create(allocator, &g_vec_type,
                                      &(vec_init_t){.auto_dispose = false});
    size_t count = n->items ? vec_get_size(n->items) : 0;
    for (size_t i = 0; i < count; i++) {
      node_t item = vec_get(n->items, i);
      if (item->kind == CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD) {
        cubec_expression_initialize_field_t f = (cubec_expression_initialize_field_t)item;
        const char *name = f->field ? string_get(f->field->value) : NULL;
        c_ir_node_t value = lower_expr(allocator, ctx, f->value, module_hash, unit);
        c_ir_node_t init = (c_ir_node_t)c_ir_expr_initializer_create(allocator, name, value, item->location);
        vec_push(fields, init);
      } else {
        c_ir_node_t value = lower_expr(allocator, ctx, item, module_hash, unit);
        c_ir_node_t init = (c_ir_node_t)c_ir_expr_initializer_create_indexed(allocator, i, value, item->location);
        vec_push(fields, init);
      }
    }
    return (c_ir_node_t)c_ir_expr_compound_create(allocator, type, fields, loc);
  }

  /* ===== Group (parenthesized) ===== */
  case CUBEC_NODE_EXPRESSION_GROUP: {
    cubec_expression_group_t n = (cubec_expression_group_t)node;
    return lower_expr(allocator, ctx, n->inner, module_hash, unit);
  }

  /* ===== Sizeof / Alignof ===== */
  case CUBEC_NODE_EXPRESSION_SIZEOF: {
    cubec_expression_sizeof_t n = (cubec_expression_sizeof_t)node;
    semantic_type_t sem_type = lower_infer_type(allocator, ctx, (node_t)n, module_hash, unit);
    c_type_t type = sem_type
        ? lower_type(allocator, sem_type, module_hash, unit)
        : c_type_primitive(allocator, "void");
    return (c_ir_node_t)c_ir_expr_sizeof_create(allocator, type, loc);
  }

  case CUBEC_NODE_EXPRESSION_ALIGNOF: {
    cubec_expression_alignof_t n = (cubec_expression_alignof_t)node;
    semantic_type_t sem_type = lower_infer_type(allocator, ctx, (node_t)n, module_hash, unit);
    c_type_t type = sem_type
        ? lower_type(allocator, sem_type, module_hash, unit)
        : c_type_primitive(allocator, "void");
    return (c_ir_node_t)c_ir_expr_alignof_create(allocator, type, loc);
  }

  /* ===== Try (.? error propagation) ===== */
  case CUBEC_NODE_EXPRESSION_TRY: {
    /* .? is stored as postfix_unary (= expression_binary_t with left=NULL) */
    /* Only valid on union and Result types — pointer .? is deprecated */
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    c_ir_node_t host = lower_expr(allocator, ctx, n->right, module_hash, unit);
    if (!host) return NULL;

    semantic_type_t host_type = lower_infer_type(allocator, ctx, n->right, module_hash, unit);

    vec_t stmts = allocator_create(allocator, &g_vec_type,
                                     &(vec_init_t){.auto_dispose = false});

    /* Cache host in a temporary */
    c_type_t tmp_type = host_type
        ? lower_type(allocator, host_type, module_hash, unit)
        : c_type_primitive(allocator, "void");
    c_ir_node_t tmp_decl = (c_ir_node_t)c_ir_stmt_local_decl_create(
        allocator, tmp_type, "_try_val", host, loc);
    vec_push(stmts, tmp_decl);

    /* _try_val.isError() → if true, return _try_val.error() */
    c_ir_node_t tmp_ref = (c_ir_node_t)c_ir_expr_ident_create(
        allocator, "_try_val", loc);
    c_ir_node_t is_err_call = (c_ir_node_t)c_ir_expr_call_create(
        allocator,
        (c_ir_node_t)c_ir_expr_member_create(allocator, tmp_ref, "isError", false, loc),
        allocator_create(allocator, &g_vec_type, &(vec_init_t){.auto_dispose = false}),
        loc);

    c_ir_node_t tmp_ref2 = (c_ir_node_t)c_ir_expr_ident_create(
        allocator, "_try_val", loc);
    c_ir_node_t err_call = (c_ir_node_t)c_ir_expr_call_create(
        allocator,
        (c_ir_node_t)c_ir_expr_member_create(allocator, tmp_ref2, "error", false, loc),
        allocator_create(allocator, &g_vec_type, &(vec_init_t){.auto_dispose = false}),
        loc);
    c_ir_node_t err_ret = (c_ir_node_t)c_ir_stmt_return_create(
        allocator, err_call, loc);
    c_ir_node_t err_if = (c_ir_node_t)c_ir_stmt_if_create(
        allocator, is_err_call, err_ret, NULL, loc);
    vec_push(stmts, err_if);

    /* Result: _try_val.value() */
    c_ir_node_t tmp_ref3 = (c_ir_node_t)c_ir_expr_ident_create(
        allocator, "_try_val", loc);
    c_ir_node_t val_call = (c_ir_node_t)c_ir_expr_call_create(
        allocator,
        (c_ir_node_t)c_ir_expr_member_create(allocator, tmp_ref3, "value", false, loc),
        allocator_create(allocator, &g_vec_type, &(vec_init_t){.auto_dispose = false}),
        loc);
    c_ir_node_t val_stmt = (c_ir_node_t)c_ir_stmt_expr_create(
        allocator, val_call, loc);
    vec_push(stmts, val_stmt);

    return (c_ir_node_t)c_ir_stmt_stmt_expr_create(allocator, stmts, loc);
  }

  /* ===== Assert (.!) ===== */
  case CUBEC_NODE_EXPRESSION_ASSERT: {
    /* .! is stored as postfix_unary (= expression_binary_t with left=NULL) */
    /* Only valid on union and Result types — pointer .! is deprecated */
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    c_ir_node_t host = lower_expr(allocator, ctx, n->right, module_hash, unit);
    if (!host) return NULL;

    semantic_type_t host_type = lower_infer_type(allocator, ctx, n->right, module_hash, unit);

    vec_t stmts = allocator_create(allocator, &g_vec_type,
                                     &(vec_init_t){.auto_dispose = false});

    c_type_t tmp_type = host_type
        ? lower_type(allocator, host_type, module_hash, unit)
        : c_type_primitive(allocator, "void");
    c_ir_node_t tmp_decl = (c_ir_node_t)c_ir_stmt_local_decl_create(
        allocator, tmp_type, "_assert_val", host, loc);
    vec_push(stmts, tmp_decl);

    /* _assert_val.isError() → abort() */
    c_ir_node_t tmp_ref = (c_ir_node_t)c_ir_expr_ident_create(
        allocator, "_assert_val", loc);
    c_ir_node_t is_err_call = (c_ir_node_t)c_ir_expr_call_create(
        allocator,
        (c_ir_node_t)c_ir_expr_member_create(allocator, tmp_ref, "isError", false, loc),
        allocator_create(allocator, &g_vec_type, &(vec_init_t){.auto_dispose = false}),
        loc);
    vec_t abort_args = allocator_create(allocator, &g_vec_type,
                                          &(vec_init_t){.auto_dispose = false});
    c_ir_node_t abort_call = (c_ir_node_t)c_ir_expr_call_create(
        allocator,
        (c_ir_node_t)c_ir_expr_ident_create(allocator, "panic", loc),
        abort_args, loc);
    c_ir_node_t abort_stmt = (c_ir_node_t)c_ir_stmt_expr_create(
        allocator, abort_call, loc);
    c_ir_node_t err_if = (c_ir_node_t)c_ir_stmt_if_create(
        allocator, is_err_call, abort_stmt, NULL, loc);
    vec_push(stmts, err_if);

    /* Result: _assert_val.value() */
    c_ir_node_t tmp_ref2 = (c_ir_node_t)c_ir_expr_ident_create(
        allocator, "_assert_val", loc);
    c_ir_node_t val_call = (c_ir_node_t)c_ir_expr_call_create(
        allocator,
        (c_ir_node_t)c_ir_expr_member_create(allocator, tmp_ref2, "value", false, loc),
        allocator_create(allocator, &g_vec_type, &(vec_init_t){.auto_dispose = false}),
        loc);
    c_ir_node_t val_stmt = (c_ir_node_t)c_ir_stmt_expr_create(
        allocator, val_call, loc);
    vec_push(stmts, val_stmt);

    return (c_ir_node_t)c_ir_stmt_stmt_expr_create(allocator, stmts, loc);
  }

  /* ===== Comma expression ===== */
  case CUBEC_NODE_EXPRESSION_COMMA: {
    /* Comma is a binary op */
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    c_ir_node_t left = lower_expr(allocator, ctx, n->left, module_hash, unit);
    c_ir_node_t right = lower_expr(allocator, ctx, n->right, module_hash, unit);
    return (c_ir_node_t)c_ir_expr_binary_create(allocator, ",", left, right, loc);
  }

  /* ===== Slice expression ===== */
  case CUBEC_NODE_EXPRESSION_SLICE: {
    cubec_expression_slice_t n = (cubec_expression_slice_t)node;

    c_ir_node_t host = lower_expr(allocator, ctx, n->host, module_hash, unit);
    if (!host) return NULL;

    c_ir_node_t start = n->start
        ? lower_expr(allocator, ctx, n->start, module_hash, unit)
        : (c_ir_node_t)c_ir_expr_numeric_create(allocator, "0", loc);

    /* Resolve slice type via lower's type inference */
    semantic_type_t slice_type_sema = lower_infer_type(allocator, ctx, (node_t)n, module_hash, unit);
    c_type_t slice_type = slice_type_sema
        ? lower_type(allocator, slice_type_sema, module_hash, unit)
        : c_type_primitive(allocator, "void");

    vec_t fields = allocator_create(allocator, &g_vec_type,
                                      &(vec_init_t){.auto_dispose = false});

    /* .ptr = host.ptr */
    c_ir_node_t host_ptr = (c_ir_node_t)c_ir_expr_member_create(
        allocator, host, "ptr", false, loc);
    c_ir_node_t init_ptr = (c_ir_node_t)c_ir_expr_initializer_create(
        allocator, "ptr", host_ptr, loc);
    vec_push(fields, init_ptr);

    /* .start = start */
    c_ir_node_t init_start = (c_ir_node_t)c_ir_expr_initializer_create(
        allocator, "start", start, loc);
    vec_push(fields, init_start);

    /* .length = length or (host.length - start) */
    if (n->length) {
      c_ir_node_t length = lower_expr(allocator, ctx, n->length, module_hash, unit);
      c_ir_node_t init_len = (c_ir_node_t)c_ir_expr_initializer_create(
          allocator, "length", length, loc);
      vec_push(fields, init_len);
    } else {
      c_ir_node_t host_len = (c_ir_node_t)c_ir_expr_member_create(
          allocator, host, "length", false, loc);
      c_ir_node_t sub = (c_ir_node_t)c_ir_expr_binary_create(
          allocator, "-", host_len, start, loc);
      c_ir_node_t init_len = (c_ir_node_t)c_ir_expr_initializer_create(
          allocator, "length", sub, loc);
      vec_push(fields, init_len);
    }

    return (c_ir_node_t)c_ir_expr_compound_create(allocator, slice_type, fields, loc);
  }

  /* ===== Subscript (tuple[index]) → struct member access ===== */
  case CUBEC_NODE_EXPRESSION_SUBSCRIPT: {
    cubec_expression_subscript_t n = (cubec_expression_subscript_t)node;
    c_ir_node_t obj = lower_expr(allocator, ctx, n->host, module_hash, unit);
    if (!obj) return NULL;

    /* Extract the comptime index value */
    uint64_t idx = 0;
    if (n->index->kind == CUBEC_NODE_LITERAL_NUMERIC) {
      cubec_literal_numeric_t num = (cubec_literal_numeric_t)n->index;
      idx = strtoull(string_get(num->value), NULL, 10);
    }

    /* Build the field name: _0, _1, _2, ... */
    char field_buf[16];
    snprintf(field_buf, sizeof(field_buf), "_%llu", (unsigned long long)idx);

    /* Determine is_arrow from host type */
    bool is_arrow = false;
    semantic_type_t host_type = lower_infer_type(allocator, ctx, n->host, module_hash, unit);
    if (host_type) {
      enum type_kind k = semantic_type_get_kind(host_type);
      if (k == TYPE_POINTER) {
        is_arrow = true;
      } else if (k == TYPE_QUALIFIER) {
        type_impl_t impl = semantic_type_get_impl(host_type);
        semantic_type_t base = impl->qualifier.base;
        if (base && semantic_type_get_kind(base) == TYPE_POINTER)
          is_arrow = true;
      }
    }

    return (c_ir_node_t)c_ir_expr_member_create(allocator, obj, field_buf,
                                                   is_arrow, loc);
  }

  /* ===== Generic instantiation ===== */
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION: {
    /* Generic instantiation is resolved by checker to concrete type/function.
     * Just produce the mangled identifier for the resolved symbol. */
    cubec_expression_generic_instantiation_t n =
        (cubec_expression_generic_instantiation_t)node;

    /* Try to get the resolved type/function via lower's type inference */
    semantic_type_t resolved_type = lower_infer_type(allocator, ctx, (node_t)n, module_hash, unit);
    if (resolved_type) {
      const char *type_name = semantic_type_get_name(resolved_type);
      if (type_name) {
        string_t mangled = mangle_name(allocator, module_hash, type_name);
        c_ir_node_t result = (c_ir_node_t)c_ir_expr_ident_create(
            allocator, string_get(mangled), loc);
        allocator_free(allocator, &mangled);
        return result;
      }
    }

    /* Subscript: callee is a variable (not type/function) — t[N] */
    if (n->callee && n->callee->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
      const char *callee_name =
          string_get(((cubec_literal_identifier_t)n->callee)->value);
      struct symbol *sym = callee_name ? scope_lookup(ctx->current_scope, callee_name) : NULL;
      if (sym && sym->kind == SYMBOL_VARIABLE) {
        /* Lower as tuple subscript */
        c_ir_node_t obj = lower_expr(allocator, ctx, n->callee, module_hash, unit);
        if (!obj) return NULL;
        /* Determine field name from index */
        if (n->arguments && vec_get_size(n->arguments) >= 1) {
          node_t idx_node = (node_t)vec_get(n->arguments, 0);
          if (idx_node->kind == CUBEC_NODE_LITERAL_NUMERIC) {
            cubec_literal_numeric_t num = (cubec_literal_numeric_t)idx_node;
            uint64_t idx = strtoull(string_get(num->value), NULL, 10);
            char field_buf[16];
            snprintf(field_buf, sizeof(field_buf), "_%" PRIu64, idx);
            /* Determine is_arrow from host type */
            bool is_arrow = false;
            if (sym->variable.type) {
              enum type_kind k = semantic_type_get_kind(sym->variable.type);
              if (k == TYPE_POINTER) {
                is_arrow = true;
              } else if (k == TYPE_QUALIFIER) {
                type_impl_t impl = semantic_type_get_impl(sym->variable.type);
                semantic_type_t base = impl->qualifier.base;
                if (base && semantic_type_get_kind(base) == TYPE_POINTER)
                  is_arrow = true;
              }
            }
            return (c_ir_node_t)c_ir_expr_member_create(allocator, obj, field_buf,
                                                         is_arrow, loc);
          }
        }
        return obj;
      }
    }

    /* Fallback: lower the callee identifier */
    if (n->callee) {
      return lower_expr(allocator, ctx, n->callee, module_hash, unit);
    }

    return NULL;
  }

  /* ===== Anonymous function ===== */
  case CUBEC_NODE_EXPRESSION_FUNCTION: {
    cubec_expression_function_t n = (cubec_expression_function_t)node;

    /* Generate unique name */
    char name_buf[64];
    snprintf(name_buf, sizeof(name_buf), "%s__lambda_%d", module_hash,
             _anon_func_counter++);
    string_t mangled_name = allocator_create(allocator, &g_string_type,
                                              &(string_init_t){.str = name_buf});

    /* Get function type via lower's type inference */
    semantic_type_t func_type = lower_infer_type(allocator, ctx, (node_t)n, module_hash, unit);

    /* Build params */
    vec_t params = allocator_create(allocator, &g_vec_type,
                                     &(vec_init_t){.auto_dispose = false});
    c_type_t ret_type = c_type_primitive(allocator, "void");

    if (func_type && semantic_type_get_kind(func_type) == TYPE_FUNCTION) {
      type_impl_t impl = semantic_type_get_impl(func_type);
      ret_type = lower_type(allocator, impl->function.return_type, module_hash, unit);
      size_t param_count = impl->function.params
          ? vec_get_size(impl->function.params) : 0;
      for (size_t j = 0; j < param_count; j++) {
        semantic_type_t pt = vec_get(impl->function.params, j);
        c_type_t ct = lower_type(allocator, pt, module_hash, unit);
        const char *pn = "_";
        if (n->arguments && j < vec_get_size(n->arguments)) {
          cubec_function_argument_t arg = vec_get(n->arguments, j);
          if (arg->identifier) {
            cubec_literal_identifier_t id = (cubec_literal_identifier_t)arg->identifier;
            pn = string_get(id->value);
          }
        }
        c_ir_param_t p = c_ir_param_create(allocator, ct, pn);
        vec_push(params, p);
      }
    } else {
      /* Fallback: extract params from AST */
      size_t arg_count = n->arguments ? vec_get_size(n->arguments) : 0;
      for (size_t j = 0; j < arg_count; j++) {
        cubec_function_argument_t arg = vec_get(n->arguments, j);
        const char *pn = "_";
        if (arg->identifier) {
          cubec_literal_identifier_t id = (cubec_literal_identifier_t)arg->identifier;
          pn = string_get(id->value);
        }
        c_type_t ct = c_type_primitive(allocator, "void");
        c_ir_param_t p = c_ir_param_create(allocator, ct, pn);
        vec_push(params, p);
      }
    }

    /* Lower body */
    c_ir_node_t body = n->body
        ? lower_stmt(allocator, ctx, n->body, module_hash, NULL, unit)
        : NULL;

    /* Add static function definition to unit */
    if (unit) {
      c_ir_node_t def = (c_ir_node_t)c_ir_function_def_create(
          allocator, ret_type, string_get(mangled_name), params,
          true,   /* is_static */
          false,  /* is_inline */
          true,   /* is_hidden — no_instrument_function */
          false,  /* is_artificial */
          false,  /* is_c_variadic */
          body, loc);
      vec_push(unit->function_defs, def);
    } else {
      /* No unit available — just dispose params */
      for (size_t j = 0; j < vec_get_size(params); j++) {
        c_ir_param_t p = vec_get(params, j);
        c_ir_param_dispose(allocator, &p);
      }
      allocator_free(allocator, &params);
    }

    /* Return function pointer identifier */
    c_ir_node_t result = (c_ir_node_t)c_ir_expr_ident_create(
        allocator, string_get(mangled_name), loc);
    allocator_free(allocator, &mangled_name);
    return result;
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
