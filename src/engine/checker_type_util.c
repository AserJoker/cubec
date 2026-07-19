#include "engine/checker.h"
#include "engine/checker_type_util.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "engine/comptime_value.h"
#include "engine/comptime_eval.h"
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
    if (!t) continue;
    if (t->impl->kind == TYPE_GENERIC_PACK) {
      /* Pack: expand each type in the pack */
      size_t ecount = vec_get_size(t->impl->generic_pack.expanded_types);
      for (size_t j = 0; j < ecount; j++) {
        semantic_type_t et = (semantic_type_t)vec_get(t->impl->generic_pack.expanded_types, j);
        if (et) { type_hash_ensure(et); }
        len += 1 + 20; /* '#' + max uint64 decimal */
      }
    } else if (t->impl->kind == TYPE_GENERIC_VALUE) {
      /* Value: hash the compile-time value content */
      len += 1 + 30; /* '#' + value representation */
    } else {
      type_hash_ensure(t);
      len += 1 + 20; /* '#' + max uint64 decimal */
    }
  }
  char *key = (char *)allocator_alloc(ctx->allocator, len + 1);
  size_t pos = 0;
  memcpy(key + pos, template_name, strlen(template_name));
  pos += strlen(template_name);
  for (size_t i = 0; i < acount; i++) {
    semantic_type_t t = (semantic_type_t)vec_get(type_args, i);
    if (!t) continue;
    if (t->impl->kind == TYPE_GENERIC_PACK) {
      size_t ecount = vec_get_size(t->impl->generic_pack.expanded_types);
      for (size_t j = 0; j < ecount; j++) {
        semantic_type_t et = (semantic_type_t)vec_get(t->impl->generic_pack.expanded_types, j);
        if (!et) continue;
        key[pos++] = '#';
        pos += snprintf(key + pos, len + 1 - pos, "%zu", et->impl->hash);
      }
    } else if (t->impl->kind == TYPE_GENERIC_VALUE) {
      key[pos++] = 'v';
      comptime_value_t cv = t->impl->generic_value.value;
      if (cv) {
        switch (cv->kind) {
        case COMPTIME_VALUE_INT:
          pos += snprintf(key + pos, len + 1 - pos, "%zu", (size_t)cv->int_val.u);
          break;
        case COMPTIME_VALUE_BOOL:
          pos += snprintf(key + pos, len + 1 - pos, "%d", cv->bool_val ? 1 : 0);
          break;
        default:
          pos += snprintf(key + pos, len + 1 - pos, "%zu", (size_t)cv->kind);
          break;
        }
      }
    } else {
      key[pos++] = '#';
      pos += snprintf(key + pos, len + 1 - pos, "%zu", t->impl->hash);
    }
  }
  key[pos] = '\0';
  return key;
}

static semantic_type_t _cache_lookup(checker_t ctx, const char *name,
                                     vec_t type_args) {
  char *key = _generic_instance_cache_key(ctx, name, type_args);
  void *found = strmap_find(ctx->type_impl_cache, key);
  allocator_free(ctx->allocator, &key);
  return found ? (semantic_type_t)found : NULL;
}

static void _cache_insert(checker_t ctx, const char *name,
                           vec_t type_args, semantic_type_t type) {
  char *key = _generic_instance_cache_key(ctx, name, type_args);
  strmap_insert(ctx->type_impl_cache, key, type);
  allocator_free(ctx->allocator, &key);
}

/* ===== type substitution (internal to generic instantiation) ===== */

/* Forward declaration — needed because _substitute_type delegates to _instantiate_type */
static semantic_type_t _substitute_type(checker_t ctx, semantic_type_t type,
                                         vec_t type_args);

