#include "engine/comptime_eval_internal.h"
#include "engine/comptime_eval_binary.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_layout.h"
#include "engine/type_hash.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/strmap.h"
#include "core/vec.h"
#include <limits.h>
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
#include "cubec/expression_spread.h"
#include "cubec/function_capture.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_initialize_field.h"
#include "cubec/expression_comma.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/function_argument.h"
#include "cubec/statement_function.h"
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
    /* Default integer literal is i32 (signed) or u32 (unsigned) */
    uint8_t width = 32;
    if (type) width = (uint8_t)(type->impl->size * 8);
    if (!type) type = is_signed ? ctx->builtin_i32 : ctx->builtin_u32;

    int64_t sval = 0;
    uint64_t uval = 0;
    if (is_signed) {
      sval = strtoll(text, NULL, 10);
      uval = (uint64_t)sval;
      /* Check overflow: value must fit in the target width */
      if (width == 8 && (sval < INT8_MIN || sval > INT8_MAX)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "integer literal '%s' overflows i8", text);
        ctx->error_count++;
        return _eval_error_val(eval);
      }
      if (width == 16 && (sval < INT16_MIN || sval > INT16_MAX)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "integer literal '%s' overflows i16", text);
        ctx->error_count++;
        return _eval_error_val(eval);
      }
      if (width == 32 && (sval < INT32_MIN || sval > INT32_MAX)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "integer literal '%s' overflows i32", text);
        ctx->error_count++;
        return _eval_error_val(eval);
      }
    } else {
      uval = strtoull(text, NULL, 10);
      sval = (int64_t)uval;
      if (width == 8 && uval > UINT8_MAX) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "integer literal '%s' overflows u8", text);
        ctx->error_count++;
        return _eval_error_val(eval);
      }
      if (width == 16 && uval > UINT16_MAX) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "integer literal '%s' overflows u16", text);
        ctx->error_count++;
        return _eval_error_val(eval);
      }
      if (width == 32 && uval > UINT32_MAX) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "integer literal '%s' overflows u32", text);
        ctx->error_count++;
        return _eval_error_val(eval);
      }
    }
    return _eval_temp(eval, comptime_value_create_int(eval->allocator, sval, uval, width,
                                                       is_signed, type));
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
  return _eval_temp(eval, comptime_value_create_float(eval->allocator, fval, fwidth, type));
}

static comptime_value_t _eval_literal_string(comptime_eval_t eval,
                                              checker_t ctx, node_t node) {
  cubec_literal_string_t s = (cubec_literal_string_t)node;
  return _eval_temp(eval, comptime_value_create_string(eval->allocator, string_get(s->value),
                                                         ctx->builtin_string));
}

static comptime_value_t _eval_literal_char(comptime_eval_t eval,
                                            checker_t ctx, node_t node) {
  cubec_literal_char_t c = (cubec_literal_char_t)node;
  return _eval_temp(eval, comptime_value_create_char(eval->allocator, c->value,
                                                       ctx->builtin_char));
}

static comptime_value_t _eval_literal_identifier(comptime_eval_t eval,
                                                  checker_t ctx, node_t node) {
  const char *name = _eval_ident_str(node);
  if (!name) return _eval_error_val(eval);

  /* Borrow from scope chain — no clone */
  comptime_value_t val = comptime_env_lookup_value(eval->current_env, eval->valloc, name);
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
    return _eval_temp(eval, comptime_value_create_int(eval->allocator, sym->enum_item.value,
                                                        (uint64_t)sym->enum_item.value,
                                                        32, true, sym->enum_item.owning_type));
  }

  if (sym->kind == SYMBOL_GENERIC_PARAM) {
    if (sym->generic_param.value_type) {
      /* Value generic param: look up in comptime environment */
      comptime_value_t val = comptime_env_lookup_value(eval->current_env, eval->valloc, name);
      if (val && val->kind != COMPTIME_VALUE_ERROR) return val;
    }
    return _eval_error_val(eval);
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

  /* identifier assignment: clone rv into env, return borrowed from env */
  if (asgn->left->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
    const char *name = _eval_ident_str(asgn->left);

    /* Check const: variable must be mutable */
    {
      struct symbol *sym = scope_lookup(ctx->current_scope, name);
      if (sym && sym->kind == SYMBOL_VARIABLE && !sym->variable.is_mutable) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "cannot assign to const variable '%s'", name);
        ctx->error_count++;
        return _eval_error_val(eval);
      }
    }

    comptime_value_t cloned = comptime_value_clone(eval->allocator, rv);
    if (!comptime_env_update_value(eval->current_env, eval->valloc, name, cloned)) {
      allocator_free(eval->allocator, &cloned);
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "undefined variable '%s' in comptime assignment",
                           name ? name : "<unknown>");
      ctx->error_count++;
      return _eval_error_val(eval);
    }
    return comptime_env_lookup_value(eval->current_env, eval->valloc, name);  /* borrowed from env */
  }

  /* member assignment: obj.field = rv */
  if (asgn->left->kind == CUBEC_NODE_EXPRESSION_MEMBER) {
    cubec_expression_member_t mem = (cubec_expression_member_t)asgn->left;
    const char *fname = _eval_ident_str((node_t)mem->field);
    if (!fname) return _eval_error_val(eval);

    comptime_value_t host = NULL;
    if (mem->host->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
      /* Borrow host directly from env */
      const char *host_name = _eval_ident_str(mem->host);
      host = comptime_env_lookup_value(eval->current_env, eval->valloc, host_name);
    } else {
      host = _comptime_eval_expr(eval, ctx, mem->host);
    }
    if (!host) return _eval_error_val(eval);

    /* Check const: if host type is const, field is not writable */
    if (host->type && semantic_type_is_const(host->type)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "cannot assign to field of const-qualified expression");
      ctx->error_count++;
      return _eval_error_val(eval);
    }

    comptime_value_t target = host;
    /* If host is a pointer, dereference to get the composite */
    if (host->kind == COMPTIME_VALUE_POINTER) {
      /* Check const: if pointee is const, cannot write through pointer */
      if (host->type && host->type->impl->kind == TYPE_POINTER) {
        semantic_type_t pointee = host->type->impl->pointer.pointee;
        if (semantic_type_is_const(pointee)) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                               "cannot write through pointer to const type");
          ctx->error_count++;
          return _eval_error_val(eval);
        }
      }
      target = comptime_alloc_read(eval->valloc, host->pointer.addr);
      if (!target) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "dereference of dangling pointer");
        ctx->error_count++;
        return _eval_error_val(eval);
      }
    }

    if (target->kind == COMPTIME_VALUE_COMPOSITE) {
      if (!comptime_value_set_field(target, fname, rv)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "no field '%s' in composite", fname);
        ctx->error_count++;
        return _eval_error_val(eval);
      }
      return _eval_temp(eval, comptime_value_get_field(target, fname, eval->allocator));
    }

    return _eval_error_val(eval);
  }

  /* deref assignment: *ptr = rv */
  if (asgn->left->kind == CUBEC_NODE_EXPRESSION_DEREF) {
    cubec_expression_binary_t deref = (cubec_expression_binary_t)asgn->left;
    comptime_value_t ptr = _comptime_eval_expr(eval, ctx, deref->right);
    if (!ptr || ptr->kind != COMPTIME_VALUE_POINTER) return _eval_error_val(eval);

    /* Check const: if pointee is const, cannot write through pointer */
    if (ptr->type && ptr->type->impl->kind == TYPE_POINTER) {
      semantic_type_t pointee = ptr->type->impl->pointer.pointee;
      if (semantic_type_is_const(pointee)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "cannot write through pointer to const type");
        ctx->error_count++;
        return _eval_error_val(eval);
      }
    }

    comptime_value_t cloned = comptime_value_clone(eval->allocator, rv);
    if (!comptime_alloc_write(eval->valloc, ptr->pointer.addr, cloned)) {
      allocator_free(eval->allocator, &cloned);
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "write to invalid/dangling pointer");
      ctx->error_count++;
      return _eval_error_val(eval);
    }
    return comptime_alloc_read(eval->valloc, ptr->pointer.addr);  /* borrowed from alloc */
  }

  /* slice assignment: arr[i] = rv */
  if (asgn->left->kind == CUBEC_NODE_EXPRESSION_SLICE) {
    cubec_expression_slice_t sl = (cubec_expression_slice_t)asgn->left;
    comptime_value_t host = _comptime_eval_expr(eval, ctx, sl->host);
    if (!host || host->kind == COMPTIME_VALUE_ERROR) return _eval_error_val(eval);

    /* Resolve index */
    size_t index = 0;
    if (sl->start) {
      comptime_value_t iv = _comptime_eval_expr(eval, ctx, sl->start);
      if (!iv || iv->kind == COMPTIME_VALUE_ERROR) return _eval_error_val(eval);
      index = (size_t)comptime_value_as_u64(iv);
    }

    /* Check for __set__ magic method on host type */
    if (host->type && host->type->instance_methods) {
      size_t mc = vec_get_size(host->type->instance_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *s = (struct symbol *)vec_get(host->type->instance_methods, i);
        if (s && s->name && strcmp(s->name, "__set__") == 0 && s->kind == SYMBOL_FUNCTION) {
          /* Call __set__(host, index, rv) — TODO: implement magic method dispatch */
          /* For now, fall through to direct array access */
          break;
        }
      }
    }

    /* Direct array access (no __set__) */
    if (host->kind == COMPTIME_VALUE_COMPOSITE && host->composite.element_type) {
      if (!comptime_value_set_index(host, index, rv)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "array index %zu out of bounds", index);
        ctx->error_count++;
        return _eval_error_val(eval);
      }
      return _eval_temp(eval, comptime_value_get_index(host, index, eval->allocator));
    }

    return _eval_error_val(eval);
  }

  return _eval_error_val(eval);
}

