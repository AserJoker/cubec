#include "engine/runtime_collection.h"
#include "engine/context.h"
#include "engine/resolver.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/statement_function.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_return.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_defer.h"
#include "cubec/statement_switch.h"
#include "cubec/switch_match.h"
#include "cubec/declaration_variable.h"
#include "cubec/expression_function.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/function_argument.h"
#include "cubec/expression_member.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_call.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_group.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_alignof.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_comma.h"
#include <string.h>

/* ===== Pointer-to-string key helper ===== */

static void ptr_to_key(char *buf, size_t bufsize, const void *ptr) {
  snprintf(buf, bufsize, "%p", ptr);
}

/* ===== Create / Dispose ===== */

runtime_collection_t runtime_collection_create(allocator_t allocator) {
  runtime_collection_t rc = allocator_alloc(allocator, sizeof(struct _runtime_collection_t));
  rc->runtime_types = allocator_create(allocator, &g_vec_type,
                                        &(vec_init_t){.auto_dispose = false});
  rc->runtime_functions = allocator_create(allocator, &g_vec_type,
                                            &(vec_init_t){.auto_dispose = false});
  rc->runtime_variables = allocator_create(allocator, &g_vec_type,
                                            &(vec_init_t){.auto_dispose = false});
  rc->type_worklist = allocator_create(allocator, &g_vec_type,
                                        &(vec_init_t){.auto_dispose = false});
  rc->func_worklist = allocator_create(allocator, &g_vec_type,
                                        &(vec_init_t){.auto_dispose = false});
  rc->seen_types = allocator_create(allocator, &g_strmap_type, NULL);
  rc->seen_funcs = allocator_create(allocator, &g_strmap_type, NULL);
  rc->seen_vars = allocator_create(allocator, &g_strmap_type, NULL);
  return rc;
}

void runtime_collection_dispose(runtime_collection_t rc, allocator_t allocator) {
  if (!rc) return;
  allocator_free(allocator, &rc->seen_vars);
  allocator_free(allocator, &rc->seen_funcs);
  allocator_free(allocator, &rc->seen_types);
  allocator_free(allocator, &rc->func_worklist);
  allocator_free(allocator, &rc->type_worklist);
  allocator_free(allocator, &rc->runtime_variables);
  allocator_free(allocator, &rc->runtime_functions);
  allocator_free(allocator, &rc->runtime_types);
  allocator_free(allocator, &rc);
}

/* ===== Add helpers (with dedup) ===== */

static bool add_type(runtime_collection_t rc, semantic_type_t type) {
  char key[32];
  ptr_to_key(key, sizeof(key), type);
  if (strmap_insert(rc->seen_types, key, (void *)(uintptr_t)1)) return false;
  vec_push(rc->runtime_types, type);
  vec_push(rc->type_worklist, type);
  return true;
}

static bool add_function(runtime_collection_t rc, struct symbol *sym) {
  char key[32];
  ptr_to_key(key, sizeof(key), sym);
  if (strmap_insert(rc->seen_funcs, key, (void *)(uintptr_t)1)) return false;
  vec_push(rc->runtime_functions, sym);
  vec_push(rc->func_worklist, sym);
  return true;
}

static bool add_variable(runtime_collection_t rc, struct symbol *sym) {
  char key[32];
  ptr_to_key(key, sizeof(key), sym);
  if (strmap_insert(rc->seen_vars, key, (void *)(uintptr_t)1)) return false;
  vec_push(rc->runtime_variables, sym);
  return true;
}

/* ===== Type diffusion ===== */

static void diffuse_type(runtime_collection_t rc, semantic_type_t type);

