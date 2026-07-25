#include "engine/context.h"
#include "engine/checker_type_util.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "engine/comptime_value.h"
#include "engine/comptime_eval.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/strmap.h"
#include "core/vec.h"
#include "cubec/node.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_numeric.h"
#include "cubec/struct_field.h"
#include "cubec/union_field.h"
#include "cubec/enum_item.h"
#include "cubec/generic_param.h"
#include <string.h>

/* ===== identifier helper ===== */

const char *_checker_ident_str(node_t id_node) {
  if (!id_node) return NULL;
  cubec_literal_identifier_t id = (cubec_literal_identifier_t)id_node;
  return string_get(id->value);
}

/* ===== type bindings helper ===== */

/**
 * @brief Build a strmap_t (name → semantic_type_t) from a vec_t type_args
 *        using param names from the generic_params vec.
 *
 * This is a temporary bridge: callers still produce vec_t type_args
 * (positional), but the new data model uses name-based strmap_t.
 * The full rewrite of _substitute_type / _instantiate_type / _infer_type_args_from_call
 * will eliminate this helper by producing strmap_t directly.
 */
static strmap_t _type_bindings_from_vec(allocator_t allocator, vec_t generic_params,
                                         vec_t type_args) {
  strmap_init_t si = {.value_auto_dispose = false};
  strmap_t bindings = (strmap_t)allocator_create(allocator, &g_strmap_type, &si);
  if (!generic_params || !type_args) return bindings;
  size_t gcount = vec_get_size(generic_params);
  size_t tacount = vec_get_size(type_args);
  for (size_t i = 0; i < gcount && i < tacount; i++) {
    node_t gp_node = (node_t)vec_get(generic_params, i);
    if (!gp_node || gp_node->kind != CUBEC_NODE_GENERIC_PARAM) continue;
    cubec_generic_param_t gp = (cubec_generic_param_t)gp_node;
    const char *name = _checker_ident_str(gp->name);
    if (!name) continue;
    semantic_type_t ta = (semantic_type_t)vec_get(type_args, i);
    if (ta) strmap_insert(bindings, name, ta);
  }
  return bindings;
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

bool _is_comparable_type(semantic_type_t t) {
  if (!t || !t->impl) return false;
  /* Strip all qualifiers (const volatile T → T) */
  semantic_type_t unq = t;
  while (unq && unq->impl && unq->impl->kind == TYPE_QUALIFIER)
    unq = unq->impl->qualifier.base;
  if (!unq || !unq->impl) return false;
  enum type_kind k = unq->impl->kind;
  return (k >= TYPE_I8 && k <= TYPE_U64) ||
         (k >= TYPE_F16 && k <= TYPE_F64) ||
         k == TYPE_BOOL || k == TYPE_CHAR || k == TYPE_ENUM || k == TYPE_STR ||
         k == TYPE_TYPE;
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

semantic_type_t _common_type(context_t ctx, semantic_type_t a,
                             semantic_type_t b) {
  if (!a || !b) return ctx->error_type;
  if (semantic_type_equals(a, b)) return a;
  /* int + float is NOT implicitly promoted (design: no int→float conversion) */
  /* float widening */
  if (a->impl->size >= b->impl->size) return a;
  return b;
}

/* ===== struct/union/enum field resolution — unified for global and local ===== */

void _resolve_struct_fields(context_t ctx, semantic_type_t t, vec_t members) {
  vec_init_t fvi = {.auto_dispose = true};
  vec_t fields = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &fvi);
  if (members) {
    size_t mcount = vec_get_size(members);
    for (size_t i = 0; i < mcount; i++) {
      node_t m = (node_t)vec_get(members, i);
      if (!m) continue;
      if (m->kind != CUBEC_NODE_STRUCT_FIELD) continue;
      cubec_struct_field_t sf = (cubec_struct_field_t)m;
      const char *fname = _checker_ident_str(sf->name);
      struct symbol *fsym = symbol_create(ctx->allocator, fname,
                                          SYMBOL_FIELD, sf->super.location);
      if (sf->type)
        fsym->field.type = resolver_resolve_type(ctx, sf->type);
      fsym->field.index = i;
      fsym->field.is_pub = sf->is_pub;
      vec_push(fields, fsym);
    }
  }
  t->impl->struct_type.fields = fields;
}

void _resolve_union_fields(context_t ctx, semantic_type_t t, vec_t members) {
  vec_init_t fvi = {.auto_dispose = true};
  vec_t fields = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &fvi);
  if (members) {
    size_t mcount = vec_get_size(members);
    for (size_t i = 0; i < mcount; i++) {
      node_t m = (node_t)vec_get(members, i);
      if (m->kind != CUBEC_NODE_UNION_FIELD) continue;
      cubec_union_field_t uf = (cubec_union_field_t)m;
      const char *fname = _checker_ident_str(uf->name);
      struct symbol *fsym = symbol_create(ctx->allocator, fname,
                                          SYMBOL_FIELD, uf->super.location);
      if (uf->type)
        fsym->field.type = resolver_resolve_type(ctx, uf->type);
      fsym->field.index = i;
      vec_push(fields, fsym);
    }
  }
  t->impl->struct_type.fields = fields;
}

