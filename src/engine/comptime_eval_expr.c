#include "engine/comptime_eval_internal.h"
#include "engine/comptime_eval_binary.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_layout.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/strmap.h"
#include "core/vec.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_char.h"
#include "cubec/literal_identifier.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_call.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_group.h"
#include "cubec/expression_member.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_alignof.h"
#include "cubec/expression_function.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_initialize_field.h"
#include "cubec/expression_comma.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/function_argument.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* --- literal evaluation --- */

static comptime_value_t _eval_literal_numeric(comptime_eval_t eval,
                                               checker_t ctx, node_t node) {
  cubec_literal_numeric_t num = (cubec_literal_numeric_t)node;
  const char *text = string_get(num->value);
  if (!text) return _eval_error_val(eval);

  semantic_type_t type = NULL;
  if (num->numeric_type != CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT) {
    switch (num->numeric_type) {
    case CUBEC_LITERAL_NUMERIC_TYPE_I8:  type = ctx->builtin_i8;  break;
    case CUBEC_LITERAL_NUMERIC_TYPE_I16: type = ctx->builtin_i16; break;
    case CUBEC_LITERAL_NUMERIC_TYPE_I32: type = ctx->builtin_i32; break;
    case CUBEC_LITERAL_NUMERIC_TYPE_I64: type = ctx->builtin_i64; break;
    case CUBEC_LITERAL_NUMERIC_TYPE_U8:  type = ctx->builtin_u8;  break;
    case CUBEC_LITERAL_NUMERIC_TYPE_U16: type = ctx->builtin_u16; break;
    case CUBEC_LITERAL_NUMERIC_TYPE_U32: type = ctx->builtin_u32; break;
    case CUBEC_LITERAL_NUMERIC_TYPE_U64: type = ctx->builtin_u64; break;
    case CUBEC_LITERAL_NUMERIC_TYPE_F16: type = ctx->builtin_f16; break;
    case CUBEC_LITERAL_NUMERIC_TYPE_F32: type = ctx->builtin_f32; break;
    case CUBEC_LITERAL_NUMERIC_TYPE_F64: type = ctx->builtin_f64; break;
    default: break;
    }
  }

  if (num->kind == CUBEC_LITERAL_NUMERIC_KIND_INTEGER) {
    bool is_signed = (num->numeric_type == CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT) ||
                     (num->numeric_type >= CUBEC_LITERAL_NUMERIC_TYPE_I8 &&
                      num->numeric_type <= CUBEC_LITERAL_NUMERIC_TYPE_I64);
    uint8_t width = 64;
    if (type) width = (uint8_t)(type->impl->size * 8);
    int64_t sval = 0;
    uint64_t uval = 0;
    if (is_signed) sval = strtoll(text, NULL, 10);
    else uval = strtoull(text, NULL, 10);
    if (!type) type = is_signed ? ctx->builtin_i64 : ctx->builtin_u64;
    return comptime_value_create_int(eval->allocator, sval, uval, width,
                                      is_signed, type);
  }

  double fval = strtod(text, NULL);
  uint8_t fwidth = 64;
  if (type) {
    switch (type->impl->kind) {
    case TYPE_F16: fwidth = 16; break;
    case TYPE_F32: fwidth = 32; break;
    case TYPE_F64: fwidth = 64; break;
    default: break;
    }
  }
  if (!type) type = ctx->builtin_f64;
  return comptime_value_create_float(eval->allocator, fval, fwidth, type);
}

static comptime_value_t _eval_literal_string(comptime_eval_t eval,
                                              checker_t ctx, node_t node) {
  cubec_literal_string_t s = (cubec_literal_string_t)node;
  return comptime_value_create_string(eval->allocator, string_get(s->value),
                                       ctx->builtin_string);
}

static comptime_value_t _eval_literal_char(comptime_eval_t eval,
                                            checker_t ctx, node_t node) {
  cubec_literal_char_t c = (cubec_literal_char_t)node;
  return comptime_value_create_char(eval->allocator, c->value,
                                     ctx->builtin_char);
}