static void diffuse_type(runtime_collection_t rc, semantic_type_t type) {
  if (!type) return;

  enum type_kind kind = semantic_type_get_kind(type);
  type_impl_t impl = semantic_type_get_impl(type);

  switch (kind) {
  /* Composite types: add to runtime list and recurse into fields */
  case TYPE_STRUCT:
  case TYPE_UNION:
  case TYPE_CUNION: {
    if (!add_type(rc, type)) return;  /* already seen */
    /* Recurse into field types */
    if (impl->struct_type.fields) {
      size_t fc = vec_get_size(impl->struct_type.fields);
      for (size_t i = 0; i < fc; i++) {
        struct symbol *field = vec_get(impl->struct_type.fields, i);
        if (field->field.type) diffuse_type(rc, field->field.type);
      }
    }
    /* Collect instance methods as runtime functions */
    if (type->instance_methods) {
      size_t mc = vec_get_size(type->instance_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *method = vec_get(type->instance_methods, i);
        add_function(rc, method);
      }
    }
    /* Static methods */
    if (type->static_methods) {
      size_t mc = vec_get_size(type->static_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *method = vec_get(type->static_methods, i);
        add_function(rc, method);
      }
    }
    break;
  }

  case TYPE_GENERIC_INSTANCE: {
    if (!add_type(rc, type)) return;
    /* Recurse into substituted field types */
    if (impl->generic_instance.fields) {
      size_t fc = vec_get_size(impl->generic_instance.fields);
      for (size_t i = 0; i < fc; i++) {
        struct symbol *field = vec_get(impl->generic_instance.fields, i);
        if (field->field.type) diffuse_type(rc, field->field.type);
      }
    }
    /* Instance methods on the generic instance */
    if (type->instance_methods) {
      size_t mc = vec_get_size(type->instance_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *method = vec_get(type->instance_methods, i);
        add_function(rc, method);
      }
    }
    break;
  }

  case TYPE_ENUM: {
    if (!add_type(rc, type)) return;
    /* No sub-types to recurse for enums */
    break;
  }

  case TYPE_SLICE: {
    if (!add_type(rc, type)) return;
    diffuse_type(rc, impl->slice.element);
    break;
  }

  case TYPE_TUPLE: {
    if (!add_type(rc, type)) return;
    /* Recurse into tuple field types */
    if (impl->tuple.fields) {
      size_t fc = vec_get_size(impl->tuple.fields);
      for (size_t i = 0; i < fc; i++) {
        struct symbol *field = vec_get(impl->tuple.fields, i);
        if (field->field.type) diffuse_type(rc, field->field.type);
      }
    }
    break;
  }

  /* Container types: recurse into contained type */
  case TYPE_POINTER:
    diffuse_type(rc, impl->pointer.pointee);
    break;

  case TYPE_ARRAY:
    diffuse_type(rc, impl->array.element);
    break;

  case TYPE_FUNCTION:
    diffuse_type(rc, impl->function.return_type);
    if (impl->function.params) {
      size_t pc = vec_get_size(impl->function.params);
      for (size_t i = 0; i < pc; i++) {
        semantic_type_t pt = vec_get(impl->function.params, i);
        diffuse_type(rc, pt);
      }
    }
    break;

  case TYPE_QUALIFIER:
    diffuse_type(rc, impl->qualifier.base);
    break;

  /* String: no C struct needed (const char*) */
  case TYPE_STRING:
  case TYPE_STR:
    break;

  /* Primitives: no action */
  case TYPE_I8: case TYPE_I16: case TYPE_I32: case TYPE_I64:
  case TYPE_U8: case TYPE_U16: case TYPE_U32: case TYPE_U64:
  case TYPE_F16: case TYPE_F32: case TYPE_F64:
  case TYPE_BOOL: case TYPE_VOID: case TYPE_CHAR:
  case TYPE_NIL: case TYPE_OPAQUE: case TYPE_WILDCARD:
    break;

  /* No C output for these */
  case TYPE_INTERFACE:
  case TYPE_GENERIC_PARAM:
  case TYPE_GENERIC_PACK:
  case TYPE_GENERIC_VALUE:
  case TYPE_PACK_INDEX:
  case TYPE_TYPE:
  case TYPE_MODULE:
  case TYPE_ERROR:
    break;
  }
}

/* ===== AST Walkers for function body type collection ===== */

static void diffuse_expr(runtime_collection_t rc, struct context *ctx, node_t node);

