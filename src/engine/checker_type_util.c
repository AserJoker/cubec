#include "engine/checker.h"
#include "engine/checker_type_util.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/node.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/generic_param.h"
#include <string.h>

/* ===== identifier helper ===== */

const char *_checker_ident_str(node_t id_node) {
  if (!id_node) return NULL;
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)id_node;
  return string_get(id->value);
}

/* ===== type predicates ===== */

bool _is_numeric_type(semantic_type_t t) {
  if (!t || !t->impl) return false;
  enum type_kind k = t->impl->kind;
  return (k >= TYPE_I8 && k <= TYPE_U64) || (k >= TYPE_F16 && k <= TYPE_F64);
}

bool _is_integer_type(semantic_type_t t) {
  if (!t || !t->impl) return false;
  enum type_kind k = t->impl->kind;
  return k >= TYPE_I8 && k <= TYPE_U64;
}

bool _is_bool_type(semantic_type_t t) {
  return t && t->impl && t->impl->kind == TYPE_BOOL;
}

bool _is_lvalue(node_t expr) {
  if (!expr) return false;
  switch (expr->kind) {
  case CUBEC_NODE_LITERAL_IDENTIFIER:
  case CUBEC_NODE_EXPRESSION_MEMBER:
  case CUBEC_NODE_EXPRESSION_DEREF:
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION:
    return true;
  default:
    return false;
  }
}

bool _is_struct_like(semantic_type_t t) {
  if (!t || !t->impl) return false;
  if (t->impl->kind == TYPE_STRUCT || t->impl->kind == TYPE_UNION ||
      t->impl->kind == TYPE_CUNION)
    return true;
  if (t->impl->kind == TYPE_GENERIC_INSTANCE) {
    semantic_type_t tmpl = t->impl->generic_instance.generic_template;
    return tmpl && (tmpl->impl->kind == TYPE_STRUCT ||
                    tmpl->impl->kind == TYPE_UNION ||
                    tmpl->impl->kind == TYPE_CUNION);
  }
  return false;
}

vec_t _get_struct_fields(semantic_type_t t) {
  if (!t || !t->impl) return NULL;
  if (t->impl->kind == TYPE_GENERIC_INSTANCE)
    return t->impl->generic_instance.fields;
  return t->impl->struct_type.fields;
}

/* ===== type utilities ===== */

semantic_type_t _common_type(checker_t ctx, semantic_type_t a,
                             semantic_type_t b) {
  if (!a || !b) return ctx->error_type;
  if (semantic_type_equals(a, b)) return a;
  /* int + float → float */
  if (_is_integer_type(a) && b->impl->kind >= TYPE_F16 &&
      b->impl->kind <= TYPE_F64)
    return b;
  if (_is_integer_type(b) && a->impl->kind >= TYPE_F16 &&
      a->impl->kind <= TYPE_F64)
    return a;
  /* float widening */
  if (a->impl->size >= b->impl->size) return a;
  return b;
}

/* ===== generic instantiation helpers ===== */

char *_generic_instance_cache_key(checker_t ctx, const char *template_name,
                                   vec_t type_args) {
  size_t len = strlen(template_name);
  size_t acount = type_args ? vec_get_size(type_args) : 0;
  for (size_t i = 0; i < acount; i++) {
    semantic_type_t t = (semantic_type_t)vec_get(type_args, i);
    type_hash_ensure(t);
    len += 1 + 20; /* '#' + max uint64 decimal */
  }
  char *key = (char *)allocator_alloc(ctx->allocator, len + 1);
  size_t pos = 0;
  memcpy(key + pos, template_name, strlen(template_name));
  pos += strlen(template_name);
  for (size_t i = 0; i < acount; i++) {
    semantic_type_t t = (semantic_type_t)vec_get(type_args, i);
    key[pos++] = '#';
    pos += snprintf(key + pos, len + 1 - pos, "%zu", t->impl->hash);
  }
  key[pos] = '\0';
  return key;
}