static comptime_value_t _eval_literal_identifier(comptime_eval_t eval,
                                                  checker_t ctx, node_t node) {
  const char *name = _eval_ident_str(node);
  if (!name) return _eval_error_val(eval);

  comptime_value_t val = comptime_env_lookup(eval->current_env, name);
  if (val) return val;

  struct symbol *sym = scope_lookup(ctx->current_scope, name);
  if (!sym) return _eval_error_val(eval);

  if (sym->state == SYMBOL_NAME_KNOWN) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "cannot use imported value '%s' at compile time",
                         name);
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  if (sym->state == SYMBOL_TDZ) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "use of variable '%s' before initialization", name);
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  if (sym->kind == SYMBOL_ENUM_ITEM) {
    return comptime_value_create_int(eval->allocator, sym->enum_item.value,
                                      (uint64_t)sym->enum_item.value,
                                      32, true, sym->enum_item.owning_type);
  }

  return _eval_error_val(eval);
}

/* --- binary / unary evaluation (delegated to comptime_eval_binary.c) --- */

/* --- assignment evaluation --- */

static comptime_value_t _eval_assignment(comptime_eval_t eval, checker_t ctx,
                                          node_t node) {
  cubec_expression_assignment_t asgn = (cubec_expression_assignment_t)node;
  comptime_value_t rv = _comptime_eval_expr(eval, ctx, asgn->right);
  if (!rv || rv->kind == COMPTIME_VALUE_ERROR) return _eval_error_val(eval);

  if (asgn->left->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
    const char *name = _eval_ident_str(asgn->left);
    if (!comptime_env_update(eval->current_env, name, rv)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "undefined variable '%s' in comptime assignment",
                           name ? name : "<unknown>");
      ctx->error_count++;
      return _eval_error_val(eval);
    }
    return rv;
  }

  if (asgn->left->kind == CUBEC_NODE_EXPRESSION_MEMBER) {
    cubec_expression_member_t mem = (cubec_expression_member_t)asgn->left;
    comptime_value_t host = _comptime_eval_expr(eval, ctx, mem->host);
    if (!host || host->kind != COMPTIME_VALUE_COMPOSITE) return _eval_error_val(eval);
    const char *fname = _eval_ident_str((node_t)mem->field);
    if (!fname) return _eval_error_val(eval);
    for (size_t i = 0; i < host->composite.field_count; i++) {
      if (host->composite.field_names &&
          strcmp(host->composite.field_names[i], fname) == 0) {
        vec_set(host->composite.fields, i, rv);
        return rv;
      }
    }
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "no field '%s' in composite", fname);
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  if (asgn->left->kind == CUBEC_NODE_EXPRESSION_DEREF) {
    cubec_expression_binary_t deref = (cubec_expression_binary_t)asgn->left;
    comptime_value_t ptr = _comptime_eval_expr(eval, ctx, deref->right);
    if (!ptr || ptr->kind != COMPTIME_VALUE_POINTER) return _eval_error_val(eval);
    if (!comptime_alloc_write(eval->valloc, ptr->pointer.addr, rv)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "write to invalid/dangling pointer");
      ctx->error_count++;
      return _eval_error_val(eval);
    }
    return rv;
  }

  return _eval_error_val(eval);
}

/* --- call evaluation --- */

static comptime_value_t _eval_call(comptime_eval_t eval, checker_t ctx,
                                    node_t node) {
  cubec_expression_call_t call = (cubec_expression_call_t)node;
  comptime_value_t callee = _comptime_eval_expr(eval, ctx, call->callee);
  if (!callee || callee->kind != COMPTIME_VALUE_FUNCTION) return _eval_error_val(eval);

  if (eval->call_depth >= COMPTIME_MAX_CALL_STACK_DEPTH) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "comptime call stack overflow (max %d)",
                         COMPTIME_MAX_CALL_STACK_DEPTH);
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  size_t acount = call->arguments ? vec_get_size(call->arguments) : 0;
  comptime_value_t *args =
      (comptime_value_t *)allocator_alloc(eval->allocator,
                                           sizeof(comptime_value_t) * (acount + 1));
  for (size_t i = 0; i < acount; i++) {
    args[i] = _comptime_eval_expr(eval, ctx, (node_t)vec_get(call->arguments, i));
    if (!args[i] || args[i]->kind == COMPTIME_VALUE_ERROR) {
      allocator_free(eval->allocator, &args);
      return _eval_error_val(eval);
    }
  }

  comptime_env_t call_env =
      comptime_env_create(eval->allocator, callee->function.captured_env);

  if (callee->function.param_names) {
    size_t pcount = vec_get_size(callee->function.param_names);
    for (size_t i = 0; i < pcount && i < acount; i++) {
      const char *pname = (const char *)vec_get(callee->function.param_names, i);
      comptime_env_bind(call_env, pname, args[i]);
    }
  }
  allocator_free(eval->allocator, &args);

  comptime_env_t prev_env = eval->current_env;
  eval->current_env = call_env;
  eval->call_depth++;

  comptime_signal_t sig = _comptime_exec_block(eval, ctx,
                                                 callee->function.body);

  eval->call_depth--;
  eval->current_env = prev_env;
  comptime_env_dispose(call_env);

  if (sig.kind == COMPTIME_SIGNAL_ERROR) return _eval_error_val(eval);
  if (sig.kind == COMPTIME_SIGNAL_RETURN) return sig.return_value;
  return comptime_value_create_nil(eval->allocator, NULL);
}