static void diffuse_stmt(runtime_collection_t rc, struct context *ctx, node_t node) {
  if (!node) return;

  /* Skip error nodes */
  if (node->kind == CUBEC_NODE_ERROR || node->kind == CUBEC_NODE_STATEMENT_ERROR)
    return;

  switch (node->kind) {

  /* Block */
  case CUBEC_NODE_STATEMENT_BLOCK: {
    cubec_statement_block_t n = (cubec_statement_block_t)node;
    size_t count = n->statements ? vec_get_size(n->statements) : 0;
    for (size_t i = 0; i < count; i++) {
      diffuse_stmt(rc, ctx, vec_get(n->statements, i));
    }
    break;
  }

  /* Declaration — diffuse variable type and initializer */
  case CUBEC_NODE_STATEMENT_DECLARATION: {
    cubec_statement_declaration_t n = (cubec_statement_declaration_t)node;
    cubec_declaration_variable_t var = (cubec_declaration_variable_t)n->declarator;
    if (var) {
      if (var->type) {
        semantic_type_t t = resolver_resolve_type(ctx, var->type);
        if (t && t->impl->kind != TYPE_ERROR) diffuse_type(rc, t);
      }
      if (var->expression) {
        diffuse_expr(rc, ctx, var->expression);
      }
    }
    break;
  }

  /* Expression statement */
  case CUBEC_NODE_STATEMENT_EXPRESSION: {
    cubec_statement_expression_t n = (cubec_statement_expression_t)node;
    if (n->expression) diffuse_expr(rc, ctx, n->expression);
    break;
  }

  /* Return */
  case CUBEC_NODE_STATEMENT_RETURN: {
    cubec_statement_return_t n = (cubec_statement_return_t)node;
    if (n->expression) diffuse_expr(rc, ctx, n->expression);
    break;
  }

  /* If */
  case CUBEC_NODE_STATEMENT_IF: {
    cubec_statement_if_t n = (cubec_statement_if_t)node;
    if (n->condition) diffuse_expr(rc, ctx, n->condition);
    if (n->then_branch) diffuse_stmt(rc, ctx, n->then_branch);
    if (n->else_branch) diffuse_stmt(rc, ctx, n->else_branch);
    break;
  }

  /* While */
  case CUBEC_NODE_STATEMENT_WHILE: {
    cubec_statement_while_t n = (cubec_statement_while_t)node;
    if (n->condition) diffuse_expr(rc, ctx, n->condition);
    if (n->body) diffuse_stmt(rc, ctx, n->body);
    break;
  }

  /* Do-while */
  case CUBEC_NODE_STATEMENT_DO_WHILE: {
    cubec_statement_do_while_t n = (cubec_statement_do_while_t)node;
    if (n->body) diffuse_stmt(rc, ctx, n->body);
    if (n->condition) diffuse_expr(rc, ctx, n->condition);
    break;
  }

  /* For */
  case CUBEC_NODE_STATEMENT_FOR: {
    cubec_statement_for_t n = (cubec_statement_for_t)node;
    if (n->init) diffuse_stmt(rc, ctx, n->init);
    if (n->condition) diffuse_expr(rc, ctx, n->condition);
    if (n->increment) diffuse_expr(rc, ctx, n->increment);
    if (n->body) diffuse_stmt(rc, ctx, n->body);
    break;
  }

  /* Foreach */
  case CUBEC_NODE_STATEMENT_FOREACH: {
    cubec_statement_foreach_t n = (cubec_statement_foreach_t)node;
    if (n->iterator) diffuse_expr(rc, ctx, n->iterator);
    if (n->var_type) {
      semantic_type_t t = resolver_resolve_type(ctx, n->var_type);
      if (t && t->impl->kind != TYPE_ERROR) diffuse_type(rc, t);
    }
    if (n->body) diffuse_stmt(rc, ctx, n->body);
    break;
  }

  /* Defer */
  case CUBEC_NODE_STATEMENT_DEFER: {
    cubec_statement_defer_t n = (cubec_statement_defer_t)node;
    if (n->body) diffuse_stmt(rc, ctx, n->body);
    break;
  }

  /* Switch */
  case CUBEC_NODE_STATEMENT_SWITCH: {
    cubec_statement_switch_t n = (cubec_statement_switch_t)node;
    if (n->condition) diffuse_expr(rc, ctx, n->condition);
    if (n->matches) {
      size_t mc = vec_get_size(n->matches);
      for (size_t i = 0; i < mc; i++) {
        cubec_switch_match_t m = vec_get(n->matches, i);
        if (m->values) {
          size_t vc = vec_get_size(m->values);
          for (size_t j = 0; j < vc; j++) {
            diffuse_expr(rc, ctx, vec_get(m->values, j));
          }
        }
        if (m->body) diffuse_stmt(rc, ctx, m->body);
      }
    }
    break;
  }

  /* Nested function declaration */
  case CUBEC_NODE_STATEMENT_FUNCTION: {
    cubec_statement_function_t n = (cubec_statement_function_t)node;
    /* Skip generic functions — they're instantiated at comptime */
    if (n->generic_params) break;
    /* Diffuse return type */
    if (n->return_type) {
      semantic_type_t t = resolver_resolve_type(ctx, n->return_type);
      if (t && t->impl->kind != TYPE_ERROR) diffuse_type(rc, t);
    }
    /* Diffuse argument types */
    if (n->arguments) {
      size_t ac = vec_get_size(n->arguments);
      for (size_t i = 0; i < ac; i++) {
        cubec_function_argument_t arg = vec_get(n->arguments, i);
        if (arg->type) {
          semantic_type_t t = resolver_resolve_type(ctx, arg->type);
          if (t && t->impl->kind != TYPE_ERROR) diffuse_type(rc, t);
        }
      }
    }
    /* Recurse into body */
    if (n->body) diffuse_stmt(rc, ctx, n->body);
    break;
  }

  /* Nodes with no type information to collect */
  case CUBEC_NODE_STATEMENT_BREAK:
  case CUBEC_NODE_STATEMENT_CONTINUE:
  case CUBEC_NODE_STATEMENT_EMPTY:
  case CUBEC_NODE_STATEMENT_IMPORT:
  case CUBEC_NODE_STATEMENT_COMPTIME_IF:
  case CUBEC_NODE_STATEMENT_COMPTIME_FOREACH:
  case CUBEC_NODE_STATEMENT_TEST:
  case CUBEC_NODE_STATEMENT_EXPORT_FROM:
  case CUBEC_NODE_STATEMENT_INTERFACE:
  case CUBEC_NODE_STATEMENT_STRUCT:
  case CUBEC_NODE_STATEMENT_ENUM:
  case CUBEC_NODE_STATEMENT_UNION:
  case CUBEC_NODE_STATEMENT_CUNION:
  case CUBEC_NODE_STATEMENT_DECLARATION_TYPE:
    break;

  default:
    break;
  }
}