void _resolve_enum_items(context_t ctx, semantic_type_t t, vec_t items) {
  vec_init_t ivi = {.auto_dispose = true};
  t->impl->enum_type.items = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &ivi);
  if (!items) return;
  size_t count = vec_get_size(items);
  long long auto_val = 0;
  for (size_t i = 0; i < count; i++) {
    node_t item_node = (node_t)vec_get(items, i);
    if (!item_node || item_node->kind != CUBEC_NODE_ENUM_ITEM) continue;

    cubec_enum_item_t item = (cubec_enum_item_t)item_node;
    const char *iname = _checker_ident_str(item->name);
    struct symbol *isym = symbol_create(ctx->allocator, iname,
                                        SYMBOL_ENUM_ITEM, item->super.location);
    isym->enum_item.owning_type = t;
    if (item->value) {
      if (item->value->kind == CUBEC_NODE_LITERAL_NUMERIC) {
        cubec_literal_numeric_t num = (cubec_literal_numeric_t)item->value;
        const char *numstr = string_get(num->value);
        isym->enum_item.value = numstr ? atoll(numstr) : auto_val;
      } else {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             item->value->location,
                             "enum value must be a compile-time integer literal");
        ctx->error_count++;
        isym->enum_item.value = auto_val;
      }
      auto_val = isym->enum_item.value + 1;
    } else {
      isym->enum_item.value = auto_val++;
    }
    vec_push(t->impl->enum_type.items, isym);
  }
}

/* ===== generic instantiation helpers ===== */