/* --- call evaluation --- */

/* Helper: invoke a comptime function value with given arguments */
comptime_value_t _eval_call_function(comptime_eval_t eval, checker_t ctx,
                                             comptime_value_t callee,
                                             comptime_value_t *args, size_t acount,
                                             node_t call_node) {
  if (eval->call_depth >= COMPTIME_MAX_CALL_STACK_DEPTH) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, call_node->location,
                         "comptime call stack overflow (max %d)",
                         COMPTIME_MAX_CALL_STACK_DEPTH);
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  comptime_env_t call_env =
      comptime_env_create(eval->allocator, callee->function.captured_env);

  if (callee->function.param_names) {
    size_t pcount = vec_get_size(callee->function.param_names);
    /* Determine if the last parameter is a rest parameter by checking the type */
    bool last_is_rest = false;
    if (pcount > 0 && callee->type && callee->type->impl->kind == TYPE_FUNCTION) {
      vec_t fparams = callee->type->impl->function.params;
      size_t fpcount = fparams ? vec_get_size(fparams) : 0;
      if (fpcount > 0) {
        semantic_type_t last_pt = (semantic_type_t)vec_get(fparams, fpcount - 1);
        if (last_pt && last_pt->impl->kind == TYPE_GENERIC_PACK)
          last_is_rest = true;
      }
    }

    for (size_t i = 0; i < pcount; i++) {
      const char *pname = (const char *)vec_get(callee->function.param_names, i);
      if (i == pcount - 1 && last_is_rest) {
        /* Rest parameter: collect remaining args into a pack */
        vec_init_t pvi = {.auto_dispose = true};
        vec_t pack_elems = (vec_t)allocator_create(eval->allocator, &g_vec_type, &pvi);
        for (size_t j = i; j < acount; j++) {
          vec_push(pack_elems, comptime_value_clone(eval->allocator, args[j]));
        }
        semantic_type_t pack_type = NULL;
        if (callee->type && callee->type->impl->kind == TYPE_FUNCTION) {
          vec_t fparams = callee->type->impl->function.params;
          size_t fpcount = fparams ? vec_get_size(fparams) : 0;
          if (fpcount > 0) pack_type = (semantic_type_t)vec_get(fparams, fpcount - 1);
        }
        comptime_value_t pack_val = comptime_value_create_pack(
            eval->allocator, pack_elems, pack_type);
        comptime_env_bind_value(call_env, eval->valloc, pname, pack_val);
      } else if (i < acount) {
        /* args are already cloned by caller — ownership transfers to call_env */
        comptime_env_bind_value(call_env, eval->valloc, pname, args[i]);
      }
    }
  }

  comptime_env_t prev_env = eval->current_env;
  eval->current_env = call_env;
  eval->call_depth++;

  comptime_signal_t sig = _comptime_exec_block(eval, ctx, callee->function.body);

  eval->call_depth--;
  eval->current_env = prev_env;

  /* sig.return_value was cloned into call_env's temporaries by _comptime_exec_block.
     We must clone it again into the caller's env before disposing call_env. */
  if (sig.kind == COMPTIME_SIGNAL_RETURN && sig.return_value) {
    comptime_value_t cloned = comptime_value_clone(eval->allocator, sig.return_value);
    comptime_env_track_temp(eval->current_env, cloned);
    sig.return_value = cloned;
  }

  comptime_env_dispose(call_env);

  if (sig.kind == COMPTIME_SIGNAL_ERROR) return _eval_error_val(eval);
  if (sig.kind == COMPTIME_SIGNAL_RETURN) return sig.return_value;  /* cloned into caller env */
  return _eval_temp(eval, comptime_value_create_nil(eval->allocator, NULL));
}