bool _check_constraint(checker_t ctx, semantic_type_t type_arg,
                       semantic_type_t constraint, node_t arg_expr) {
  if (!constraint) return true;

  switch (constraint->impl->kind) {
  case TYPE_INTERFACE: {
    /* Check that type_arg implements all required methods */
    vec_t required_methods = constraint->impl->interface_type.methods;
    size_t mcount = required_methods ? vec_get_size(required_methods) : 0;

    vec_t instance_methods = type_arg->instance_methods;
    size_t imcount = instance_methods ? vec_get_size(instance_methods) : 0;

    for (size_t i = 0; i < mcount; i++) {
      struct symbol *req = (struct symbol *)vec_get(required_methods, i);
      if (!req || !req->name) continue;
      bool found = false;
      for (size_t j = 0; j < imcount; j++) {
        struct symbol *has = (struct symbol *)vec_get(instance_methods, j);
        if (has && has->name && strcmp(has->name, req->name) == 0) {
          if (req->function.type && has->function.type &&
              semantic_type_equals(req->function.type, has->function.type)) {
            found = true;
            break;
          }
        }
      }
      if (!found) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: missing method '%s'",
                             req->name);
        ctx->error_count++;
        return false;
      }
    }
    return true;
  }

  case TYPE_GENERIC_INSTANCE: {
    /* Constraint like T extends Vec[Readable]
       Check that type_arg is a GENERIC_INSTANCE of the same template
       and that the corresponding type args satisfy the constraint's args.
       Wildcard ? in constraint skips the corresponding arg check. */
    if (type_arg->impl->kind != TYPE_GENERIC_INSTANCE) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                           "type does not satisfy generic constraint: not a generic instance");
      ctx->error_count++;
      return false;
    }

    /* Same template */
    if (type_arg->impl->generic_instance.generic_template !=
        constraint->impl->generic_instance.generic_template) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                           "type does not satisfy constraint: wrong generic template");
      ctx->error_count++;
      return false;
    }

    /* Check type args pairwise */
    vec_t a_args = type_arg->impl->generic_instance.type_args;
    vec_t c_args = constraint->impl->generic_instance.type_args;
    size_t ac = a_args ? vec_get_size(a_args) : 0;
    size_t cc = c_args ? vec_get_size(c_args) : 0;
    if (ac != cc) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                           "type does not satisfy constraint: type arg count mismatch");
      ctx->error_count++;
      return false;
    }

    for (size_t i = 0; i < cc; i++) {
      semantic_type_t ca = (semantic_type_t)vec_get(c_args, i);
      semantic_type_t aa = (semantic_type_t)vec_get(a_args, i);
      /* Wildcard in constraint skips this arg check */
      if (ca->impl->kind == TYPE_WILDCARD) continue;
      /* Recursively check nested constraint */
      if (!_check_constraint(ctx, aa, ca, arg_expr)) return false;
    }
    return true;
  }

  case TYPE_POINTER: {
    /* Constraint like *Readable — check pointee */
    if (type_arg->impl->kind != TYPE_POINTER) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                           "type does not satisfy constraint: expected pointer type");
      ctx->error_count++;
      return false;
    }
    return _check_constraint(ctx, type_arg->impl->pointer.pointee,
                             constraint->impl->pointer.pointee, arg_expr);
  }

  case TYPE_SLICE: {
    /* Constraint like []Readable — check element */
    if (type_arg->impl->kind != TYPE_SLICE) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                           "type does not satisfy constraint: expected slice type");
      ctx->error_count++;
      return false;
    }
    return _check_constraint(ctx, type_arg->impl->slice.element,
                             constraint->impl->slice.element, arg_expr);
  }

  case TYPE_ARRAY: {
    /* Constraint like [N]? — check element, ignore length if wildcard */
    if (type_arg->impl->kind != TYPE_ARRAY) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                           "type does not satisfy constraint: expected array type");
      ctx->error_count++;
      return false;
    }
    if (constraint->impl->array.length != type_arg->impl->array.length &&
        constraint->impl->array.length != 0) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                           "type does not satisfy constraint: array length mismatch");
      ctx->error_count++;
      return false;
    }
    return _check_constraint(ctx, type_arg->impl->array.element,
                             constraint->impl->array.element, arg_expr);
  }

  case TYPE_STRUCT:
  case TYPE_UNION:
  case TYPE_CUNION: {
    /* Structural constraint: check that type_arg has at least the fields
       with compatible types (wildcard ? skips type check for a field) */
    if (type_arg->impl->kind != constraint->impl->kind) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                           "type does not satisfy constraint: type kind mismatch");
      ctx->error_count++;
      return false;
    }
    vec_t c_fields = constraint->impl->struct_type.fields;
    vec_t a_fields = type_arg->impl->struct_type.fields;
    size_t cfc = c_fields ? vec_get_size(c_fields) : 0;
    size_t afc = a_fields ? vec_get_size(a_fields) : 0;
    if (afc < cfc) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                           "type does not satisfy constraint: missing fields");
      ctx->error_count++;
      return false;
    }
    for (size_t i = 0; i < cfc; i++) {
      struct symbol *cf = (struct symbol *)vec_get(c_fields, i);
      struct symbol *af = (struct symbol *)vec_get(a_fields, i);
      if (!cf || !cf->name) continue;
      if (!af || !af->name || strcmp(cf->name, af->name) != 0) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: field '%s' mismatch",
                             cf->name);
        ctx->error_count++;
        return false;
      }
      /* Wildcard ? skips field type check */
      if (cf->field.type && cf->field.type->impl->kind == TYPE_WILDCARD) continue;
      if (!_check_constraint(ctx, af->field.type, cf->field.type, arg_expr))
        return false;
    }
    return true;
  }

  default:
    /* Other constraint types: structural equality check */
    return true;
  }
}