char *_generic_instance_cache_key(context_t ctx, const char *template_name,
                                   strmap_t type_bindings) {
  size_t len = strlen(template_name);
  /* Estimate key length from bindings — must account for pack expanded_types
     which generate multiple '#hash' entries per binding */
  size_t entry_count = 0;
  if (type_bindings) {
    strmap_iter_t iter = strmap_iter_first(type_bindings);
    const char *bname = NULL;
    while ((bname = strmap_iter_next(&iter)) != NULL) {
      semantic_type_t t = (semantic_type_t)strmap_find(type_bindings, bname);
      if (!t) continue;
      if (t->impl->kind == TYPE_GENERIC_PACK) {
        entry_count += vec_get_size(t->impl->generic_pack.expanded_types);
      } else {
        entry_count++;
      }
    }
  }
  len += entry_count * (1 + 20); /* '#' + hash per entry */
  len += 64; /* extra safety margin */
  char *key = (char *)allocator_alloc(ctx->allocator, len + 1);
  size_t pos = 0;
  memcpy(key + pos, template_name, strlen(template_name));
  pos += strlen(template_name);
  if (type_bindings) {
    strmap_iter_t iter = strmap_iter_first(type_bindings);
    const char *bname = NULL;
    while ((bname = strmap_iter_next(&iter)) != NULL) {
      semantic_type_t t = (semantic_type_t)strmap_find(type_bindings, bname);
      if (!t) continue;
      if (t->impl->kind == TYPE_GENERIC_PACK) {
        size_t ecount = vec_get_size(t->impl->generic_pack.expanded_types);
        for (size_t j = 0; j < ecount; j++) {
          semantic_type_t et = (semantic_type_t)vec_get(t->impl->generic_pack.expanded_types, j);
          if (!et) continue;
          type_hash_ensure(et);
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
        type_hash_ensure(t);
        key[pos++] = '#';
        pos += snprintf(key + pos, len + 1 - pos, "%zu", t->impl->hash);
      }
    }
  }
  key[pos] = '\0';
  return key;
}

static semantic_type_t _cache_lookup(context_t ctx, const char *name,
                                     strmap_t type_bindings) {
  char *key = _generic_instance_cache_key(ctx, name, type_bindings);
  void *found = strmap_find(ctx->type_impl_cache, key);
  allocator_free(ctx->allocator, &key);
  return found ? (semantic_type_t)found : NULL;
}

static void _cache_insert(context_t ctx, const char *name,
                           strmap_t type_bindings, semantic_type_t type) {
  char *key = _generic_instance_cache_key(ctx, name, type_bindings);
  strmap_insert(ctx->type_impl_cache, key, type);
  allocator_free(ctx->allocator, &key);
}

/* ===== type substitution ===== */

/* Forward declaration — needed because _substitute_type delegates to _instantiate_type */
semantic_type_t _substitute_type(context_t ctx, semantic_type_t type,
                                   strmap_t type_bindings);

/* Forward declaration — needed because _substitute_type delegates to _instantiate_type */
semantic_type_t _substitute_type(context_t ctx, semantic_type_t type,
                                   strmap_t type_bindings);

static int _subst_depth = 0;

semantic_type_t _substitute_type(context_t ctx, semantic_type_t type,
                                   strmap_t type_bindings) {
  if (!type || !type->impl) return type;
  _subst_depth++;
  if (_subst_depth > 500) {
    _subst_depth--;
    return type;
  }

  semantic_type_t result = type; /* default: return unchanged */

  switch (type->impl->kind) {
  case TYPE_GENERIC_PARAM: {
    const char *name = type->impl->generic_param.name;
    if (type_bindings && name) {
      semantic_type_t replacement = (semantic_type_t)strmap_find(type_bindings, name);
      if (replacement) { result = replacement; break; }
    }
    break;
  }

  case TYPE_GENERIC_PACK: {
    const char *name = type->impl->generic_pack.name;
    if (type_bindings && name) {
      semantic_type_t replacement = (semantic_type_t)strmap_find(type_bindings, name);
      if (replacement) { result = replacement; break; }
    }
    break;
  }

  case TYPE_POINTER: {
    semantic_type_t inner = _substitute_type(ctx, type->impl->pointer.pointee, type_bindings);
    if (inner != type->impl->pointer.pointee) {
      result = semantic_type_create_pointer(ctx->allocator, inner);
      type_hash_ensure(result);
      vec_push(ctx->all_types, result);
    }
    break;
  }

  case TYPE_SLICE: {
    semantic_type_t elem = _substitute_type(ctx, type->impl->slice.element, type_bindings);
    if (elem != type->impl->slice.element) {
      result = semantic_type_create_slice(ctx->allocator, elem);
      type_hash_ensure(result);
      vec_push(ctx->all_types, result);
    }
    break;
  }

  case TYPE_ARRAY: {
    semantic_type_t elem = _substitute_type(ctx, type->impl->array.element, type_bindings);
    const char *length_param_name = type->impl->array.length_param_name;

    if (length_param_name && type_bindings) {
      semantic_type_t replacement = (semantic_type_t)strmap_find(type_bindings, length_param_name);
      if (replacement && replacement->impl->kind == TYPE_GENERIC_VALUE) {
        size_t concrete_len =
            (size_t)comptime_value_as_u64(replacement->impl->generic_value.value);
        if (elem != type->impl->array.element || concrete_len != type->impl->array.length) {
          result = semantic_type_create_array(
              ctx->allocator, elem, concrete_len, NULL);
          type_hash_ensure(result);
          vec_push(ctx->all_types, result);
        }
        break;
      }
      if (elem != type->impl->array.element) {
        result = semantic_type_create_array(
            ctx->allocator, elem, 0, length_param_name);
        type_hash_ensure(result);
        vec_push(ctx->all_types, result);
      }
      break;
    }

    if (elem != type->impl->array.element) {
      result = semantic_type_create_array(
          ctx->allocator, elem, type->impl->array.length, type->impl->array.length_param_name);
      type_hash_ensure(result);
      vec_push(ctx->all_types, result);
    }
    break;
  }

  case TYPE_QUALIFIER: {
    semantic_type_t base = _substitute_type(ctx, type->impl->qualifier.base, type_bindings);
    if (base != type->impl->qualifier.base) {
      result = semantic_type_create_qualifier(ctx->allocator, base,
          type->impl->qualifier.is_const, type->impl->qualifier.is_volatile);
      type_hash_ensure(result);
      vec_push(ctx->all_types, result);
    }
    break;
  }

  case TYPE_FUNCTION: {
    bool changed = false;
    vec_init_t vi = {.auto_dispose = false};
    vec_t new_params = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
    size_t pcount = vec_get_size(type->impl->function.params);
    for (size_t i = 0; i < pcount; i++) {
      semantic_type_t p = (semantic_type_t)vec_get(type->impl->function.params, i);
      semantic_type_t new_p = _substitute_type(ctx, p, type_bindings);
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
    semantic_type_t new_ret = _substitute_type(ctx, type->impl->function.return_type, type_bindings);
    if (new_ret != type->impl->function.return_type) changed = true;

    if (!changed) {
      allocator_free(ctx->allocator, &new_params);
    } else {
      result = semantic_type_create_function(ctx->allocator, new_ret, new_params,
          type->impl->function.is_variadic);
      type_hash_ensure(result);
      vec_push(ctx->all_types, result);
    }
    break;
  }

  case TYPE_GENERIC_INSTANCE: {
    /* Substitute type bindings, then delegate to _instantiate_type for proper
       field creation, method copying, and cache dedup */
    semantic_type_t tmpl = type->impl->generic_instance.generic_template;
    strmap_t tmpl_bindings = type->impl->generic_instance.type_bindings;
    bool changed = false;
    strmap_init_t si = {.value_auto_dispose = false};
    strmap_t new_bindings = (strmap_t)allocator_create(ctx->allocator, &g_strmap_type, &si);
    if (tmpl_bindings) {
      strmap_iter_t iter = strmap_iter_first(tmpl_bindings);
      const char *bname = NULL;
      while ((bname = strmap_iter_next(&iter)) != NULL) {
        semantic_type_t arg = (semantic_type_t)strmap_find(tmpl_bindings, bname);
        semantic_type_t new_arg = _substitute_type(ctx, arg, type_bindings);
        strmap_insert(new_bindings, bname, new_arg);
        if (new_arg != arg) changed = true;
      }
    }

    if (!changed) {
      allocator_free(ctx->allocator, &new_bindings);
    } else {
      /* _instantiate_type takes ownership of new_bindings */
      result = _instantiate_type(ctx, tmpl, new_bindings, NULL);
    }
    break;
  }

  case TYPE_STRUCT:
  case TYPE_UNION:
  case TYPE_CUNION:
    /* Cannot substitute a struct/union type in-place (would corrupt the template).
       Return as-is; the caller should use _instantiate_struct_fields for
       creating substituted field copies. */
    break;

  case TYPE_TUPLE: {
    /* Substitute element types, expanding packs */
    vec_t elems = type->impl->tuple.element_types;
    size_t ecount = elems ? vec_get_size(elems) : 0;
    bool changed = false;
    vec_init_t tvi = {.auto_dispose = false};
    vec_t new_elems = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &tvi);
    for (size_t i = 0; i < ecount; i++) {
      semantic_type_t e = (semantic_type_t)vec_get(elems, i);
      semantic_type_t new_e = _substitute_type(ctx, e, type_bindings);
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
    } else {
      result = semantic_type_create_tuple(ctx->allocator, new_elems);
      type_hash_ensure(result);
      type_layout_compute(result, 8);
      vec_push(ctx->all_types, result);
    }
    break;
  }

  case TYPE_PACK_INDEX: {
    /* Args[N] — index into a pack parameter's expanded_types using name-based lookup */
    const char *pack_name = type->impl->pack_index.pack_name;
    const char *index_param_name = type->impl->pack_index.index_param_name;
    if (type_bindings && index_param_name && pack_name) {
      semantic_type_t index_type = (semantic_type_t)strmap_find(type_bindings, index_param_name);
      semantic_type_t pack_type = (semantic_type_t)strmap_find(type_bindings, pack_name);
      if (pack_type && pack_type->impl->kind == TYPE_GENERIC_PACK &&
          pack_type->impl->generic_pack.expanded_types &&
          index_type && index_type->impl->kind == TYPE_GENERIC_VALUE) {
        size_t idx = (size_t)comptime_value_as_u64(
            index_type->impl->generic_value.value);
        vec_t expanded = pack_type->impl->generic_pack.expanded_types;
        if (idx < vec_get_size(expanded)) {
          result = (semantic_type_t)vec_get(expanded, idx);
          break;
        }
      }
    }
    break; /* Return as-is — caller resolves via eval_call if needed */
  }

  case TYPE_GENERIC_VALUE:
    /* Already a concrete compile-time value, return as-is */
    break;

  default:
    break;
  }

  _subst_depth--;
  return result;
}

/* Core constraint checking logic. When silent=true, no diagnostics are
   emitted — used by _check_constraint_silent for extends expression eval. */
static bool _check_constraint_impl(context_t ctx, semantic_type_t type_arg,
                                   semantic_type_t constraint, node_t arg_expr,
                                   bool silent) {
  if (!constraint) return true;
  if (!type_arg || type_arg->impl->kind == TYPE_ERROR) return false;

  switch (constraint->impl->kind) {
  case TYPE_INTERFACE: {
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
        if (!silent) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                               "type does not satisfy constraint: missing method '%s'",
                               req->name);
          ctx->error_count++;
        }
        return false;
      }
    }
    return true;
  }

  case TYPE_GENERIC_INSTANCE: {
    if (type_arg->impl->kind != TYPE_GENERIC_INSTANCE) {
      if (!silent) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy generic constraint: not a generic instance");
        ctx->error_count++;
      }
      return false;
    }

    if (type_arg->impl->generic_instance.generic_template !=
        constraint->impl->generic_instance.generic_template) {
      if (!silent) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: wrong generic template");
        ctx->error_count++;
      }
      return false;
    }

    strmap_t a_bindings = type_arg->impl->generic_instance.type_bindings;
    strmap_t c_bindings = constraint->impl->generic_instance.type_bindings;
    size_t ac = a_bindings ? strmap_get_size(a_bindings) : 0;
    size_t cc = c_bindings ? strmap_get_size(c_bindings) : 0;
    if (ac != cc) {
      if (!silent) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: type arg count mismatch");
        ctx->error_count++;
      }
      return false;
    }

    /* Iterate constraint bindings and check each against the arg binding */
    if (c_bindings) {
      strmap_iter_t iter = strmap_iter_first(c_bindings);
      const char *bname = NULL;
      while ((bname = strmap_iter_next(&iter)) != NULL) {
        semantic_type_t ca = (semantic_type_t)strmap_find(c_bindings, bname);
        semantic_type_t aa = a_bindings ? (semantic_type_t)strmap_find(a_bindings, bname) : NULL;
        if (ca && ca->impl->kind == TYPE_WILDCARD) continue;
        if (!_check_constraint_impl(ctx, aa, ca, arg_expr, silent)) return false;
      }
    }
    return true;
  }

  case TYPE_POINTER: {
    if (type_arg->impl->kind != TYPE_POINTER) {
      if (!silent) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: expected pointer type");
        ctx->error_count++;
      }
      return false;
    }
    return _check_constraint_impl(ctx, type_arg->impl->pointer.pointee,
                                  constraint->impl->pointer.pointee, arg_expr, silent);
  }

  case TYPE_SLICE: {
    if (type_arg->impl->kind != TYPE_SLICE) {
      if (!silent) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: expected slice type");
        ctx->error_count++;
      }
      return false;
    }
    return _check_constraint_impl(ctx, type_arg->impl->slice.element,
                                  constraint->impl->slice.element, arg_expr, silent);
  }

  case TYPE_ARRAY: {
    if (type_arg->impl->kind != TYPE_ARRAY) {
      if (!silent) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: expected array type");
        ctx->error_count++;
      }
      return false;
    }
    if (constraint->impl->array.length != type_arg->impl->array.length &&
        constraint->impl->array.length != 0) {
      if (!silent) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: array length mismatch");
        ctx->error_count++;
      }
      return false;
    }
    return _check_constraint_impl(ctx, type_arg->impl->array.element,
                                  constraint->impl->array.element, arg_expr, silent);
  }

  case TYPE_STRUCT:
  case TYPE_UNION:
  case TYPE_CUNION: {
    if (type_arg->impl->kind != constraint->impl->kind) {
      if (!silent) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: type kind mismatch");
        ctx->error_count++;
      }
      return false;
    }
    vec_t c_fields = constraint->impl->struct_type.fields;
    vec_t a_fields = type_arg->impl->struct_type.fields;
    size_t cfc = c_fields ? vec_get_size(c_fields) : 0;
    size_t afc = a_fields ? vec_get_size(a_fields) : 0;
    if (afc < cfc) {
      if (!silent) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: missing fields");
        ctx->error_count++;
      }
      return false;
    }
    for (size_t i = 0; i < cfc; i++) {
      struct symbol *cf = (struct symbol *)vec_get(c_fields, i);
      struct symbol *af = (struct symbol *)vec_get(a_fields, i);
      if (!cf || !cf->name) continue;
      if (!af || !af->name || strcmp(cf->name, af->name) != 0) {
        if (!silent) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                               "type does not satisfy constraint: field '%s' mismatch",
                               cf->name);
          ctx->error_count++;
        }
        return false;
      }
      if (cf->field.type && cf->field.type->impl->kind == TYPE_WILDCARD) continue;
      if (!_check_constraint_impl(ctx, af->field.type, cf->field.type, arg_expr, silent))
        return false;
    }
    return true;
  }

  case TYPE_TUPLE: {
    if (type_arg->impl->kind != TYPE_TUPLE) {
      if (!silent) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: expected tuple type");
        ctx->error_count++;
      }
      return false;
    }
    vec_t c_elems = constraint->impl->tuple.element_types;
    vec_t a_elems = type_arg->impl->tuple.element_types;
    size_t cec = c_elems ? vec_get_size(c_elems) : 0;
    size_t aec = a_elems ? vec_get_size(a_elems) : 0;
    if (aec < cec) {
      if (!silent) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                             "type does not satisfy constraint: tuple element count mismatch");
        ctx->error_count++;
      }
      return false;
    }
    for (size_t i = 0; i < cec; i++) {
      semantic_type_t ce = (semantic_type_t)vec_get(c_elems, i);
      semantic_type_t ae = (semantic_type_t)vec_get(a_elems, i);
      if (ce->impl->kind == TYPE_WILDCARD) continue;
      if (!_check_constraint_impl(ctx, ae, ce, arg_expr, silent))
        return false;
    }
    return true;
  }

  case TYPE_WILDCARD: {
    if (constraint->impl->wildcard.is_tuple) {
      if (type_arg->impl->kind != TYPE_TUPLE) {
        if (!silent) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, arg_expr->location,
                               "type does not satisfy constraint: expected tuple type");
          ctx->error_count++;
        }
        return false;
      }
    }
    return true;
  }
  default:
    return true;
  }
}