/* --- method value creation from symbol --- */

comptime_value_t _comptime_create_method_value(comptime_eval_t eval,
                                                checker_t ctx,
                                                struct symbol *method_sym) {
  if (!method_sym || method_sym->kind != SYMBOL_FUNCTION || !method_sym->function.ast_node)
    return NULL;

  cubec_statement_function_t mfn =
      (cubec_statement_function_t)method_sym->function.ast_node;

  if (!mfn->body) return NULL;

  /* Extract param_names from AST arguments */
  vec_t param_names = NULL;
  if (mfn->arguments) {
    vec_init_t pvi = {.auto_dispose = false};
    param_names = (vec_t)allocator_create(eval->allocator, &g_vec_type, &pvi);
    size_t acount = vec_get_size(mfn->arguments);
    for (size_t i = 0; i < acount; i++) {
      node_t arg = (node_t)vec_get(mfn->arguments, i);
      if (arg->kind == CUBEC_NODE_FUNCTION_ARGUMENT) {
        cubec_function_argument_t farg = (cubec_function_argument_t)arg;
        const char *pname = _eval_ident_str((node_t)farg->identifier);
        if (pname) vec_push(param_names, (void *)pname);
      }
    }
  }

  comptime_value_t fn_val = comptime_value_create_function(
      eval->allocator,
      eval->current_env,  /* captured_env = env where the type was declared */
      mfn->body,
      param_names,
      method_sym->function.type);

  return fn_val;
}