/* --- member / namespace access --- */

static comptime_value_t _eval_member(comptime_eval_t eval, checker_t ctx,
                                      node_t node) {
  cubec_expression_member_t mem = (cubec_expression_member_t)node;
  comptime_value_t host = _comptime_eval_expr(eval, ctx, mem->host);
  if (!host) return _eval_error_val(eval);
  const char *fname = _eval_ident_str((node_t)mem->field);
  if (!fname) return _eval_error_val(eval);

  if (host->kind == COMPTIME_VALUE_COMPOSITE) {
    for (size_t i = 0; i < host->composite.field_count; i++) {
      if (host->composite.field_names &&
          strcmp(host->composite.field_names[i], fname) == 0)
        return (comptime_value_t)vec_get(host->composite.fields, i);
    }
    return _eval_error_val(eval);
  }

  if (host->kind == COMPTIME_VALUE_TYPE && host->type_val) {
    semantic_type_t t = host->type_val;
    if (t->static_fields) {
      size_t fc = vec_get_size(t->static_fields);
      for (size_t i = 0; i < fc; i++) {
        struct symbol *s = (struct symbol *)vec_get(t->static_fields, i);
        if (s && s->name && strcmp(s->name, fname) == 0 && s->kind == SYMBOL_VARIABLE)
          return comptime_env_lookup(eval->current_env, s->name);
      }
    }
  }

  if (host->kind == COMPTIME_VALUE_POINTER) {
    comptime_value_t pointed = comptime_alloc_read(eval->valloc,
                                                     host->pointer.addr);
    if (!pointed) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "dereference of dangling pointer");
      ctx->error_count++;
      return _eval_error_val(eval);
    }
    if (pointed->kind == COMPTIME_VALUE_COMPOSITE) {
      for (size_t i = 0; i < pointed->composite.field_count; i++) {
        if (pointed->composite.field_names &&
            strcmp(pointed->composite.field_names[i], fname) == 0)
          return (comptime_value_t)vec_get(pointed->composite.fields, i);
      }
    }
  }

  return _eval_error_val(eval);
}

static comptime_value_t _eval_namespace_access(comptime_eval_t eval,
                                                checker_t ctx, node_t node) {
  cubec_expression_namespace_access_t ns = (cubec_expression_namespace_access_t)node;
  comptime_value_t host = _comptime_eval_expr(eval, ctx, ns->host);
  if (!host || host->kind != COMPTIME_VALUE_TYPE) return _eval_error_val(eval);
  const char *fname = _eval_ident_str((node_t)ns->field);
  if (!fname) return _eval_error_val(eval);

  semantic_type_t t = host->type_val;
  if (t->associated_types) {
    size_t ac = vec_get_size(t->associated_types);
    for (size_t i = 0; i < ac; i++) {
      struct symbol *s = (struct symbol *)vec_get(t->associated_types, i);
      if (s && s->name && strcmp(s->name, fname) == 0 && s->kind == SYMBOL_TYPE)
        return comptime_value_create_type(eval->allocator, s->type.type);
    }
  }
  if (t->static_methods) {
    size_t mc = vec_get_size(t->static_methods);
    for (size_t i = 0; i < mc; i++) {
      struct symbol *s = (struct symbol *)vec_get(t->static_methods, i);
      if (s && s->name && strcmp(s->name, fname) == 0 && s->kind == SYMBOL_FUNCTION)
        return comptime_env_lookup(eval->current_env, s->name);
    }
  }
  if (t->static_fields) {
    size_t fc = vec_get_size(t->static_fields);
    for (size_t i = 0; i < fc; i++) {
      struct symbol *s = (struct symbol *)vec_get(t->static_fields, i);
      if (s && s->name && strcmp(s->name, fname) == 0 && s->kind == SYMBOL_VARIABLE)
        return comptime_env_lookup(eval->current_env, s->name);
    }
  }
  return _eval_error_val(eval);
}