bool _check_generic_param_constraints(checker_t ctx, vec_t generic_params,
                                       vec_t type_args, node_t expr) {
  if (!generic_params || !type_args) return true;
  size_t gcount = vec_get_size(generic_params);
  bool all_ok = true;

  for (size_t i = 0; i < gcount; i++) {
    node_t gp_node = (node_t)vec_get(generic_params, i);
    if (!gp_node || gp_node->kind != CUBEC_NODE_GENERIC_PARAM) continue;

    cubec_generic_param_t gp = (cubec_generic_param_t)gp_node;
    if (!gp->constraint) continue;

    if (i >= vec_get_size(type_args)) continue;
    semantic_type_t ta = (semantic_type_t)vec_get(type_args, i);
    if (!ta) continue;

    semantic_type_t constraint_type = resolver_resolve_type(ctx, gp->constraint);
    if (!constraint_type || constraint_type->impl->kind == TYPE_ERROR) continue;

    if (!_check_constraint(ctx, ta, constraint_type, expr)) {
      all_ok = false;
    }
  }
  return all_ok;
}

vec_t _resolve_generic_type_args(checker_t ctx, vec_t arg_exprs) {
  if (!arg_exprs) return NULL;
  size_t acount = vec_get_size(arg_exprs);
  if (acount == 0) return NULL;

  vec_init_t vi = {.auto_dispose = false};
  vec_t type_args = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  for (size_t i = 0; i < acount; i++) {
    node_t arg = (node_t)vec_get(arg_exprs, i);
    semantic_type_t t = resolver_resolve_type(ctx, arg);
    if (t->impl->kind == TYPE_ERROR) {
      /* Forward-declared to allow late resolution */
      extern semantic_type_t _check_expression(checker_t, node_t);
      t = _check_expression(ctx, arg);
    }
    if (!t || t->impl->kind == TYPE_ERROR) {
      allocator_free(ctx->allocator, &type_args);
      return NULL;
    }
    vec_push(type_args, t);
  }
  return type_args;
}

static vec_t _copy_symbol_vec(checker_t ctx, vec_t src) {
  if (!src) return NULL;
  vec_init_t vi = {.auto_dispose = true};
  vec_t dst = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  size_t count = vec_get_size(src);
  for (size_t i = 0; i < count; i++)
    vec_push(dst, (struct symbol *)vec_get(src, i));
  return dst;
}

static void _instantiate_struct_fields(checker_t ctx, semantic_type_t inst,
                                        vec_t tpl_fields, vec_t type_args) {
  vec_init_t vi = {.auto_dispose = true};
  inst->impl->generic_instance.fields =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  size_t fcount = tpl_fields ? vec_get_size(tpl_fields) : 0;
  for (size_t i = 0; i < fcount; i++) {
    struct symbol *f = (struct symbol *)vec_get(tpl_fields, i);
    struct symbol *nf = symbol_create(ctx->allocator, f->name,
                                       SYMBOL_FIELD, f->location);
    nf->field.index = i;
    nf->field.is_pub = f->field.is_pub;
    /* Substitute generic params in field type */
    nf->field.type = semantic_type_substitute(ctx->allocator, f->field.type, type_args, ctx->all_types);
    vec_push(inst->impl->generic_instance.fields, nf);
  }
  type_layout_compute(inst, 8);
}