static semantic_type_t _substitute_type(checker_t ctx, semantic_type_t type,
                                         vec_t type_args) {
  if (!type || !type->impl) return type;

  switch (type->impl->kind) {
  case TYPE_GENERIC_PARAM: {
    size_t idx = type->impl->generic_param.index;
    if (type_args && idx < vec_get_size(type_args)) {
      semantic_type_t replacement = (semantic_type_t)vec_get(type_args, idx);
      if (replacement) return replacement;
    }
    return type;
  }

  case TYPE_GENERIC_PACK: {
    /* Substitute the pack parameter with the corresponding type_args entry.
       The entry should be a TYPE_GENERIC_PACK containing expanded_types. */
    size_t idx = type->impl->generic_pack.index;
    if (type_args && idx < vec_get_size(type_args)) {
      semantic_type_t replacement = (semantic_type_t)vec_get(type_args, idx);
      if (replacement) return replacement;
    }
    return type;
  }

  case TYPE_POINTER: {
    semantic_type_t inner = _substitute_type(ctx, type->impl->pointer.pointee, type_args);
    if (inner == type->impl->pointer.pointee) return type;
    semantic_type_t result = semantic_type_create_pointer(ctx->allocator, inner);
    type_hash_ensure(result);
    vec_push(ctx->all_types, result);
    return result;
  }

  case TYPE_SLICE: {
    semantic_type_t elem = _substitute_type(ctx, type->impl->slice.element, type_args);
    if (elem == type->impl->slice.element) return type;
    semantic_type_t result = semantic_type_create_slice(ctx->allocator, elem);
    type_hash_ensure(result);
    vec_push(ctx->all_types, result);
    return result;
  }

  case TYPE_ARRAY: {
    semantic_type_t elem = _substitute_type(ctx, type->impl->array.element, type_args);
    size_t param_idx = type->impl->array.length_param_idx;

    if (param_idx != (size_t)-1 && type_args && param_idx < vec_get_size(type_args)) {
      semantic_type_t replacement = (semantic_type_t)vec_get(type_args, param_idx);
      if (replacement && replacement->impl->kind == TYPE_GENERIC_VALUE) {
        /* Resolve symbolic length from TYPE_GENERIC_VALUE */
        size_t concrete_len =
            (size_t)comptime_value_as_u64(replacement->impl->generic_value.value);
        if (elem == type->impl->array.element && concrete_len == type->impl->array.length)
          return type;
        semantic_type_t result = semantic_type_create_array(
            ctx->allocator, elem, concrete_len, (size_t)-1);
        type_hash_ensure(result);
        vec_push(ctx->all_types, result);
        return result;
      }
      /* Replacement not yet a concrete value — propagate symbolic array */
      if (elem == type->impl->array.element) return type;
      semantic_type_t result = semantic_type_create_array(
          ctx->allocator, elem, 0, param_idx);
      type_hash_ensure(result);
      vec_push(ctx->all_types, result);
      return result;
    }

    /* Concrete length */
    if (elem == type->impl->array.element) return type;
    semantic_type_t result = semantic_type_create_array(
        ctx->allocator, elem, type->impl->array.length, type->impl->array.length_param_idx);
    type_hash_ensure(result);
    vec_push(ctx->all_types, result);
    return result;
  }

  case TYPE_QUALIFIER: {
    semantic_type_t base = _substitute_type(ctx, type->impl->qualifier.base, type_args);
    if (base == type->impl->qualifier.base) return type;
    semantic_type_t result = semantic_type_create_qualifier(ctx->allocator, base,
        type->impl->qualifier.is_const, type->impl->qualifier.is_volatile);
    type_hash_ensure(result);
    vec_push(ctx->all_types, result);
    return result;
  }

  case TYPE_FUNCTION: {
    bool changed = false;
    vec_init_t vi = {.auto_dispose = false};
    vec_t new_params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
    size_t pcount = vec_get_size(type->impl->function.params);
    for (size_t i = 0; i < pcount; i++) {
      semantic_type_t p = (semantic_type_t)vec_get(type->impl->function.params, i);
      semantic_type_t new_p = _substitute_type(ctx, p, type_args);
      /* If substitution produced a TYPE_GENERIC_PACK, expand it into individual types */
      if (new_p && new_p->impl->kind == TYPE_GENERIC_PACK) {
        vec_t expanded = new_p->impl->generic_pack.expanded_types;
        size_t ecount = expanded ? vec_get_size(expanded) : 0;
        for (size_t j = 0; j < ecount; j++) {
          vec_push(new_params, (semantic_type_t)vec_get(expanded, j));
        }
        changed = true;
      } else {
        vec_push(new_params, new_p);
        if (new_p != p) changed = true;
      }
    }
    semantic_type_t new_ret = _substitute_type(ctx, type->impl->function.return_type, type_args);
    if (new_ret != type->impl->function.return_type) changed = true;

    if (!changed) {
      allocator_free(ctx->allocator, &new_params);
      return type;
    }
    semantic_type_t result = semantic_type_create_function(ctx->allocator, new_ret, new_params,
        type->impl->function.is_variadic);
    type_hash_ensure(result);
    vec_push(ctx->all_types, result);
    return result;
  }

  case TYPE_GENERIC_INSTANCE: {
    /* Substitute type args, then delegate to _instantiate_type for proper
       field creation, method copying, and cache dedup */
    semantic_type_t tmpl = type->impl->generic_instance.generic_template;
    vec_t tmpl_args = type->impl->generic_instance.type_args;
    bool changed = false;
    vec_init_t vi = {.auto_dispose = false};
    vec_t new_args = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
    size_t acount = tmpl_args ? vec_get_size(tmpl_args) : 0;
    for (size_t i = 0; i < acount; i++) {
      semantic_type_t arg = (semantic_type_t)vec_get(tmpl_args, i);
      semantic_type_t new_arg = _substitute_type(ctx, arg, type_args);
      vec_push(new_args, new_arg);
      if (new_arg != arg) changed = true;
    }

    if (!changed) {
      allocator_free(ctx->allocator, &new_args);
      return type;
    }

    /* _instantiate_type takes ownership of new_args (stores or frees on cache hit) */
    return _instantiate_type(ctx, tmpl, new_args, NULL);
  }

  case TYPE_STRUCT:
  case TYPE_UNION:
  case TYPE_CUNION:
    /* Cannot substitute a struct/union type in-place (would corrupt the template).
       Return as-is; the caller should use _instantiate_struct_fields for
       creating substituted field copies. */
    return type;

  case TYPE_TUPLE: {
    /* Substitute element types, expanding packs */
    vec_t elems = type->impl->tuple.element_types;
    size_t ecount = elems ? vec_get_size(elems) : 0;
    bool changed = false;
    vec_init_t vi = {.auto_dispose = false};
    vec_t new_elems = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
    for (size_t i = 0; i < ecount; i++) {
      semantic_type_t e = (semantic_type_t)vec_get(elems, i);
      semantic_type_t new_e = _substitute_type(ctx, e, type_args);
      /* If substitution produced a TYPE_GENERIC_PACK, expand it into individual types */
      if (new_e && new_e->impl->kind == TYPE_GENERIC_PACK) {
        vec_t expanded = new_e->impl->generic_pack.expanded_types;
        size_t ecount2 = expanded ? vec_get_size(expanded) : 0;
        for (size_t j = 0; j < ecount2; j++) {
          vec_push(new_elems, (semantic_type_t)vec_get(expanded, j));
        }
        changed = true;
      } else {
        vec_push(new_elems, new_e);
        if (new_e != e) changed = true;
      }
    }
    if (!changed) {
      allocator_free(ctx->allocator, &new_elems);
      return type;
    }
    semantic_type_t result = semantic_type_create_tuple(ctx->allocator, new_elems);
    type_hash_ensure(result);
    type_layout_compute(result, 8);
    vec_push(ctx->all_types, result);
    return result;
  }

  case TYPE_PACK_INDEX: {
    /* Args[N] — index into a pack parameter's expanded_types */
    size_t pack_param_idx = type->impl->pack_index.pack_param_idx;
    size_t index_param_idx = type->impl->pack_index.index_param_idx;
    if (type_args && index_param_idx < vec_get_size(type_args) &&
        pack_param_idx < vec_get_size(type_args)) {
      semantic_type_t index_type = (semantic_type_t)vec_get(type_args, index_param_idx);
      semantic_type_t pack_type = (semantic_type_t)vec_get(type_args, pack_param_idx);
      if (pack_type && pack_type->impl->kind == TYPE_GENERIC_PACK &&
          pack_type->impl->generic_pack.expanded_types &&
          index_type && index_type->impl->kind == TYPE_GENERIC_VALUE) {
        size_t idx = (size_t)comptime_value_as_u64(
            index_type->impl->generic_value.value);
        vec_t expanded = pack_type->impl->generic_pack.expanded_types;
        if (idx < vec_get_size(expanded)) {
          return (semantic_type_t)vec_get(expanded, idx);
        }
      }
    }
    return type; /* Return as-is — caller resolves via eval_call if needed */
  }

  case TYPE_GENERIC_VALUE:
    /* Already a concrete compile-time value, return as-is */
    return type;

  default:
    return type;
  }
}