/* --- other expression evaluations --- */

static comptime_value_t _eval_ternary(comptime_eval_t eval, checker_t ctx,
                                       node_t node) {
  cubec_expression_ternary_t tern = (cubec_expression_ternary_t)node;
  comptime_value_t cond = _comptime_eval_expr(eval, ctx, tern->condition);
  if (!cond || cond->kind == COMPTIME_VALUE_ERROR) return _eval_error_val(eval);
  if (comptime_value_is_truthy(cond))
    return _comptime_eval_expr(eval, ctx, tern->consequent);
  return _comptime_eval_expr(eval, ctx, tern->alternate);
}

static comptime_value_t _eval_group(comptime_eval_t eval, checker_t ctx,
                                     node_t node) {
  return _comptime_eval_expr(eval, ctx, ((cubec_expression_group_t)node)->inner);
}

static comptime_value_t _eval_typeof(comptime_eval_t eval, checker_t ctx,
                                      node_t node) {
  semantic_type_t type = resolver_resolve_type(ctx,
      ((cubec_expression_typeof_t)node)->expression);
  if (!type) return _eval_error_val(eval);
  return comptime_value_create_type(eval->allocator, type);
}

static comptime_value_t _eval_sizeof(comptime_eval_t eval, checker_t ctx,
                                      node_t node) {
  semantic_type_t type = resolver_resolve_type(ctx,
      ((cubec_expression_sizeof_t)node)->expression);
  if (!type) return _eval_error_val(eval);
  type_layout_compute(type, 8);
  return comptime_value_create_int(eval->allocator, (int64_t)type->impl->size,
                                    type->impl->size, 64, false, ctx->builtin_u64);
}

static comptime_value_t _eval_alignof(comptime_eval_t eval, checker_t ctx,
                                       node_t node) {
  semantic_type_t type = resolver_resolve_type(ctx,
      ((cubec_expression_alignof_t)node)->expression);
  if (!type) return _eval_error_val(eval);
  type_layout_compute(type, 8);
  return comptime_value_create_int(eval->allocator, (int64_t)type->impl->alignment,
                                    type->impl->alignment, 64, false, ctx->builtin_u64);
}

static comptime_value_t _eval_function_expr(comptime_eval_t eval,
                                             checker_t ctx, node_t node) {
  cubec_expression_function_t fn = (cubec_expression_function_t)node;
  vec_t param_names = NULL;
  if (fn->arguments) {
    vec_init_t vi = {.auto_dispose = false};
    param_names = (vec_t)allocator_create(eval->allocator, &g_vec_type, &vi);
    size_t ac = vec_get_size(fn->arguments);
    for (size_t i = 0; i < ac; i++) {
      node_t arg = (node_t)vec_get(fn->arguments, i);
      if (arg->kind == CUBEC_NODE_FUNCTION_ARGUMENT) {
        const char *name = _checker_ident_str(
            ((cubec_function_argument_t)arg)->identifier);
        if (name) vec_push(param_names, (void *)name);
      }
    }
  }

  comptime_env_t captured = eval->current_env;
  semantic_type_t ftype = NULL;
  struct symbol *sym = fn->name
      ? scope_lookup(ctx->current_scope, _eval_ident_str(fn->name))
      : NULL;
  if (sym && sym->kind == SYMBOL_FUNCTION) ftype = sym->function.type;

  return comptime_value_create_function(eval->allocator, captured, fn->body,
                                         param_names, ftype);
}