static void diffuse_expr(runtime_collection_t rc, struct context *ctx, node_t node) {
  if (!node) return;

  /* Skip error nodes */
  if (node->kind == CUBEC_NODE_ERROR || node->kind == CUBEC_NODE_STATEMENT_ERROR)
    return;

  switch (node->kind) {

  /* Initialize list — the type field references the struct being constructed */
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST: {
    cubec_expression_initialize_list_t n = (cubec_expression_initialize_list_t)node;
    if (n->type) {
      semantic_type_t t = resolver_resolve_type(ctx, n->type);
      if (t && t->impl->kind != TYPE_ERROR) diffuse_type(rc, t);
    }
    /* Recurse into items (field values) */
    if (n->items) {
      size_t ic = vec_get_size(n->items);
      for (size_t i = 0; i < ic; i++) {
        diffuse_expr(rc, ctx, vec_get(n->items, i));
      }
    }
    break;
  }

  /* Anonymous/expression function */
  case CUBEC_NODE_EXPRESSION_FUNCTION: {
    cubec_expression_function_t n = (cubec_expression_function_t)node;
    /* Diffuse return type */
    if (n->return_type) {
      semantic_type_t t = resolver_resolve_type(ctx, n->return_type);
      if (t && t->impl->kind != TYPE_ERROR) diffuse_type(rc, t);
    }
    /* Diffuse argument types */
    if (n->arguments) {
      size_t ac = vec_get_size(n->arguments);
      for (size_t i = 0; i < ac; i++) {
        cubec_function_argument_t arg = vec_get(n->arguments, i);
        if (arg->type) {
          semantic_type_t t = resolver_resolve_type(ctx, arg->type);
          if (t && t->impl->kind != TYPE_ERROR) diffuse_type(rc, t);
        }
      }
    }
    /* Recurse into body */
    if (n->body) diffuse_stmt(rc, ctx, n->body);
    break;
  }

  /* Call — recurse into callee and arguments */
  case CUBEC_NODE_EXPRESSION_CALL: {
    cubec_expression_call_t n = (cubec_expression_call_t)node;
    if (n->callee) diffuse_expr(rc, ctx, n->callee);
    if (n->arguments) {
      size_t ac = vec_get_size(n->arguments);
      for (size_t i = 0; i < ac; i++) {
        diffuse_expr(rc, ctx, vec_get(n->arguments, i));
      }
    }
    break;
  }

  /* Member access — the host expression's type may be a struct */
  case CUBEC_NODE_EXPRESSION_MEMBER: {
    cubec_expression_member_t n = (cubec_expression_member_t)node;
    if (n->host) diffuse_expr(rc, ctx, n->host);
    /* Diffuse the field type by looking up the field in the host's type.
     * We infer the host type from the host expression, then find the field
     * by name in the struct's fields list. */
    {
      const char *fname = n->field ? string_get(n->field->value) : NULL;
      if (fname && n->host) {
        /* Try to infer the host type */
        semantic_type_t host_type = NULL;
        if (n->host->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
          const char *hname = string_get(((cubec_literal_identifier_t)n->host)->value);
          struct symbol *sym = scope_lookup(ctx->current_scope, hname);
          if (sym) {
            switch (sym->kind) {
            case SYMBOL_VARIABLE: host_type = sym->variable.type; break;
            case SYMBOL_FUNCTION: host_type = sym->function.type; break;
            case SYMBOL_TYPE:     host_type = sym->type.type; break;
            default: break;
            }
          }
        }
        if (host_type) {
          /* Strip qualifiers and deref pointers */
          enum type_kind k = semantic_type_get_kind(host_type);
          while (k == TYPE_QUALIFIER) {
            host_type = semantic_type_get_impl(host_type)->qualifier.base;
            k = semantic_type_get_kind(host_type);
          }
          if (k == TYPE_POINTER) {
            host_type = semantic_type_get_impl(host_type)->pointer.pointee;
            k = semantic_type_get_kind(host_type);
          }
          /* Find field in struct-like type */
          vec_t fields = NULL;
          if (k == TYPE_STRUCT || k == TYPE_UNION || k == TYPE_CUNION)
            fields = semantic_type_get_impl(host_type)->struct_type.fields;
          else if (k == TYPE_GENERIC_INSTANCE)
            fields = semantic_type_get_impl(host_type)->generic_instance.fields;
          else if (k == TYPE_TUPLE)
            fields = semantic_type_get_impl(host_type)->tuple.fields;
          if (fields) {
            size_t fcount = vec_get_size(fields);
            for (size_t fi = 0; fi < fcount; fi++) {
              struct symbol *f = vec_get(fields, fi);
              if (f && f->name && strcmp(f->name, fname) == 0 && f->field.type) {
                diffuse_type(rc, f->field.type);
                break;
              }
            }
          }
          /* Also check instance methods */
          if (host_type->instance_methods) {
            size_t mc = vec_get_size(host_type->instance_methods);
            for (size_t mi = 0; mi < mc; mi++) {
              struct symbol *m = vec_get(host_type->instance_methods, mi);
              if (m && m->name && strcmp(m->name, fname) == 0) {
                diffuse_type(rc, m->function.type);
                break;
              }
            }
          }
        }
      }
    }
    break;
  }

  /* Binary — recurse left and right */
  case CUBEC_NODE_EXPRESSION_BINARY: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    if (n->left) diffuse_expr(rc, ctx, n->left);
    if (n->right) diffuse_expr(rc, ctx, n->right);
    break;
  }

  /* Assignment (reuses binary_t: left=lvalue, right=rvalue) */
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    if (n->left) diffuse_expr(rc, ctx, n->left);
    if (n->right) diffuse_expr(rc, ctx, n->right);
    break;
  }

  /* Comma expression */
  case CUBEC_NODE_EXPRESSION_COMMA: {
    cubec_expression_comma_t n = (cubec_expression_comma_t)node;
    if (n->left) diffuse_expr(rc, ctx, n->left);
    if (n->right) diffuse_expr(rc, ctx, n->right);
    break;
  }

  /* Ternary */
  case CUBEC_NODE_EXPRESSION_TERNARY: {
    cubec_expression_ternary_t n = (cubec_expression_ternary_t)node;
    if (n->condition) diffuse_expr(rc, ctx, n->condition);
    if (n->consequent) diffuse_expr(rc, ctx, n->consequent);
    if (n->alternate) diffuse_expr(rc, ctx, n->alternate);
    break;
  }

  /* Dereference (postfix .*) */
  case CUBEC_NODE_EXPRESSION_DEREF: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    if (n->left) diffuse_expr(rc, ctx, n->left);
    break;
  }

  /* Address-of (postfix .&) */
  case CUBEC_NODE_EXPRESSION_ADDR: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    if (n->left) diffuse_expr(rc, ctx, n->left);
    break;
  }

  /* Group (parenthesized) */
  case CUBEC_NODE_EXPRESSION_GROUP: {
    cubec_expression_group_t n = (cubec_expression_group_t)node;
    if (n->inner) diffuse_expr(rc, ctx, n->inner);
    break;
  }

  /* Slice expression */
  case CUBEC_NODE_EXPRESSION_SLICE: {
    cubec_expression_slice_t n = (cubec_expression_slice_t)node;
    if (n->host) diffuse_expr(rc, ctx, n->host);
    if (n->start) diffuse_expr(rc, ctx, n->start);
    if (n->length) diffuse_expr(rc, ctx, n->length);
    break;
  }

  /* Namespace access */
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS: {
    cubec_expression_namespace_access_t n = (cubec_expression_namespace_access_t)node;
    if (n->host) diffuse_expr(rc, ctx, n->host);
    break;
  }

  /* Try (.?) */
  case CUBEC_NODE_EXPRESSION_TRY: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    if (n->left) diffuse_expr(rc, ctx, n->left);
    break;
  }

  /* Assert (.!) */
  case CUBEC_NODE_EXPRESSION_ASSERT: {
    cubec_expression_binary_t n = (cubec_expression_binary_t)node;
    if (n->left) diffuse_expr(rc, ctx, n->left);
    break;
  }

  /* Sizeof — the operand is a type expression or value */
  case CUBEC_NODE_EXPRESSION_SIZEOF: {
    cubec_expression_sizeof_t n = (cubec_expression_sizeof_t)node;
    if (n->expression) {
      semantic_type_t t = resolver_resolve_type(ctx, n->expression);
      if (t && t->impl->kind != TYPE_ERROR) diffuse_type(rc, t);
    }
    break;
  }

  /* Alignof */
  case CUBEC_NODE_EXPRESSION_ALIGNOF: {
    cubec_expression_alignof_t n = (cubec_expression_alignof_t)node;
    if (n->expression) {
      semantic_type_t t = resolver_resolve_type(ctx, n->expression);
      if (t && t->impl->kind != TYPE_ERROR) diffuse_type(rc, t);
    }
    break;
  }

  /* Typeof */
  case CUBEC_NODE_EXPRESSION_TYPEOF: {
    cubec_expression_typeof_t n = (cubec_expression_typeof_t)node;
    if (n->expression) diffuse_expr(rc, ctx, n->expression);
    break;
  }

  /* Generic instantiation */
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION: {
    cubec_expression_generic_instantiation_t n =
        (cubec_expression_generic_instantiation_t)node;
    if (n->callee) {
      semantic_type_t t = resolver_resolve_type(ctx, n->callee);
      if (t && t->impl->kind != TYPE_ERROR) diffuse_type(rc, t);
    }
    if (n->arguments) {
      size_t ac = vec_get_size(n->arguments);
      for (size_t i = 0; i < ac; i++) {
        semantic_type_t t = resolver_resolve_type(ctx, vec_get(n->arguments, i));
        if (t && t->impl->kind != TYPE_ERROR) diffuse_type(rc, t);
      }
    }
    break;
  }

  /* Leaf nodes — no type diffusion needed */
  case CUBEC_NODE_LITERAL_IDENTIFIER:
  case CUBEC_NODE_LITERAL_NUMERIC:
  case CUBEC_NODE_LITERAL_STRING:
  case CUBEC_NODE_LITERAL_CHAR:
  case CUBEC_NODE_LITERAL_UNDEFINED:
  case CUBEC_NODE_EXPRESSION_WILDCARD:
  case CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD:
  case CUBEC_NODE_EXPRESSION_COMPUTE_MEMBER:
  case CUBEC_NODE_EXPRESSION_SPREAD:
  case CUBEC_NODE_EXPRESSION_TYPE_ENUM:
  case CUBEC_NODE_EXPRESSION_TYPE_UNION:
  case CUBEC_NODE_EXPRESSION_TYPE_FUNCTION:
  case CUBEC_NODE_EXPRESSION_TYPE_INTERFACE:
  case CUBEC_NODE_EXPRESSION_TYPE_STRUCT:
  case CUBEC_NODE_EXPRESSION_TYPE_TUPLE:
  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER:
    break;

  default:
    break;
  }
}