static comptime_value_t _eval_call(comptime_eval_t eval, checker_t ctx,
                                    node_t node) {
  cubec_expression_call_t call = (cubec_expression_call_t)node;
  /* --- builtin dispatch --- */
  {
    const char *callee_name = NULL;
    struct symbol *callee_sym = NULL;
    node_t callee_for_dispatch = call->callee;

    /* Unwrap generic_instantiation: getTupleItem[0](t) → callee = getTupleItem */
    if (callee_for_dispatch && callee_for_dispatch->kind == CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION) {
      cubec_expression_generic_instantiation_t gi =
          (cubec_expression_generic_instantiation_t)callee_for_dispatch;
      callee_name = _eval_ident_str(gi->callee);
      callee_sym = callee_name ? scope_lookup(ctx->current_scope, callee_name) : NULL;
    } else if (callee_for_dispatch && callee_for_dispatch->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
      callee_name = _eval_ident_str(callee_for_dispatch);
      callee_sym = callee_name ? scope_lookup(ctx->current_scope, callee_name) : NULL;
    }

    if (callee_sym && callee_sym->is_builtin) {
      builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, callee_name);
      if (be && be->eval_call) {
        return be->eval_call(eval, ctx, node, be);
      }
    }
  }

  /* --- member call desugaring --- */
  /* a.method(args) → typeof(a)::method(&a, args) for objects,
                       typeof(a)::method(a, args) for pointers */
  if (call->callee->kind == CUBEC_NODE_EXPRESSION_MEMBER) {
    cubec_expression_member_t mem = (cubec_expression_member_t)call->callee;
    comptime_value_t host = _comptime_eval_expr(eval, ctx, mem->host);
    if (!host || host->kind == COMPTIME_VALUE_ERROR) return _eval_error_val(eval);
    const char *fname = _eval_ident_str((node_t)mem->field);

    /* Determine receiver type (auto-deref pointers for method lookup) */
    semantic_type_t receiver_type = host->type;
    bool host_is_pointer = (host->kind == COMPTIME_VALUE_POINTER);
    if (host_is_pointer && host->type && host->type->impl->kind == TYPE_POINTER)
      receiver_type = host->type->impl->pointer.pointee;

    if (fname && receiver_type &&
        (receiver_type->impl->kind == TYPE_STRUCT ||
         receiver_type->impl->kind == TYPE_UNION ||
         receiver_type->impl->kind == TYPE_CUNION) &&
        receiver_type->instance_methods) {
      size_t mc = vec_get_size(receiver_type->instance_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *m = (struct symbol *)vec_get(receiver_type->instance_methods, i);
        if (m && m->name && strcmp(m->name, fname) == 0 && m->kind == SYMBOL_FUNCTION) {
          comptime_value_t method_val = _comptime_create_method_value(eval, ctx, m);
          if (!method_val) return _eval_error_val(eval);
          comptime_env_track_temp(eval->current_env, method_val);

          /* Compute self argument: &host for objects, host directly for pointers */
          comptime_value_t self_val;
          if (host_is_pointer) {
            /* Pointer host: pass the pointer value directly */
            self_val = comptime_value_clone(eval->allocator, host);
          } else if (mem->host->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
            /* Object host is an identifier: take address directly from env
               (no clone - pointer references the same alloc slot) */
            const char *host_name = _eval_ident_str(mem->host);
            uint64_t addr = comptime_env_lookup_addr(eval->current_env, host_name);
            if (!addr) return _eval_error_val(eval);
            semantic_type_t ptr_type = host->type
                ? semantic_type_create_pointer(eval->allocator, host->type)
                : NULL;
            if (ptr_type) vec_push(ctx->all_types, ptr_type);
            self_val = comptime_value_create_pointer(eval->allocator, addr, ptr_type);
          } else {
            /* Object host is a complex expression: allocate a copy */
            uint64_t addr = comptime_alloc_allocate(eval->valloc,
                                                     comptime_value_clone(eval->allocator, host),
                                                     eval->valloc->scope_depth);
            semantic_type_t ptr_type = host->type
                ? semantic_type_create_pointer(eval->allocator, host->type)
                : NULL;
            if (ptr_type) vec_push(ctx->all_types, ptr_type);
            self_val = comptime_value_create_pointer(eval->allocator, addr, ptr_type);
          }

          /* Build args: [self, user_arg0, user_arg1, ...] */
          size_t acount = call->arguments ? vec_get_size(call->arguments) : 0;

          /* Check if any argument is a spread */
          bool has_spread = false;
          for (size_t j = 0; j < acount; j++) {
            node_t arg_node = (node_t)vec_get(call->arguments, j);
            if (arg_node && arg_node->kind == CUBEC_NODE_EXPRESSION_SPREAD) {
              has_spread = true;
              break;
            }
          }

          if (has_spread) {
            vec_init_t margvi = {.auto_dispose = false};
            vec_t marg_vec = (vec_t)allocator_create(eval->allocator, &g_vec_type, &margvi);
            vec_push(marg_vec, self_val);
            for (size_t j = 0; j < acount; j++) {
              node_t arg_node = (node_t)vec_get(call->arguments, j);
              if (arg_node && arg_node->kind == CUBEC_NODE_EXPRESSION_SPREAD) {
                cubec_expression_spread_t spread = (cubec_expression_spread_t)arg_node;
                comptime_value_t pack_val = _comptime_eval_expr(eval, ctx, spread->value);
                if (!pack_val || pack_val->kind == COMPTIME_VALUE_ERROR) {
                  allocator_free(eval->allocator, &marg_vec);
                  return _eval_error_val(eval);
                }
                if (pack_val->kind == COMPTIME_VALUE_PACK && pack_val->pack.elements) {
                  size_t ecount = vec_get_size(pack_val->pack.elements);
                  for (size_t k = 0; k < ecount; k++) {
                    comptime_value_t elem = (comptime_value_t)vec_get(pack_val->pack.elements, k);
                    vec_push(marg_vec, comptime_value_clone(eval->allocator, elem));
                  }
                }
              } else {
                comptime_value_t arg = _comptime_eval_expr(eval, ctx, arg_node);
                if (!arg || arg->kind == COMPTIME_VALUE_ERROR) {
                  allocator_free(eval->allocator, &marg_vec);
                  return _eval_error_val(eval);
                }
                vec_push(marg_vec, comptime_value_clone(eval->allocator, arg));
              }
            }
            size_t total_margs = vec_get_size(marg_vec);
            comptime_value_t *args =
                (comptime_value_t *)allocator_alloc(eval->allocator,
                                                     sizeof(comptime_value_t) * (total_margs + 1));
            for (size_t j = 0; j < total_margs; j++)
              args[j] = (comptime_value_t)vec_get(marg_vec, j);
            comptime_value_t result = _eval_call_function(eval, ctx, method_val, args, total_margs, node);
            allocator_free(eval->allocator, &args);
            allocator_free(eval->allocator, &marg_vec);
            return result;
          }

          /* No spread — original fast path */
          comptime_value_t *args =
              (comptime_value_t *)allocator_alloc(eval->allocator,
                                                   sizeof(comptime_value_t) * (acount + 1));
          args[0] = self_val;
          for (size_t j = 0; j < acount; j++) {
            comptime_value_t arg = _comptime_eval_expr(eval, ctx,
                                                         (node_t)vec_get(call->arguments, j));
            if (!arg || arg->kind == COMPTIME_VALUE_ERROR) {
              allocator_free(eval->allocator, &args);
              return _eval_error_val(eval);
            }
            args[j + 1] = comptime_value_clone(eval->allocator, arg);
          }
          comptime_value_t result = _eval_call_function(eval, ctx, method_val, args, acount + 1, node);
          allocator_free(eval->allocator, &args);
          return result;
        }
      }
    }
    /* Not an instance method — fall through to normal dispatch */
  }

  /* --- normal call dispatch --- */
  comptime_value_t callee = _comptime_eval_expr(eval, ctx, call->callee);
  if (!callee) return _eval_error_val(eval);

  /* Direct function call */
  if (callee->kind == COMPTIME_VALUE_FUNCTION) {
    size_t acount = call->arguments ? vec_get_size(call->arguments) : 0;

    /* Check if any argument is a spread — if so, use dynamic vec */
    bool has_spread = false;
    for (size_t i = 0; i < acount; i++) {
      node_t arg_node = (node_t)vec_get(call->arguments, i);
      if (arg_node && arg_node->kind == CUBEC_NODE_EXPRESSION_SPREAD) {
        has_spread = true;
        break;
      }
    }

    if (has_spread) {
      /* Build argument list with spread expansion */
      vec_init_t argvi = {.auto_dispose = false};
      vec_t arg_vec = (vec_t)allocator_create(eval->allocator, &g_vec_type, &argvi);
      for (size_t i = 0; i < acount; i++) {
        node_t arg_node = (node_t)vec_get(call->arguments, i);
        if (arg_node && arg_node->kind == CUBEC_NODE_EXPRESSION_SPREAD) {
          cubec_expression_spread_t spread = (cubec_expression_spread_t)arg_node;
          comptime_value_t pack_val = _comptime_eval_expr(eval, ctx, spread->value);
          if (!pack_val || pack_val->kind == COMPTIME_VALUE_ERROR) {
            allocator_free(eval->allocator, &arg_vec);
            return _eval_error_val(eval);
          }
          if (pack_val->kind == COMPTIME_VALUE_PACK && pack_val->pack.elements) {
            size_t ecount = vec_get_size(pack_val->pack.elements);
            for (size_t j = 0; j < ecount; j++) {
              comptime_value_t elem = (comptime_value_t)vec_get(pack_val->pack.elements, j);
              vec_push(arg_vec, comptime_value_clone(eval->allocator, elem));
            }
          }
        } else {
          comptime_value_t arg = _comptime_eval_expr(eval, ctx, arg_node);
          if (!arg || arg->kind == COMPTIME_VALUE_ERROR) {
            allocator_free(eval->allocator, &arg_vec);
            return _eval_error_val(eval);
          }
          vec_push(arg_vec, comptime_value_clone(eval->allocator, arg));
        }
      }
      size_t total_args = vec_get_size(arg_vec);
      comptime_value_t *args =
          (comptime_value_t *)allocator_alloc(eval->allocator,
                                               sizeof(comptime_value_t) * (total_args + 1));
      for (size_t i = 0; i < total_args; i++)
        args[i] = (comptime_value_t)vec_get(arg_vec, i);
      comptime_value_t result = _eval_call_function(eval, ctx, callee, args, total_args, node);
      allocator_free(eval->allocator, &args);
      allocator_free(eval->allocator, &arg_vec);
      return result;
    }

    /* No spread — original fast path */
    comptime_value_t *args =
        (comptime_value_t *)allocator_alloc(eval->allocator,
                                             sizeof(comptime_value_t) * (acount + 1));
    for (size_t i = 0; i < acount; i++) {
      comptime_value_t arg = _comptime_eval_expr(eval, ctx,
                                                   (node_t)vec_get(call->arguments, i));
      if (!arg || arg->kind == COMPTIME_VALUE_ERROR) {
        allocator_free(eval->allocator, &args);
        return _eval_error_val(eval);
      }
      args[i] = comptime_value_clone(eval->allocator, arg);
    }
    comptime_value_t result = _eval_call_function(eval, ctx, callee, args, acount, node);
    allocator_free(eval->allocator, &args);
    return result;
  }

  /* __call__ magic method dispatch */
  if (callee->type && callee->type->instance_methods) {
    size_t mc = vec_get_size(callee->type->instance_methods);
    for (size_t i = 0; i < mc; i++) {
      struct symbol *s = (struct symbol *)vec_get(callee->type->instance_methods, i);
      if (s && s->name && strcmp(s->name, "__call__") == 0 && s->kind == SYMBOL_FUNCTION) {
        /* TODO: implement __call__ dispatch — find the function in env, call with self + args */
        break;
      }
    }
  }

  return _eval_error_val(eval);
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
    comptime_value_t field = comptime_value_get_field(host, fname, eval->allocator);
    if (field) return _eval_temp(eval, field);  /* owned, tracked as temp */

    /* No field match — check instance methods */
    if (host->type && host->type->instance_methods) {
      size_t mc = vec_get_size(host->type->instance_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *s = (struct symbol *)vec_get(host->type->instance_methods, i);
        if (s && s->name && strcmp(s->name, fname) == 0 && s->kind == SYMBOL_FUNCTION) {
          comptime_value_t method_val = _comptime_create_method_value(eval, ctx, s);
          if (method_val) return _eval_temp(eval, method_val);
        }
      }
    }

    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                         "no field or method '%s' in composite", fname);
    ctx->error_count++;
    return _eval_error_val(eval);
  }

  if (host->kind == COMPTIME_VALUE_TYPE && host->type_val) {
    semantic_type_t t = host->type_val;
    if (t->static_fields) {
      size_t fc = vec_get_size(t->static_fields);
      for (size_t i = 0; i < fc; i++) {
        struct symbol *s = (struct symbol *)vec_get(t->static_fields, i);
        if (s && s->name && strcmp(s->name, fname) == 0 && s->kind == SYMBOL_VARIABLE) {
          comptime_value_t v = comptime_env_lookup_value(eval->current_env, eval->valloc, s->name);
          if (v) return v;  /* borrowed from env */
        }
      }
    }
  }

  if (host->kind == COMPTIME_VALUE_POINTER) {
    comptime_value_t pointed = comptime_alloc_read(eval->valloc, host->pointer.addr);
    if (!pointed) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "dereference of dangling pointer");
      ctx->error_count++;
      return _eval_error_val(eval);
    }
    if (pointed->kind == COMPTIME_VALUE_COMPOSITE) {
      comptime_value_t field = comptime_value_get_field(pointed, fname, eval->allocator);
      if (field) return _eval_temp(eval, field);
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
        return _eval_temp(eval, comptime_value_create_type(eval->allocator, s->type.type));
    }
  }
  if (t->static_methods) {
    size_t mc = vec_get_size(t->static_methods);
    for (size_t i = 0; i < mc; i++) {
      struct symbol *s = (struct symbol *)vec_get(t->static_methods, i);
      if (s && s->name && strcmp(s->name, fname) == 0 && s->kind == SYMBOL_FUNCTION) {
        comptime_value_t v = comptime_env_lookup_value(eval->current_env, eval->valloc, s->name);
        if (v) return v;  /* borrowed from env */
      }
    }
  }
  if (t->static_fields) {
    size_t fc = vec_get_size(t->static_fields);
    for (size_t i = 0; i < fc; i++) {
      struct symbol *s = (struct symbol *)vec_get(t->static_fields, i);
      if (s && s->name && strcmp(s->name, fname) == 0 && s->kind == SYMBOL_VARIABLE) {
        comptime_value_t v = comptime_env_lookup_value(eval->current_env, eval->valloc, s->name);
        if (v) return v;  /* borrowed from env */
      }
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
  cubec_expression_typeof_t to = (cubec_expression_typeof_t)node;
  /* typeof can wrap both value expressions (typeof(42)) and type expressions
   * (typeof(i32)). Try eval first — if the inner expression produces a
   * comptime value, extract its type field. Otherwise fall back to
   * resolver_resolve_type for pure type expressions. */
  comptime_value_t inner = _comptime_eval_expr(eval, ctx, to->expression);
  if (inner && inner->kind != COMPTIME_VALUE_ERROR) {
    if (inner->kind == COMPTIME_VALUE_TYPE)
      return _eval_temp(eval, comptime_value_create_type(eval->allocator, inner->type_val));
    if (inner->type)
      return _eval_temp(eval, comptime_value_create_type(eval->allocator, inner->type));
  }
  /* Fallback: pure type expression (e.g. typeof(*i32)) */
  semantic_type_t type = resolver_resolve_type(ctx, to->expression);
  if (!type) return _eval_error_val(eval);
  return _eval_temp(eval, comptime_value_create_type(eval->allocator, type));
}

static comptime_value_t _eval_sizeof(comptime_eval_t eval, checker_t ctx,
                                      node_t node) {
  semantic_type_t type = resolver_resolve_type(ctx,
      ((cubec_expression_sizeof_t)node)->expression);
  if (!type) return _eval_error_val(eval);
  type_layout_compute(type, 8);
  return _eval_temp(eval, comptime_value_create_int(eval->allocator, (int64_t)type->impl->size,
                                                      type->impl->size, 64, false, ctx->builtin_u64));
}

static comptime_value_t _eval_alignof(comptime_eval_t eval, checker_t ctx,
                                       node_t node) {
  semantic_type_t type = resolver_resolve_type(ctx,
      ((cubec_expression_alignof_t)node)->expression);
  if (!type) return _eval_error_val(eval);
  type_layout_compute(type, 8);
  return _eval_temp(eval, comptime_value_create_int(eval->allocator, (int64_t)type->impl->alignment,
                                                      type->impl->alignment, 64, false, ctx->builtin_u64));
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

  /* Create isolated environment for captured values (by-value capture) */
  comptime_env_t captured_env = comptime_env_create(eval->allocator, NULL);
  if (fn->captures) {
    size_t cc = vec_get_size(fn->captures);
    for (size_t i = 0; i < cc; i++) {
      node_t cap_node = (node_t)vec_get(fn->captures, i);
      if (cap_node->kind != CUBEC_NODE_FUNCTION_CAPTURE) continue;
      cubec_function_capture_t cap = (cubec_function_capture_t)cap_node;
      const char *cap_name = _checker_ident_str(cap->identifier);
      if (!cap_name) continue;

      /* Look up the value in current environment */
      comptime_value_t val = comptime_env_lookup_value(
          eval->current_env, eval->valloc, cap_name);
      if (val) {
        /* Clone the value for by-value capture */
        comptime_value_t cloned = comptime_value_clone(eval->allocator, val);
        if (cloned) {
          comptime_env_bind_value(captured_env, eval->valloc, cap_name, cloned);
        }
      }
    }
  }

  semantic_type_t ftype = NULL;
  struct symbol *sym = fn->name
      ? scope_lookup(ctx->current_scope, _eval_ident_str(fn->name))
      : NULL;
  if (sym && sym->kind == SYMBOL_FUNCTION) ftype = sym->function.type;

  return _eval_temp(eval, comptime_value_create_function(eval->allocator, captured_env, fn->body,
                                                          param_names, ftype));
}

static comptime_value_t _eval_init_list(comptime_eval_t eval, checker_t ctx,
                                         node_t node) {
  cubec_expression_initialize_list_t il = (cubec_expression_initialize_list_t)node;
  semantic_type_t type = il->type ? resolver_resolve_type(ctx, il->type) : NULL;

  /* Anonymous initialize list: infer type from content */
  if (!type) {
    if (il->is_field && il->items && vec_get_size(il->items) > 0) {
      /* Named fields → anonymous struct */
      type = semantic_type_create_named(ctx->allocator, NULL, TYPE_STRUCT);
      vec_init_t fvi = {.auto_dispose = true};
      type->impl->struct_type.fields =
          (vec_t)allocator_create(ctx->allocator, &g_vec_type, &fvi);
      size_t ic = vec_get_size(il->items);
      for (size_t i = 0; i < ic; i++) {
        node_t item = (node_t)vec_get(il->items, i);
        if (item->kind != CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD) continue;
        cubec_expression_initialize_field_t f =
            (cubec_expression_initialize_field_t)item;
        const char *fname = _eval_ident_str((node_t)f->field);
        semantic_type_t ftype = f->value
            ? checker_check_expression(ctx, f->value)
            : ctx->error_type;
        struct symbol *fsym = symbol_create(ctx->allocator, fname, SYMBOL_FIELD,
                                            item->location);
        fsym->field.type = ftype;
        fsym->field.index = i;
        fsym->field.is_pub = true;
        vec_push(type->impl->struct_type.fields, fsym);
      }
      type_layout_compute(type, 8);
      type_hash_ensure(type);
      vec_push(ctx->all_types, type);
    } else if (il->items && vec_get_size(il->items) > 0) {
      /* Positional → tuple */
      size_t ic = vec_get_size(il->items);
      vec_init_t evi = {.auto_dispose = false};
      vec_t elem_types = (vec_t)allocator_create(ctx->allocator, &g_vec_type,
                                                   &evi);
      for (size_t i = 0; i < ic; i++) {
        node_t item = (node_t)vec_get(il->items, i);
        semantic_type_t et = checker_check_expression(ctx, item);
        vec_push(elem_types, et);
      }
      type = semantic_type_create_tuple(ctx->allocator, elem_types);
      type_layout_compute(type, 8);
      type_hash_ensure(type);
      vec_push(ctx->all_types, type);
    } else {
      /* Empty .{} → empty struct */
      type = semantic_type_create_named(ctx->allocator, NULL, TYPE_STRUCT);
      vec_init_t evi = {.auto_dispose = true};
      type->impl->struct_type.fields =
          (vec_t)allocator_create(ctx->allocator, &g_vec_type, &evi);
      type_layout_compute(type, 8);
      type_hash_ensure(type);
      vec_push(ctx->all_types, type);
    }
  }

  if (!type) return _eval_error_val(eval);

  type_layout_compute(type, 8);
  size_t data_size = type->impl->size;

  if (type->impl->kind == TYPE_STRUCT) {
    vec_t type_fields = type->impl->struct_type.fields;
    size_t field_count = type_fields ? vec_get_size(type_fields) : 0;

    comptime_value_t comp = comptime_value_create_composite(
        eval->allocator, type, NULL, data_size);

    if (il->is_field && il->items) {
      /* Field-initialized: set each named field, others stay zero */
      size_t ic = vec_get_size(il->items);
      for (size_t i = 0; i < ic; i++) {
        node_t item = (node_t)vec_get(il->items, i);
        if (item->kind != CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD) continue;
        cubec_expression_initialize_field_t f =
            (cubec_expression_initialize_field_t)item;
        const char *fname = _eval_ident_str((node_t)f->field);
        comptime_value_t v = _comptime_eval_expr(eval, ctx, f->value);
        if (v && v->kind != COMPTIME_VALUE_ERROR)
          comptime_value_set_field(comp, fname, v);
      }
    } else if (il->items) {
      /* Positional init — supports pack spread */
      size_t ic = vec_get_size(il->items);
      size_t field_idx = 0;
      /* Count total values (including expanded pack elements) for overflow check */
      size_t total_values = 0;
      for (size_t i = 0; i < ic; i++) {
        node_t item = (node_t)vec_get(il->items, i);

        if (item->kind == CUBEC_NODE_EXPRESSION_SPREAD) {
          /* Pack spread: evaluate and expand */
          comptime_value_t spread_val = _comptime_eval_expr(eval, ctx, item);
          if (spread_val && spread_val->kind == COMPTIME_VALUE_PACK) {
            vec_t elements = spread_val->pack.elements;
            size_t ecount = elements ? vec_get_size(elements) : 0;
            total_values += ecount;
            for (size_t j = 0; j < ecount && field_idx < field_count; j++, field_idx++) {
              struct symbol *fsym = (struct symbol *)vec_get(type_fields, field_idx);
              comptime_value_t ev = (comptime_value_t)vec_get(elements, j);
              if (ev && ev->kind != COMPTIME_VALUE_ERROR && fsym)
                comptime_value_write_field(comp, fsym->field.offset, fsym->field.type, ev);
            }
          }
        } else {
          /* Regular positional item */
          total_values++;
          if (field_idx < field_count) {
            struct symbol *fsym = (struct symbol *)vec_get(type_fields, field_idx);
            comptime_value_t v = _comptime_eval_expr(eval, ctx, item);
            if (v && v->kind != COMPTIME_VALUE_ERROR && fsym)
              comptime_value_write_field(comp, fsym->field.offset, fsym->field.type, v);
            field_idx++;
          }
        }
      }
      if (total_values > field_count) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "too many initializers for type '%s' (%zu values for %zu fields)",
                             type->name ? type->name : "<anonymous>",
                             total_values, field_count);
        ctx->error_count++;
      }
    }
    return _eval_temp(eval, comp);
  }

  if (type->impl->kind == TYPE_TUPLE) {
    /* Tuple initialization */
    vec_t type_fields = type->impl->tuple.fields;
    size_t field_count = type_fields ? vec_get_size(type_fields) : 0;

    comptime_value_t comp = comptime_value_create_composite(
        eval->allocator, type, NULL, data_size);

    if (il->is_field && il->items) {
      size_t ic = vec_get_size(il->items);
      for (size_t i = 0; i < ic; i++) {
        node_t item = (node_t)vec_get(il->items, i);
        if (item->kind != CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD) continue;
        cubec_expression_initialize_field_t f =
            (cubec_expression_initialize_field_t)item;
        const char *fname = _eval_ident_str((node_t)f->field);
        comptime_value_t v = _comptime_eval_expr(eval, ctx, f->value);
        if (v && v->kind != COMPTIME_VALUE_ERROR)
          comptime_value_set_field(comp, fname, v);
      }
    } else if (il->items) {
      /* Positional init — supports pack spread */
      size_t ic = vec_get_size(il->items);
      size_t field_idx = 0;
      size_t total_values = 0;
      for (size_t i = 0; i < ic; i++) {
        node_t item = (node_t)vec_get(il->items, i);

        if (item->kind == CUBEC_NODE_EXPRESSION_SPREAD) {
          comptime_value_t spread_val = _comptime_eval_expr(eval, ctx, item);
          if (spread_val && spread_val->kind == COMPTIME_VALUE_PACK) {
            vec_t elements = spread_val->pack.elements;
            size_t ecount = elements ? vec_get_size(elements) : 0;
            total_values += ecount;
            for (size_t j = 0; j < ecount && field_idx < field_count; j++, field_idx++) {
              struct symbol *fsym = (struct symbol *)vec_get(type_fields, field_idx);
              comptime_value_t ev = (comptime_value_t)vec_get(elements, j);
              if (ev && ev->kind != COMPTIME_VALUE_ERROR && fsym)
                comptime_value_write_field(comp, fsym->field.offset, fsym->field.type, ev);
            }
          }
        } else {
          total_values++;
          if (field_idx < field_count) {
            struct symbol *fsym = (struct symbol *)vec_get(type_fields, field_idx);
            comptime_value_t v = _comptime_eval_expr(eval, ctx, item);
            if (v && v->kind != COMPTIME_VALUE_ERROR && fsym)
              comptime_value_write_field(comp, fsym->field.offset, fsym->field.type, v);
            field_idx++;
          }
        }
      }
      if (total_values > field_count) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "too many initializers for type '%s' (%zu values for %zu fields)",
                             type->name ? type->name : "<tuple>",
                             total_values, field_count);
        ctx->error_count++;
      }
    }
    return _eval_temp(eval, comp);
  }

  if (type->impl->kind == TYPE_GENERIC_INSTANCE && type->impl->generic_instance.fields) {
    /* Generic instance (struct/union) initialization */
    vec_t type_fields = type->impl->generic_instance.fields;
    size_t field_count = type_fields ? vec_get_size(type_fields) : 0;

    comptime_value_t comp = comptime_value_create_composite(
        eval->allocator, type, NULL, data_size);

    if (il->is_field && il->items) {
      size_t ic = vec_get_size(il->items);
      for (size_t i = 0; i < ic; i++) {
        node_t item = (node_t)vec_get(il->items, i);
        if (item->kind != CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD) continue;
        cubec_expression_initialize_field_t f =
            (cubec_expression_initialize_field_t)item;
        const char *fname = _eval_ident_str((node_t)f->field);
        comptime_value_t v = _comptime_eval_expr(eval, ctx, f->value);
        if (v && v->kind != COMPTIME_VALUE_ERROR)
          comptime_value_set_field(comp, fname, v);
      }
    } else if (il->items) {
      /* Positional init — supports pack spread */
      size_t ic = vec_get_size(il->items);
      size_t field_idx = 0;
      size_t total_values = 0;
      for (size_t i = 0; i < ic; i++) {
        node_t item = (node_t)vec_get(il->items, i);

        if (item->kind == CUBEC_NODE_EXPRESSION_SPREAD) {
          comptime_value_t spread_val = _comptime_eval_expr(eval, ctx, item);
          if (spread_val && spread_val->kind == COMPTIME_VALUE_PACK) {
            vec_t elements = spread_val->pack.elements;
            size_t ecount = elements ? vec_get_size(elements) : 0;
            total_values += ecount;
            for (size_t j = 0; j < ecount && field_idx < field_count; j++, field_idx++) {
              struct symbol *fsym = (struct symbol *)vec_get(type_fields, field_idx);
              comptime_value_t ev = (comptime_value_t)vec_get(elements, j);
              if (ev && ev->kind != COMPTIME_VALUE_ERROR && fsym)
                comptime_value_write_field(comp, fsym->field.offset, fsym->field.type, ev);
            }
          }
        } else {
          total_values++;
          if (field_idx < field_count) {
            struct symbol *fsym = (struct symbol *)vec_get(type_fields, field_idx);
            comptime_value_t v = _comptime_eval_expr(eval, ctx, item);
            if (v && v->kind != COMPTIME_VALUE_ERROR && fsym)
              comptime_value_write_field(comp, fsym->field.offset, fsym->field.type, v);
            field_idx++;
          }
        }
      }
      if (total_values > field_count) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                             "too many initializers for type '%s' (%zu values for %zu fields)",
                             type->name ? type->name : "<anonymous>",
                             total_values, field_count);
        ctx->error_count++;
      }
    }
    return _eval_temp(eval, comp);
  }

  if (type->impl->kind == TYPE_ARRAY && il->items) {
    semantic_type_t elem_type = type->impl->array.element;
    type_layout_compute(elem_type, 8);
    size_t ic = vec_get_size(il->items);

    comptime_value_t comp = comptime_value_create_composite(
        eval->allocator, type, elem_type, data_size);

    size_t arr_idx = 0;
    for (size_t i = 0; i < ic; i++) {
      node_t item = (node_t)vec_get(il->items, i);

      if (item->kind == CUBEC_NODE_EXPRESSION_SPREAD) {
        /* Pack spread: evaluate and expand */
        comptime_value_t spread_val = _comptime_eval_expr(eval, ctx, item);
        if (spread_val && spread_val->kind == COMPTIME_VALUE_PACK) {
          vec_t elements = spread_val->pack.elements;
          size_t ecount = elements ? vec_get_size(elements) : 0;
          for (size_t j = 0; j < ecount && arr_idx < (size_t)type->impl->array.length;
               j++, arr_idx++) {
            comptime_value_t ev = (comptime_value_t)vec_get(elements, j);
            if (ev && ev->kind != COMPTIME_VALUE_ERROR)
              comptime_value_set_index(comp, arr_idx, ev);
          }
        }
      } else {
        comptime_value_t v = _comptime_eval_expr(eval, ctx, item);
        if (v && v->kind != COMPTIME_VALUE_ERROR)
          comptime_value_set_index(comp, arr_idx, v);
        arr_idx++;
      }
    }
    return _eval_temp(eval, comp);
  }

  return _eval_error_val(eval);
}