bool _check_constraint(context_t ctx, semantic_type_t type_arg,
                       semantic_type_t constraint, node_t arg_expr) {
  return _check_constraint_impl(ctx, type_arg, constraint, arg_expr, false);
}

bool _check_constraint_silent(context_t ctx, semantic_type_t type_arg,
                              semantic_type_t constraint) {
  return _check_constraint_impl(ctx, type_arg, constraint, NULL, true);
}

bool _check_generic_param_constraints(context_t ctx, vec_t generic_params,
                                       strmap_t type_bindings, node_t expr) {
  if (!generic_params || !type_bindings) return true;
  size_t gcount = vec_get_size(generic_params);
  bool all_ok = true;

  for (size_t i = 0; i < gcount; i++) {
    node_t gp_node = (node_t)vec_get(generic_params, i);
    if (!gp_node || gp_node->kind != CUBEC_NODE_GENERIC_PARAM) continue;

    cubec_generic_param_t gp = (cubec_generic_param_t)gp_node;
    const char *gp_name = _checker_ident_str(gp->name);
    semantic_type_t ta = gp_name ? (semantic_type_t)strmap_find(type_bindings, gp_name) : NULL;

    /* Value generic param: validate the value type */
    if (gp->value_type) {
      semantic_type_t resolved_vt = resolver_resolve_type(ctx, gp->value_type);
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
            gp_name ? gp_name : "?");
        ctx->error_count++;
        all_ok = false;
      }
      continue; /* Skip extends constraint check for value params */
    }

    if (!gp->constraints) continue;
    if (!ta) continue;

    size_t ccount = vec_get_size(gp->constraints);
    for (size_t c = 0; c < ccount; c++) {
      node_t cnode = (node_t)vec_get(gp->constraints, c);
      semantic_type_t constraint_type = resolver_resolve_type(ctx, cnode);
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
  }
  return all_ok;
}