bool _check_constraint(checker_t ctx, semantic_type_t type_arg,
                       semantic_type_t constraint, node_t arg_expr) {
  if (!constraint) return true;
  if (!type_arg || type_arg->impl->kind == TYPE_ERROR) return false;

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

  case TYPE_TUPLE: {
    /* Tuple constraint: T extends <?> means T must be a tuple type.
       If constraint has element types, check them pairwise (wildcard skips). */
    if (type_arg->impl->kind != TYPE_TUPLE) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                           "type does not satisfy constraint: expected tuple type");
      ctx->error_count++;
      return false;
    }
    vec_t c_elems = constraint->impl->tuple.element_types;
    vec_t a_elems = type_arg->impl->tuple.element_types;
    size_t cec = c_elems ? vec_get_size(c_elems) : 0;
    size_t aec = a_elems ? vec_get_size(a_elems) : 0;
    if (aec < cec) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                           "type does not satisfy constraint: tuple element count mismatch");
      ctx->error_count++;
      return false;
    }
    for (size_t i = 0; i < cec; i++) {
      semantic_type_t ce = (semantic_type_t)vec_get(c_elems, i);
      semantic_type_t ae = (semantic_type_t)vec_get(a_elems, i);
      if (ce->impl->kind == TYPE_WILDCARD) continue;
      if (!_check_constraint(ctx, ae, ce, arg_expr))
        return false;
    }
    return true;
  }

  case TYPE_WILDCARD: {
    /* <?> (is_tuple=true): type_arg must be a tuple type.
       ? (is_tuple=false): matches any type. */
    if (constraint->impl->wildcard.is_tuple) {
      if (type_arg->impl->kind != TYPE_TUPLE) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: expected tuple type");
        ctx->error_count++;
        return false;
      }
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

    /* Value generic param: validate the value type */
    if (gp->value_type) {
      semantic_type_t resolved_vt = resolver_resolve_type(ctx, gp->value_type);
      if (i < vec_get_size(type_args)) {
        semantic_type_t ta = (semantic_type_t)vec_get(type_args, i);
        if (ta && ta->impl->kind == TYPE_GENERIC_VALUE) {
          comptime_value_t cv = ta->impl->generic_value.value;
          if (cv && cv->type && resolved_vt) {
            if (!semantic_type_can_implicit_convert(cv->type, resolved_vt)) {
              diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
                  "value generic argument type '%s' is not compatible with declared type '%s'",
                  semantic_type_get_name(cv->type) ? semantic_type_get_name(cv->type) : "<anonymous>",
                  semantic_type_get_name(resolved_vt) ? semantic_type_get_name(resolved_vt) : "<anonymous>");
              ctx->error_count++;
              all_ok = false;
            }
          }
        } else if (ta && ta->impl->kind != TYPE_GENERIC_VALUE) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, expr->location,
              "expected a compile-time value for value generic parameter '%s'",
              _checker_ident_str(gp->name));
          ctx->error_count++;
          all_ok = false;
        }
      }
      continue; /* Skip extends constraint check for value params */
    }

    if (!gp->constraint) continue;

    if (i >= vec_get_size(type_args)) continue;
    semantic_type_t ta = (semantic_type_t)vec_get(type_args, i);
    if (!ta) continue;

    semantic_type_t constraint_type = resolver_resolve_type(ctx, gp->constraint);
    if (!constraint_type || constraint_type->impl->kind == TYPE_ERROR) continue;

    if (gp->is_rest) {
      /* Pack parameter: check constraint against each expanded type */
      if (ta->impl->kind == TYPE_GENERIC_PACK) {
        size_t ecount = vec_get_size(ta->impl->generic_pack.expanded_types);
        for (size_t j = 0; j < ecount; j++) {
          semantic_type_t et = (semantic_type_t)vec_get(ta->impl->generic_pack.expanded_types, j);
          if (!_check_constraint(ctx, et, constraint_type, expr)) {
            all_ok = false;
          }
        }
      }
    } else {
      if (!_check_constraint(ctx, ta, constraint_type, expr)) {
        all_ok = false;
      }
    }
  }
  return all_ok;
}