/* ===== Function diffusion ===== */

static void diffuse_function(runtime_collection_t rc, struct symbol *sym,
                              struct context *ctx) {
  if (!add_function(rc, sym)) return;  /* already seen */

  /* Diffuse function type (params + return) */
  if (sym->function.type) {
    type_impl_t impl = semantic_type_get_impl(sym->function.type);
    diffuse_type(rc, impl->function.return_type);
    if (impl->function.params) {
      size_t pc = vec_get_size(impl->function.params);
      for (size_t i = 0; i < pc; i++) {
        semantic_type_t pt = vec_get(impl->function.params, i);
        diffuse_type(rc, pt);
      }
    }
  }

  /* Walk the function body AST to collect types referenced inside.
   * This catches struct types used for local variables, initializer lists,
   * member accesses, etc. that don't appear in the function signature. */
  if (sym->function.ast_node && ctx) {
    cubec_statement_function_t fn_ast =
        (cubec_statement_function_t)sym->function.ast_node;
    if (fn_ast->body) {
      diffuse_stmt(rc, ctx, fn_ast->body);
    }
  }
}

/* ===== Main Pass 5 entry point ===== */

void context_collect_runtime(struct context *ctx, node_t program, bool generate_executable) {
  if (!ctx || !program) return;

  runtime_collection_t rc = ctx->runtime;
  scope_t scope = ctx->global_scope;
  vec_t symbols = scope_get_symbols(scope);
  size_t count = vec_get_size(symbols);

  /* Step 1: Seed from exported functions, main, and non-comptime variables */
  for (size_t i = 0; i < count; i++) {
    struct symbol *sym = vec_get(symbols, i);

    if (sym->kind == SYMBOL_FUNCTION) {
      cubec_statement_function_t fn_ast =
          (cubec_statement_function_t)sym->function.ast_node;

      /* Skip comptime, builtin, and generic functions */
      if (sym->function.is_comptime || sym->is_builtin) continue;
      if (fn_ast && fn_ast->generic_params) continue;

      /* Export functions are entry points */
      if (sym->is_export) {
        diffuse_function(rc, sym, ctx);
      }

      /* Main function is an entry point for executables */
      if (generate_executable && strcmp(sym->name, "main") == 0) {
        diffuse_function(rc, sym, ctx);
      }

      /* Non-export functions with bodies are also included (they may be
       * called internally). For now, include all non-comptime non-builtin
       * non-generic functions — the dead code elimination can be refined later. */
      if (fn_ast && fn_ast->body && !fn_ast->is_extern) {
        diffuse_function(rc, sym, ctx);
      }

      /* Extern functions: include export extern in .h, non-export in .c */
      if (fn_ast && fn_ast->is_extern) {
        diffuse_function(rc, sym, ctx);
      }
    }

    if (sym->kind == SYMBOL_VARIABLE) {
      /* Skip comptime and builtin variables */
      if (sym->variable.is_comptime || sym->is_builtin) continue;

      /* Add all non-comptime variables */
      if (add_variable(rc, sym)) {
        /* Diffuse the variable's type */
        if (sym->variable.type) {
          diffuse_type(rc, sym->variable.type);
        }
      }
    }

    if (sym->kind == SYMBOL_TYPE) {
      /* Skip comptime and builtin types */
      if (sym->is_builtin) continue;
      /* Skip generic type definitions (they are instantiated at comptime) */
      if (sym->type.generic_params) continue;
      /* Diffuse the type — this adds it to runtime_types and recurses into fields */
      if (sym->type.type) {
        diffuse_type(rc, sym->type.type);
      }
    }
  }

  /* Step 2: Process type worklist — types added by function diffusion
   * may reference new types (e.g., struct fields pointing to other structs).
   * Continue until stable. */
  size_t type_idx = 0;
  while (type_idx < vec_get_size(rc->type_worklist)) {
    semantic_type_t type __attribute__((unused)) = vec_get(rc->type_worklist, type_idx);
    type_idx++;
    /* diffuse_type already recurses when first added; the worklist
     * just tracks which types have been added. Methods from newly-added
     * types were already added to func_worklist by diffuse_type. */
  }

  /* Step 3: Process function worklist — methods from type diffusion
   * may reference new types. Continue until stable. */
  size_t func_idx = 0;
  while (func_idx < vec_get_size(rc->func_worklist)) {
    struct symbol *sym __attribute__((unused)) = vec_get(rc->func_worklist, func_idx);
    func_idx++;
    /* diffuse_function already handled type diffusion when first added.
     * But we need to check if any new types were added by this function
     * that have methods we haven't processed yet. The add_function
     * dedup prevents reprocessing. */
  }

  /* Step 4: Fixpoint iteration — new methods may introduce new types.
   * Repeat until no new types or functions are discovered. */
  size_t max_iterations = (vec_get_size(rc->runtime_types) +
                            vec_get_size(rc->runtime_functions)) * 2 + 10;
  for (size_t iter = 0; iter < max_iterations; iter++) {
    size_t prev_types = vec_get_size(rc->runtime_types);
    size_t prev_funcs = vec_get_size(rc->runtime_functions);

    /* Re-process all types to pick up any methods from newly added types */
    for (size_t i = 0; i < vec_get_size(rc->runtime_types); i++) {
      semantic_type_t type = vec_get(rc->runtime_types, i);
      type_impl_t impl __attribute__((unused)) = semantic_type_get_impl(type);
      enum type_kind kind = semantic_type_get_kind(type);

      /* Collect methods from struct/union/cunion */
      if ((kind == TYPE_STRUCT || kind == TYPE_UNION || kind == TYPE_CUNION) &&
          type->instance_methods) {
        size_t mc = vec_get_size(type->instance_methods);
        for (size_t j = 0; j < mc; j++) {
          struct symbol *method = vec_get(type->instance_methods, j);
          diffuse_function(rc, method, ctx);
        }
      }
      if ((kind == TYPE_STRUCT || kind == TYPE_UNION || kind == TYPE_CUNION) &&
          type->static_methods) {
        size_t mc = vec_get_size(type->static_methods);
        for (size_t j = 0; j < mc; j++) {
          struct symbol *method = vec_get(type->static_methods, j);
          diffuse_function(rc, method, ctx);
        }
      }

      /* Collect methods from generic instances */
      if (kind == TYPE_GENERIC_INSTANCE && type->instance_methods) {
        size_t mc = vec_get_size(type->instance_methods);
        for (size_t j = 0; j < mc; j++) {
          struct symbol *method = vec_get(type->instance_methods, j);
          diffuse_function(rc, method, ctx);
        }
      }
    }

    /* Check if anything new was discovered */
    if (vec_get_size(rc->runtime_types) == prev_types &&
        vec_get_size(rc->runtime_functions) == prev_funcs) {
      break;  /* stable — no new discoveries */
    }
  }
}