vec_t _resolve_generic_type_args(context_t ctx, vec_t arg_exprs,
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
        extern comptime_value_t _comptime_eval_expr(comptime_eval_t, struct context *, node_t);
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
      extern semantic_type_t _check_expression(context_t, node_t);
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

strmap_t _resolve_generic_type_bindings_pack(context_t ctx, vec_t arg_exprs,
                                              vec_t generic_params) {
  vec_t type_args = _resolve_generic_type_args(ctx, arg_exprs, generic_params);
  if (!type_args) {
    strmap_init_t si = {.value_auto_dispose = false};
    strmap_t empty = (strmap_t)allocator_create(ctx->allocator, &g_strmap_type, &si);
    return empty;
  }

  /* Coalesce excess type args into packs for generic types with rest params.
     E.g. for Tuple[...Args], Tuple[i32, f64] → type_args = [PACK([i32, f64])].
     Or for foo[N, ...Args], foo[0, i32, f64] → [GENERIC_VALUE(0), PACK([i32, f64])]. */
  size_t gcount = generic_params ? vec_get_size(generic_params) : 0;
  size_t tacount = vec_get_size(type_args);

  /* Find the pack (rest) parameter position */
  size_t pack_idx = gcount; /* default: no pack */
  for (size_t i = 0; i < gcount; i++) {
    cubec_generic_param_t gp_node =
        (cubec_generic_param_t)(void *)vec_get(generic_params, i);
    if (gp_node && gp_node->is_rest) {
      pack_idx = i;
      break;
    }
  }

  if (pack_idx < gcount && tacount > pack_idx) {
    /* Check if all args from pack_idx onward form exactly one TYPE_GENERIC_PACK
       (e.g. Tuple[...Args] where Args is already a pack), use it directly
       instead of re-wrapping. */
    if (pack_idx + 1 == tacount) {
      semantic_type_t only_arg = (semantic_type_t)vec_get(type_args, pack_idx);
      if (only_arg && only_arg->impl->kind == TYPE_GENERIC_PACK) {
        /* Already a pack — no coalescing needed */
      } else {
        /* Single non-pack arg at pack position — wrap into a pack */
        goto do_coalesce_pack;
      }
    } else {
    do_coalesce_pack:;
      vec_init_t vi = {.auto_dispose = false};
      vec_t new_type_args = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
      for (size_t i = 0; i < pack_idx; i++)
        vec_push(new_type_args, vec_get(type_args, i));

      const char *pack_name = NULL;
      cubec_generic_param_t pack_gp =
          (cubec_generic_param_t)(void *)vec_get(generic_params, pack_idx);
      if (pack_gp) {
        const char *raw = _checker_ident_str(pack_gp->name);
        if (raw) pack_name = raw;
      }
      semantic_type_t pack_type = semantic_type_create_generic_pack(
          ctx->allocator, pack_name);
      for (size_t i = pack_idx; i < tacount; i++) {
        semantic_type_t ta = (semantic_type_t)vec_get(type_args, i);
        vec_push(pack_type->impl->generic_pack.expanded_types, ta);
      }
      type_hash_ensure(pack_type);
      vec_push(ctx->all_types, pack_type);
      vec_push(new_type_args, pack_type);
      allocator_free(ctx->allocator, &type_args);
      type_args = new_type_args;
    }
  }

  strmap_t bindings = _type_bindings_from_vec(ctx->allocator, generic_params, type_args);
  allocator_free(ctx->allocator, &type_args);
  return bindings;
}

static vec_t _copy_symbol_vec(context_t ctx, vec_t src) {
  if (!src) return NULL;
  /* auto_dispose = false: symbols are owned by the template type.
     If true, both template and instance would free the same symbols (double-free). */
  vec_init_t vi = {.auto_dispose = false};
  vec_t dst = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  size_t count = vec_get_size(src);
  for (size_t i = 0; i < count; i++)
    vec_push(dst, (struct symbol *)vec_get(src, i));
  return dst;
}

static void _instantiate_struct_fields(context_t ctx, semantic_type_t inst,
                                        vec_t tpl_fields, strmap_t type_bindings) {
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
    nf->field.type = _substitute_type(ctx, f->field.type, type_bindings);
    vec_push(inst->impl->generic_instance.fields, nf);
  }
  type_layout_compute(inst, 8);
}