vec_t _resolve_generic_type_args(checker_t ctx, vec_t arg_exprs,
                                  vec_t generic_params) {
  if (!arg_exprs) return NULL;
  size_t acount = vec_get_size(arg_exprs);
  if (acount == 0) return NULL;

  vec_init_t vi = {.auto_dispose = false};
  vec_t type_args = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);

  for (size_t i = 0; i < acount; i++) {
    node_t arg = (node_t)vec_get(arg_exprs, i);

    /* Check if this position corresponds to a value generic param */
    bool is_value_param = false;
    semantic_type_t value_type = NULL;
    if (generic_params && i < vec_get_size(generic_params)) {
      cubec_generic_param_t gp = (cubec_generic_param_t)vec_get(generic_params, i);
      if (gp && gp->value_type) {
        is_value_param = true;
        value_type = resolver_resolve_type(ctx, gp->value_type);
      }
    }

    if (is_value_param) {
      /* Resolve as a compile-time constant value */
      if (arg->kind == CUBEC_NODE_LITERAL_NUMERIC) {
        cubec_literal_numeric_t num = (cubec_literal_numeric_t)arg;
        uint64_t uval = strtoull(string_get(num->value), NULL, 10);
        int64_t sval = (int64_t)uval;
        /* Use the literal's own type (i32, u64, etc.), not the constraint type */
        semantic_type_t lit_type = _check_literal_numeric(ctx, arg);
        comptime_value_t cv = comptime_value_create_int(
            ctx->allocator, sval, uval, 64, false, lit_type);
        semantic_type_t gv = semantic_type_create_generic_value(ctx->allocator, cv);
        type_hash_ensure(gv);
        vec_push(ctx->all_types, gv);
        vec_push(type_args, gv);
        continue;
      }
      if (arg->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
        const char *name = _checker_ident_str(arg);
        if (name && strcmp(name, "true") == 0) {
          comptime_value_t cv = comptime_value_create_bool(
              ctx->allocator, true, ctx->builtin_bool);
          semantic_type_t gv = semantic_type_create_generic_value(ctx->allocator, cv);
          type_hash_ensure(gv);
          vec_push(ctx->all_types, gv);
          vec_push(type_args, gv);
          continue;
        }
        if (name && strcmp(name, "false") == 0) {
          comptime_value_t cv = comptime_value_create_bool(
              ctx->allocator, false, ctx->builtin_bool);
          semantic_type_t gv = semantic_type_create_generic_value(ctx->allocator, cv);
          type_hash_ensure(gv);
          vec_push(ctx->all_types, gv);
          vec_push(type_args, gv);
          continue;
        }
      }
      /* General comptime eval — supports arbitrary compile-time expressions */
      {
        extern comptime_value_t _comptime_eval_expr(comptime_eval_t, struct checker *, node_t);
        comptime_eval_t eval = comptime_eval_create(ctx->allocator);
        if (!eval) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                               arg->location,
                               "failed to create comptime evaluator");
          ctx->error_count++;
          allocator_free(ctx->allocator, &type_args);
          return NULL;
        }
        comptime_value_t cv = _comptime_eval_expr(eval, ctx, arg);
        if (cv && cv->kind != COMPTIME_VALUE_ERROR) {
          /* Clone value into checker's allocator before disposing eval,
             because comptime_eval_dispose frees all values allocated
             through eval's comptime_allocator. */
          comptime_value_t cloned = comptime_value_clone(ctx->allocator, cv);
          comptime_eval_dispose(eval);
          allocator_free(ctx->allocator, &eval);
          semantic_type_t gv = semantic_type_create_generic_value(ctx->allocator, cloned);
          type_hash_ensure(gv);
          vec_push(ctx->all_types, gv);
          vec_push(type_args, gv);
          continue;
        }
        comptime_eval_dispose(eval);
        allocator_free(ctx->allocator, &eval);
      }
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           ((node_t)vec_get(arg_exprs, i))->location,
                           "value generic argument must be a compile-time constant");
      ctx->error_count++;
      allocator_free(ctx->allocator, &type_args);
      return NULL;
    }

    /* Existing type resolution path */
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
    nf->field.type = _substitute_type(ctx, f->field.type, type_args);
    vec_push(inst->impl->generic_instance.fields, nf);
  }
  type_layout_compute(inst, 8);
}