static comptime_value_t _eval_init_list(comptime_eval_t eval, checker_t ctx,
                                         node_t node) {
  cubec_expression_initialize_list_t il = (cubec_expression_initialize_list_t)node;
  semantic_type_t type = il->type ? resolver_resolve_type(ctx, il->type) : NULL;
  if (!type) return _eval_error_val(eval);

  vec_t fields = NULL;
  vec_init_t vi = {.auto_dispose = true};
  fields = (vec_t)allocator_create(eval->allocator, &g_vec_type, &vi);

  const char **field_names = NULL;
  size_t field_count = 0;

  if (type->impl->kind == TYPE_STRUCT) {
    vec_t type_fields = type->impl->struct_type.fields;
    field_count = type_fields ? vec_get_size(type_fields) : 0;

    if (il->is_field && il->items) {
      field_names = (const char **)allocator_alloc(
          eval->allocator, sizeof(const char *) * field_count);
      for (size_t i = 0; i < field_count; i++) {
        struct symbol *fsym = (struct symbol *)vec_get(type_fields, i);
        field_names[i] = fsym ? fsym->name : NULL;
        comptime_value_t nil_v =
            comptime_value_create_nil(eval->allocator,
                                       fsym ? fsym->field.type : NULL);
        vec_push(fields, nil_v);
      }
      size_t ic = vec_get_size(il->items);
      for (size_t i = 0; i < ic; i++) {
        node_t item = (node_t)vec_get(il->items, i);
        if (item->kind != CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD) continue;
        cubec_expression_initialize_field_t f =
            (cubec_expression_initialize_field_t)item;
        const char *fname = _eval_ident_str((node_t)f->field);
        for (size_t j = 0; j < field_count; j++) {
          if (field_names[j] && strcmp(field_names[j], fname) == 0) {
            comptime_value_t v = _comptime_eval_expr(eval, ctx, f->value);
            vec_set(fields, j, v);
            break;
          }
        }
      }
    } else if (il->items) {
      size_t ic = vec_get_size(il->items);
      field_names = (const char **)allocator_alloc(
          eval->allocator, sizeof(const char *) * field_count);
      for (size_t i = 0; i < field_count; i++) {
        struct symbol *fsym = (struct symbol *)vec_get(type_fields, i);
        field_names[i] = fsym ? fsym->name : NULL;
        if (i < ic) {
          comptime_value_t v = _comptime_eval_expr(eval, ctx,
                                                     (node_t)vec_get(il->items, i));
          vec_push(fields, v);
        } else {
          vec_push(fields,
                   comptime_value_create_nil(eval->allocator,
                                              fsym ? fsym->field.type : NULL));
        }
      }
    }
  } else if (type->impl->kind == TYPE_ARRAY && il->items) {
    size_t ic = vec_get_size(il->items);
    field_count = ic;
    for (size_t i = 0; i < ic; i++) {
      comptime_value_t v = _comptime_eval_expr(eval, ctx,
                                                 (node_t)vec_get(il->items, i));
      vec_push(fields, v);
    }
  }

  return comptime_value_create_composite(eval->allocator, type, fields,
                                          field_names, field_count);
}

static comptime_value_t _eval_comma(comptime_eval_t eval, checker_t ctx,
                                     node_t node) {
  cubec_expression_comma_t c = (cubec_expression_comma_t)node;
  _comptime_eval_expr(eval, ctx, c->left);
  return _comptime_eval_expr(eval, ctx, c->right);
}

static comptime_value_t _eval_deref(comptime_eval_t eval, checker_t ctx,
                                     node_t node) {
  cubec_expression_binary_t deref = (cubec_expression_binary_t)node;
  comptime_value_t val = _comptime_eval_expr(eval, ctx, deref->right);
  if (!val || val->kind != COMPTIME_VALUE_POINTER) return _eval_error_val(eval);
  comptime_value_t pointed = comptime_alloc_read(eval->valloc, val->pointer.addr);
  if (!pointed) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "dereference of dangling pointer");
    ctx->error_count++;
    return _eval_error_val(eval);
  }
  return pointed;
}