semantic_type_t _instantiate_type(checker_t ctx, semantic_type_t template_type,
                                   vec_t type_args, node_t instantiation_expr) {
  const char *name = template_type->name;
  if (!name) name = "<anonymous>";

  /* Check cache */
  char *key = _generic_instance_cache_key(ctx, name, type_args);
  void *cached = strmap_find(ctx->type_impl_cache, key);
  allocator_free(ctx->allocator, &key);
  if (cached) {
    allocator_free(ctx->allocator, &type_args);
    return (semantic_type_t)cached;
  }

  /* Create the specialized type as a GENERIC_INSTANCE */
  semantic_type_t inst = semantic_type_create_generic_instance(
      ctx->allocator, template_type, type_args);
  vec_push(ctx->all_types, inst);
  type_hash_ensure(inst);

  /* Copy structural info from template based on kind */
  enum type_kind tkind = template_type->impl->kind;
  if (tkind == TYPE_STRUCT || tkind == TYPE_UNION || tkind == TYPE_CUNION)
    _instantiate_struct_fields(ctx, inst, template_type->impl->struct_type.fields, type_args);

  /* Copy method lists from template (free the init-created vecs first) */
  allocator_free(ctx->allocator, &inst->instance_methods);
  allocator_free(ctx->allocator, &inst->static_methods);
  inst->instance_methods = _copy_symbol_vec(ctx, template_type->instance_methods);
  inst->static_methods = _copy_symbol_vec(ctx, template_type->static_methods);

  /* Cache the result */
  key = _generic_instance_cache_key(ctx, name, type_args);
  strmap_insert(ctx->type_impl_cache, key, inst);
  allocator_free(ctx->allocator, &key);

  return inst;
}

semantic_type_t _instantiate_function(checker_t ctx, struct symbol *func_sym,
                                      vec_t type_args, node_t instantiation_expr) {
  const char *name = func_sym->name;
  semantic_type_t func_type = func_sym->function.type;
  if (!func_type) return ctx->error_type;

  /* Check cache */
  char *key = _generic_instance_cache_key(ctx, name, type_args);
  void *cached = strmap_find(ctx->type_impl_cache, key);
  allocator_free(ctx->allocator, &key);
  if (cached) {
    allocator_free(ctx->allocator, &type_args);
    return (semantic_type_t)cached;
  }

  /* Substitute generic params in params and return_type */
  vec_init_t vi = {.auto_dispose = false};
  vec_t new_params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  size_t pcount = vec_get_size(func_type->impl->function.params);
  for (size_t i = 0; i < pcount; i++) {
    semantic_type_t p = (semantic_type_t)vec_get(func_type->impl->function.params, i);
    semantic_type_t new_p = semantic_type_substitute(ctx->allocator, p, type_args, ctx->all_types);
    vec_push(new_params, new_p);
  }

  semantic_type_t new_return = semantic_type_substitute(ctx->allocator,
      func_type->impl->function.return_type, type_args, ctx->all_types);

  /* Create new function type with substituted types */
  semantic_type_t inst_type = semantic_type_create_function(ctx->allocator,
      new_return, new_params, func_type->impl->function.is_variadic);
  vec_push(ctx->all_types, inst_type);
  type_hash_ensure(inst_type);

  /* Cache */
  key = _generic_instance_cache_key(ctx, name, type_args);
  strmap_insert(ctx->type_impl_cache, key, inst_type);
  allocator_free(ctx->allocator, &key);

  /* type_args is not stored in the function type, free it */
  allocator_free(ctx->allocator, &type_args);

  return inst_type;
}

/* ===== literal numeric helper ===== */

semantic_type_t _check_literal_numeric(checker_t ctx, node_t num_node) {
  if (!num_node) return ctx->error_type;
  cubec_literal_numeric_t num = (cubec_literal_numeric_t)num_node;
  switch (num->numeric_type) {
  case CUBEC_LITERAL_NUMERIC_TYPE_I8:  return ctx->builtin_i8;
  case CUBEC_LITERAL_NUMERIC_TYPE_I16: return ctx->builtin_i16;
  case CUBEC_LITERAL_NUMERIC_TYPE_I32: return ctx->builtin_i32;
  case CUBEC_LITERAL_NUMERIC_TYPE_I64: return ctx->builtin_i64;
  case CUBEC_LITERAL_NUMERIC_TYPE_U8:  return ctx->builtin_u8;
  case CUBEC_LITERAL_NUMERIC_TYPE_U16: return ctx->builtin_u16;
  case CUBEC_LITERAL_NUMERIC_TYPE_U32: return ctx->builtin_u32;
  case CUBEC_LITERAL_NUMERIC_TYPE_U64: return ctx->builtin_u64;
  case CUBEC_LITERAL_NUMERIC_TYPE_F16: return ctx->builtin_f16;
  case CUBEC_LITERAL_NUMERIC_TYPE_F32: return ctx->builtin_f32;
  case CUBEC_LITERAL_NUMERIC_TYPE_F64: return ctx->builtin_f64;
  default:
    return num->kind == CUBEC_LITERAL_NUMERIC_KIND_FLOAT
               ? ctx->builtin_f64
               : ctx->builtin_i32;
  }
}