semantic_type_t _instantiate_type(checker_t ctx, semantic_type_t template_type,
                                   vec_t type_args, node_t instantiation_expr) {
  const char *name = template_type->name;
  if (!name) name = "<anonymous>";
  /* Check cache */
  semantic_type_t cached = _cache_lookup(ctx, name, type_args);
  if (cached) {
    allocator_free(ctx->allocator, &type_args);
    return cached;
  }

  /* Create the specialized type as a GENERIC_INSTANCE */
  semantic_type_t inst = NULL;

  /* Copy structural info from template based on kind */
  enum type_kind tkind = template_type->impl->kind;

  /* Special handling for TYPE_TUPLE: create a native TYPE_TUPLE instead of
     GENERIC_INSTANCE. type_args contains a TYPE_GENERIC_PACK with expanded_types. */
  if (tkind == TYPE_TUPLE) {
    vec_t field_types = NULL;
    size_t tacount = type_args ? vec_get_size(type_args) : 0;
    for (size_t i = 0; i < tacount; i++) {
      semantic_type_t ta = (semantic_type_t)vec_get(type_args, i);
      if (ta && ta->impl && ta->impl->kind == TYPE_GENERIC_PACK) {
        field_types = ta->impl->generic_pack.expanded_types;
        break;
      }
    }
    /* Build element_types vec from pack */
    vec_init_t evi = {.auto_dispose = false};
    vec_t elem_types = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &evi);
    if (field_types) {
      size_t fcount = vec_get_size(field_types);
      for (size_t i = 0; i < fcount; i++) {
        vec_push(elem_types, vec_get(field_types, i));
      }
    }
    inst = semantic_type_create_tuple(ctx->allocator, elem_types);
    vec_push(ctx->all_types, inst);
    type_hash_ensure(inst);
    type_layout_compute(inst, 8);
    /* Cache the result */
    _cache_insert(ctx, name, type_args, inst);
    return inst;
  }

  /* For non-TUPLE types, create a GENERIC_INSTANCE */
  inst = semantic_type_create_generic_instance(
      ctx->allocator, template_type, type_args);
  vec_push(ctx->all_types, inst);
  type_hash_ensure(inst);

  if (tkind == TYPE_STRUCT || tkind == TYPE_UNION || tkind == TYPE_CUNION) {
    _instantiate_struct_fields(ctx, inst, template_type->impl->struct_type.fields, type_args);
  }

  /* Copy method lists from template (free the init-created vecs first) */
  allocator_free(ctx->allocator, &inst->instance_methods);
  allocator_free(ctx->allocator, &inst->static_methods);
  inst->instance_methods = _copy_symbol_vec(ctx,template_type->instance_methods);
  inst->static_methods = _copy_symbol_vec(ctx, template_type->static_methods);

  /* Cache the result */
  _cache_insert(ctx, name, type_args, inst);

  return inst;
}

semantic_type_t _instantiate_function(checker_t ctx, struct symbol *func_sym,
                                      vec_t type_args, node_t instantiation_expr) {
  const char *name = func_sym->name;
  semantic_type_t func_type = func_sym->function.type;
  if (!func_type) return ctx->error_type;

  /* Check cache */
  semantic_type_t cached = _cache_lookup(ctx, name, type_args);
  if (cached) {
    allocator_free(ctx->allocator, &type_args);
    return cached;
  }

  /* Substitute generic params in the entire function type.
     Using _substitute_type on the whole type handles pack expansion
     correctly (TYPE_FUNCTION branch expands TYPE_GENERIC_PACK params). */
  semantic_type_t inst_type = _substitute_type(ctx, func_type, type_args);

  /* If nothing changed (no generic params), return original */
  if (inst_type == func_type) {
    allocator_free(ctx->allocator, &type_args);
    return func_type;
  }

  /* Cache */
  _cache_insert(ctx, name, type_args, inst_type);

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