static comptime_value_t _eval_addr(comptime_eval_t eval, checker_t ctx,
                                    node_t node) {
  cubec_expression_binary_t addr = (cubec_expression_binary_t)node;
  comptime_value_t val = _comptime_eval_expr(eval, ctx, addr->right);
  if (!val) return _eval_error_val(eval);

  if (addr->right->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
    const char *name = _eval_ident_str(addr->right);
    comptime_value_t existing = comptime_env_lookup(eval->current_env, name);
    if (existing) {
      uint64_t a = comptime_alloc_allocate(eval->valloc,
                                            comptime_value_clone(eval->allocator, existing),
                                            eval->valloc->scope_depth);
      semantic_type_t ptr_type = existing->type
          ? semantic_type_create_pointer(eval->allocator, existing->type)
          : NULL;
      return comptime_value_create_pointer(eval->allocator, a, ptr_type);
    }
  }

  uint64_t a = comptime_alloc_allocate(eval->valloc,
                                        comptime_value_clone(eval->allocator, val),
                                        eval->valloc->scope_depth);
  semantic_type_t ptr_type = val->type
      ? semantic_type_create_pointer(eval->allocator, val->type)
      : NULL;
  return comptime_value_create_pointer(eval->allocator, a, ptr_type);
}

static comptime_value_t _eval_slice(comptime_eval_t eval, checker_t ctx,
                                     node_t node) {
  /* TODO: implement with array support */
  (void)eval; (void)ctx; (void)node;
  return _eval_error_val(eval);
}

static comptime_value_t _eval_generic_inst(comptime_eval_t eval, checker_t ctx,
                                            node_t node) {
  cubec_expression_generic_instantiation_t gi =
      (cubec_expression_generic_instantiation_t)node;
  const char *name = _eval_ident_str(gi->callee);
  if (name) {
    struct symbol *sym = scope_lookup(ctx->current_scope, name);
    if (sym && sym->kind == SYMBOL_TYPE && sym->type.type)
      return comptime_value_create_type(eval->allocator, sym->type.type);
  }
  return _eval_error_val(eval);
}

/* --- main expression dispatcher --- */

comptime_value_t _comptime_eval_expr(comptime_eval_t eval, checker_t ctx,
                                      node_t expr) {
  if (!expr) return _eval_error_val(eval);
  switch (expr->kind) {
  case CUBEC_NODE_LITERAL_NUMERIC:
    return _eval_literal_numeric(eval, ctx, expr);
  case CUBEC_NODE_LITERAL_STRING:
    return _eval_literal_string(eval, ctx, expr);
  case CUBEC_NODE_LITERAL_CHAR:
    return _eval_literal_char(eval, ctx, expr);
  case CUBEC_NODE_LITERAL_IDENTIFIER:
    return _eval_literal_identifier(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_BINARY:
    return _comptime_eval_binary(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT:
    return _eval_assignment(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_CALL:
    return _eval_call(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_TERNARY:
    return _eval_ternary(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_GROUP:
    return _eval_group(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_MEMBER:
    return _eval_member(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS:
    return _eval_namespace_access(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_TYPEOF:
    return _eval_typeof(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_SIZEOF:
    return _eval_sizeof(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_ALIGNOF:
    return _eval_alignof(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_FUNCTION:
    return _eval_function_expr(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST:
    return _eval_init_list(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_COMMA:
    return _eval_comma(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_DEREF:
    return _eval_deref(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_ADDR:
    return _eval_addr(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_SLICE:
    return _eval_slice(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION:
    return _eval_generic_inst(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_TYPE_STRUCT:
  case CUBEC_NODE_EXPRESSION_TYPE_ENUM:
  case CUBEC_NODE_EXPRESSION_TYPE_UNION:
  case CUBEC_NODE_EXPRESSION_TYPE_FUNCTION:
  case CUBEC_NODE_EXPRESSION_TYPE_INTERFACE:
  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER:
  case CUBEC_NODE_DECLARATION_POINTER:
  case CUBEC_NODE_DECLARATION_ARRAY:
  case CUBEC_NODE_DECLARATION_SLICE: {
    semantic_type_t type = resolver_resolve_type(ctx, expr);
    if (!type) return _eval_error_val(eval);
    return comptime_value_create_type(eval->allocator, type);
  }
  default:
    return _eval_error_val(eval);
  }
}

comptime_value_t comptime_eval_expr(comptime_eval_t eval, checker_t ctx,
                                     node_t expr) {
  return _comptime_eval_expr(eval, ctx, expr);
}