static comptime_value_t _eval_comma(comptime_eval_t eval, checker_t ctx,
                                     node_t node) {
  cubec_expression_comma_t c = (cubec_expression_comma_t)node;
  _comptime_eval_expr(eval, ctx, c->left);  /* side effect only, result is temporary */
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
  return pointed;  /* borrowed from alloc */
}

static comptime_value_t _eval_addr(comptime_eval_t eval, checker_t ctx,
                                    node_t node) {
  cubec_expression_binary_t addr = (cubec_expression_binary_t)node;
  comptime_value_t val = _comptime_eval_expr(eval, ctx, addr->right);
  if (!val) return _eval_error_val(eval);

  if (addr->right->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
    const char *name = _eval_ident_str(addr->right);
    uint64_t a = comptime_env_lookup_addr(eval->current_env, name);
    if (a) {
      /* Directly use the addr from env — no clone! Pointer references the
         same alloc slot as the variable, so writes through the pointer
         modify the original variable. */
      comptime_value_t existing = comptime_alloc_read(eval->valloc, a);
      semantic_type_t ptr_type = existing && existing->type
          ? semantic_type_create_pointer(eval->allocator, existing->type)
          : NULL;
      if (ptr_type) vec_push(ctx->all_types, ptr_type);
      return _eval_temp(eval, comptime_value_create_pointer(eval->allocator, a, ptr_type));
    }
  }

  uint64_t a = comptime_alloc_allocate(eval->valloc,
                                        comptime_value_clone(eval->allocator, val),
                                        eval->valloc->scope_depth);
  semantic_type_t ptr_type = val->type
      ? semantic_type_create_pointer(eval->allocator, val->type)
      : NULL;
  if (ptr_type) vec_push(ctx->all_types, ptr_type);
  return _eval_temp(eval, comptime_value_create_pointer(eval->allocator, a, ptr_type));
}