semantic_type_t _instantiate_type(context_t ctx, semantic_type_t template_type,
                                   strmap_t type_bindings, node_t instantiation_expr) {
  const char *name = template_type->name;
  if (!name) name = "<anonymous>";
  /* Check cache */
  semantic_type_t cached = _cache_lookup(ctx, name, type_bindings);
  if (cached) {
    allocator_free(ctx->allocator, &type_bindings);
    return cached;
  }

  /* Create the specialized type as a GENERIC_INSTANCE */
  semantic_type_t inst = NULL;

  /* Copy structural info from template based on kind */
  enum type_kind tkind = template_type->impl->kind;

  /* Special handling for TYPE_TUPLE: create a native TYPE_TUPLE instead of
     GENERIC_INSTANCE. type_bindings should contain a TYPE_GENERIC_PACK entry. */
  if (tkind == TYPE_TUPLE) {
    vec_t field_types = NULL;
    if (type_bindings) {
      strmap_iter_t iter = strmap_iter_first(type_bindings);
      const char *bname = NULL;
      while ((bname = strmap_iter_next(&iter)) != NULL) {
        semantic_type_t ta = (semantic_type_t)strmap_find(type_bindings, bname);
        if (ta && ta->impl && ta->impl->kind == TYPE_GENERIC_PACK) {
          field_types = ta->impl->generic_pack.expanded_types;
          break;
        }
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
    _cache_insert(ctx, name, type_bindings, inst);
    allocator_free(ctx->allocator, &type_bindings);
    return inst;
  }

  /* For non-TUPLE types, create a GENERIC_INSTANCE */
  inst = semantic_type_create_generic_instance(
      ctx->allocator, template_type, type_bindings);
  vec_push(ctx->all_types, inst);
  type_hash_ensure(inst);

  if (tkind == TYPE_STRUCT || tkind == TYPE_UNION || tkind == TYPE_CUNION) {
    _instantiate_struct_fields(ctx, inst, template_type->impl->struct_type.fields, type_bindings);
  }

  /* Copy method lists from template (free the init-created vecs first) */
  allocator_free(ctx->allocator, &inst->instance_methods);
  allocator_free(ctx->allocator, &inst->static_methods);
  inst->instance_methods = _copy_symbol_vec(ctx,template_type->instance_methods);
  inst->static_methods = _copy_symbol_vec(ctx, template_type->static_methods);

  /* Propagate implements from template, with generic param substitution */
  if (template_type->implements) {
    size_t ic = vec_get_size(template_type->implements);
    vec_t inst_impls = (vec_t)allocator_create(
        ctx->allocator, &g_vec_type, &(vec_init_t){false});
    for (size_t i = 0; i < ic; i++) {
      semantic_type_t iface = (semantic_type_t)vec_get(template_type->implements, i);
      semantic_type_t substituted = _substitute_type(ctx, iface, type_bindings);
      vec_push(inst_impls, substituted);
    }
    inst->implements = inst_impls;
  }

  /* Cache the result */
  _cache_insert(ctx, name, type_bindings, inst);

  /* Do NOT free type_bindings — the GENERIC_INSTANCE owns it (stored at
     inst->impl->generic_instance.type_bindings). Freeing would cause use-after-free
     when the instance's bindings are later read (e.g. by _substitute_type,
     _type_unify, or constraint checking). */
  return inst;
}

semantic_type_t _instantiate_function(context_t ctx, struct symbol *func_sym,
                                      strmap_t type_bindings, node_t instantiation_expr) {
  const char *name = func_sym->name;
  ctx->instantiate_func_count++;
  if (ctx->instantiate_func_count > 10000) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
        instantiation_expr ? instantiation_expr->location : (location_t){0},
        "too many generic instantiations for '%s' (possible infinite recursion)",
        name ? name : "<anonymous>");
    ctx->error_count++;
    return ctx->error_type;
  }
  semantic_type_t func_type = func_sym->function.type;
  if (!func_type) return ctx->error_type;

  /* Check cache */
  semantic_type_t cached = _cache_lookup(ctx, name, type_bindings);
  if (cached) {
    return cached;
  }

  /* Substitute generic params in the entire function type.
     Using _substitute_type on the whole type handles pack expansion
     correctly (TYPE_FUNCTION branch expands TYPE_GENERIC_PACK params). */
  semantic_type_t inst_type = _substitute_type(ctx, func_type, type_bindings);

  /* If nothing changed (no generic params), return original */
  if (inst_type == func_type) {
    return func_type;
  }

  /* Cache */
  _cache_insert(ctx, name, type_bindings, inst_type);

  /* NOTE: caller owns type_bindings and must free it or pass to _enqueue_body_check */
  return inst_type;
}

/* ===== literal numeric helper ===== */

semantic_type_t _check_literal_numeric(context_t ctx, node_t num_node) {
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