static comptime_value_t _eval_slice(comptime_eval_t eval, checker_t ctx,
                                     node_t node) {
  cubec_expression_slice_t sl = (cubec_expression_slice_t)node;
  comptime_value_t host = _comptime_eval_expr(eval, ctx, sl->host);
  if (!host || host->kind == COMPTIME_VALUE_ERROR) return _eval_error_val(eval);

  /* resolve start/length indices */
  size_t start = 0;
  size_t len = 0;
  if (sl->start) {
    comptime_value_t sv = _comptime_eval_expr(eval, ctx, sl->start);
    if (!sv || sv->kind == COMPTIME_VALUE_ERROR) return _eval_error_val(eval);
    start = (size_t)comptime_value_as_u64(sv);
  }
  if (sl->length) {
    comptime_value_t lv = _comptime_eval_expr(eval, ctx, sl->length);
    if (!lv || lv->kind == COMPTIME_VALUE_ERROR) return _eval_error_val(eval);
    len = (size_t)comptime_value_as_u64(lv);
  }

  /* TODO: check for __get__ / slice magic method on host type */

  /* string slice */
  if (host->kind == COMPTIME_VALUE_STRING) {
    const char *s = comptime_value_get_string(host);
    if (!s) return _eval_error_val(eval);
    size_t slen = strlen(s);
    if (start > slen) start = slen;
    if (len > slen - start) len = slen - start;
    char *buf = (char *)allocator_alloc(eval->allocator, len + 1);
    memcpy(buf, s + start, len);
    buf[len] = '\0';
    comptime_value_t result = comptime_value_create_string(eval->allocator,
                                                            buf, ctx->builtin_string);
    allocator_free(eval->allocator, &buf);
    return _eval_temp(eval, result);
  }

  /* composite (array) slice */
  if (host->kind == COMPTIME_VALUE_COMPOSITE && host->composite.element_type) {
    size_t elem_size = host->composite.element_type->impl->size;
    size_t total = elem_size > 0 ? host->composite.data_size / elem_size : 0;
    if (start > total) start = total;
    if (len > total - start) len = total - start;
    size_t slice_data_size = len * elem_size;
    comptime_value_t slice = comptime_value_create_composite(
        eval->allocator, NULL, host->composite.element_type, slice_data_size);
    if (slice_data_size > 0)
      memcpy(slice->composite.data, host->composite.data + start * elem_size,
             slice_data_size);
    return _eval_temp(eval, slice);
  }

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
      return _eval_temp(eval, comptime_value_create_type(eval->allocator, sym->type.type));
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
  case CUBEC_NODE_LITERAL_UNDEFINED:
    /* undefined is not directly evaluable — it only makes sense as an
     * initializer in a declaration (handled in comptime_eval_stmt).
     * Returning error here prevents comptime evaluation of the bare literal. */
    return _eval_error_val(eval);
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION:
    return _eval_generic_inst(eval, ctx, expr);
  case CUBEC_NODE_EXPRESSION_TYPE_STRUCT:
  case CUBEC_NODE_EXPRESSION_TYPE_TUPLE:
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
    return _eval_temp(eval, comptime_value_create_type(eval->allocator, type));
  }
  default:
    return _eval_error_val(eval);
  }
}

comptime_value_t comptime_eval_expr(comptime_eval_t eval, checker_t ctx,
                                     node_t expr) {
  return _comptime_eval_expr(eval, ctx, expr);
}
