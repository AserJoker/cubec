/**
 * @file type_unify.c
 * @brief Type unification engine for generic type inference.
 *
 * Core algorithm: match an actual type against an expected type pattern,
 * extracting generic parameter bindings. For example:
 *   Vec[i32] vs Vec[T] → bind T = i32
 *   *const i32 vs *T   → bind T = const i32
 *   []i32 vs []?       → match slice structure, skip element check
 */
#include "engine/checker_type_util.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "core/allocator.h"
#include "core/strmap.h"
#include "core/vec.h"
#include <string.h>

/* ===== internal: recursive unification ===== */

/**
 * Unify actual type against expected pattern, collecting bindings.
 * Returns true if unification succeeds, false on conflict.
 *
 * @param actual    The concrete type from the call argument
 * @param expected  The pattern type (may contain TYPE_GENERIC_PARAM)
 * @param bindings  Map: param_name (string) → semantic_type_t (pointer stored as void*)
 * @param allocator For string allocations
 */
static bool _type_unify(semantic_type_t actual, semantic_type_t expected,
                        strmap_t *bindings, allocator_t allocator) {
  if (!actual || !expected || !expected->impl) return false;

  /* If actual is an error, unification fails */
  if (actual->impl->kind == TYPE_ERROR) return false;

  /* If expected is a wildcard, always matches */
  if (expected->impl->kind == TYPE_WILDCARD) return true;

  /* Expected is a generic param → record binding */
  if (expected->impl->kind == TYPE_GENERIC_PARAM) {
    const char *name = expected->impl->generic_param.name;
    if (!name) return false;

    /* Check for existing binding (same param used in multiple positions) */
    void *existing = strmap_find(*bindings, name);
    if (existing) {
      semantic_type_t prev = (semantic_type_t)existing;
      /* Must be structurally equivalent */
      return prev->impl->hash == actual->impl->hash;
    }

    /* Record new binding */
    strmap_insert(*bindings, name, actual);
    return true;
  }

  /* Expected is a generic pack → record binding (collect all remaining actual types) */
  if (expected->impl->kind == TYPE_GENERIC_PACK) {
    const char *name = expected->impl->generic_pack.name;
    if (!name) return false;

    /* Check for existing binding */
    void *existing = strmap_find(*bindings, name);
    if (existing) {
      /* Already bound — check consistency */
      semantic_type_t prev = (semantic_type_t)existing;
      return prev->impl->hash == actual->impl->hash;
    }

    /* Record new binding */
    strmap_insert(*bindings, name, actual);
    return true;
  }

  /* Both must have the same type kind for structural matching */
  if (actual->impl->kind != expected->impl->kind) return false;

  switch (expected->impl->kind) {
  case TYPE_POINTER:
    return _type_unify(actual->impl->pointer.pointee,
                       expected->impl->pointer.pointee, bindings, allocator);

  case TYPE_SLICE:
    return _type_unify(actual->impl->slice.element,
                       expected->impl->slice.element, bindings, allocator);

  case TYPE_ARRAY:
    /* Array length must match */
    if (actual->impl->array.length != expected->impl->array.length) return false;
    return _type_unify(actual->impl->array.element,
                       expected->impl->array.element, bindings, allocator);

  case TYPE_QUALIFIER:
    /* Qualifier flags must match */
    if (actual->impl->qualifier.is_const != expected->impl->qualifier.is_const ||
        actual->impl->qualifier.is_volatile != expected->impl->qualifier.is_volatile)
      return false;
    return _type_unify(actual->impl->qualifier.base,
                       expected->impl->qualifier.base, bindings, allocator);

  case TYPE_FUNCTION: {
    /* Check if expected params contain a TYPE_GENERIC_PACK for elastic matching */
    size_t apcount = vec_get_size(actual->impl->function.params);
    size_t epcount = vec_get_size(expected->impl->function.params);

    /* Find pack position in expected params */
    size_t pack_pos = epcount; /* position of the first pack param, or epcount if none */
    for (size_t i = 0; i < epcount; i++) {
      semantic_type_t ep = (semantic_type_t)vec_get(expected->impl->function.params, i);
      if (ep && ep->impl->kind == TYPE_GENERIC_PACK) {
        pack_pos = i;
        break;
      }
    }

    if (pack_pos < epcount) {
      /* Elastic matching: params before the pack must match exactly,
         pack consumes all remaining actual params */
      /* Check params before the pack */
      for (size_t i = 0; i < pack_pos; i++) {
        if (i >= apcount) return false;
        semantic_type_t ap = (semantic_type_t)vec_get(actual->impl->function.params, i);
        semantic_type_t ep = (semantic_type_t)vec_get(expected->impl->function.params, i);
        if (!_type_unify(ap, ep, bindings, allocator)) return false;
      }

      /* Collect remaining actual params into the pack binding */
      semantic_type_t pack_type = (semantic_type_t)vec_get(expected->impl->function.params, pack_pos);
      const char *pack_name = pack_type->impl->generic_pack.name;

      /* Create a TYPE_GENERIC_PACK with the expanded types */
      semantic_type_t pack_result = semantic_type_create_generic_pack(
          allocator, pack_name, pack_type->impl->generic_pack.index);
      for (size_t i = pack_pos; i < apcount; i++) {
        semantic_type_t ap = (semantic_type_t)vec_get(actual->impl->function.params, i);
        vec_push(pack_result->impl->generic_pack.expanded_types, ap);
      }
      type_hash_ensure(pack_result);

      /* Record binding */
      void *existing = strmap_find(*bindings, pack_name);
      if (existing) {
        semantic_type_t prev = (semantic_type_t)existing;
        if (prev->impl->hash != pack_result->impl->hash) return false;
      } else {
        strmap_insert(*bindings, pack_name, pack_result);
      }
    } else {
      /* No pack — strict parameter count matching */
      if (apcount != epcount) return false;

      /* Unify each parameter */
      for (size_t i = 0; i < epcount; i++) {
        semantic_type_t ap = (semantic_type_t)vec_get(actual->impl->function.params, i);
        semantic_type_t ep = (semantic_type_t)vec_get(expected->impl->function.params, i);
        if (!_type_unify(ap, ep, bindings, allocator)) return false;
      }
    }

    /* Unify return type */
    return _type_unify(actual->impl->function.return_type,
                       expected->impl->function.return_type, bindings, allocator);
  }

  case TYPE_GENERIC_INSTANCE: {
    /* Both must be instances of the same template */
    if (actual->impl->generic_instance.generic_template !=
        expected->impl->generic_instance.generic_template)
      return false;

    /* Find pack position in expected type_args */
    size_t acount = vec_get_size(actual->impl->generic_instance.type_args);
    size_t ecount = vec_get_size(expected->impl->generic_instance.type_args);
    size_t pack_idx = ecount;
    for (size_t i = 0; i < ecount; i++) {
      semantic_type_t ea = (semantic_type_t)vec_get(expected->impl->generic_instance.type_args, i);
      if (ea && ea->impl->kind == TYPE_GENERIC_PACK) {
        pack_idx = i;
        break;
      }
    }

    if (pack_idx < ecount) {
      /* Expected has a pack in its type_args — elastic matching:
         args before the pack must match exactly, remaining actual
         args are collected into the pack binding. */
      for (size_t i = 0; i < pack_idx; i++) {
        if (i >= acount) return false;
        semantic_type_t aa = (semantic_type_t)vec_get(actual->impl->generic_instance.type_args, i);
        semantic_type_t ea = (semantic_type_t)vec_get(expected->impl->generic_instance.type_args, i);
        if (!_type_unify(aa, ea, bindings, allocator)) return false;
      }
      /* Collect remaining actual args into the pack */
      semantic_type_t pack_type = (semantic_type_t)vec_get(
          expected->impl->generic_instance.type_args, pack_idx);
      const char *pack_name = pack_type->impl->generic_pack.name;
      semantic_type_t pack_result = semantic_type_create_generic_pack(
          allocator, pack_name, pack_type->impl->generic_pack.index);
      for (size_t i = pack_idx; i < acount; i++) {
        semantic_type_t aa = (semantic_type_t)vec_get(actual->impl->generic_instance.type_args, i);
        vec_push(pack_result->impl->generic_pack.expanded_types, aa);
      }
      type_hash_ensure(pack_result);
      /* Record binding */
      void *existing = strmap_find(*bindings, pack_name);
      if (existing) {
        semantic_type_t prev = (semantic_type_t)existing;
        if (prev->impl->hash != pack_result->impl->hash) return false;
      } else {
        strmap_insert(*bindings, pack_name, pack_result);
      }
    } else {
      /* No pack — strict count matching */
      if (acount != ecount) return false;

      for (size_t i = 0; i < ecount; i++) {
        semantic_type_t aa = (semantic_type_t)vec_get(actual->impl->generic_instance.type_args, i);
        semantic_type_t ea = (semantic_type_t)vec_get(expected->impl->generic_instance.type_args, i);
        if (!_type_unify(aa, ea, bindings, allocator)) return false;
      }
    }
    return true;
  }

  /* Primitive types: exact match (same hash) */
  case TYPE_VOID: case TYPE_BOOL:
  case TYPE_I8: case TYPE_I16: case TYPE_I32: case TYPE_I64:
  case TYPE_U8: case TYPE_U16: case TYPE_U32: case TYPE_U64:
  case TYPE_F16: case TYPE_F32: case TYPE_F64:
  case TYPE_CHAR: case TYPE_STRING:
  case TYPE_NIL:
    return actual->impl->hash == expected->impl->hash;

  /* Struct/union/cunion: match field-by-field */
  case TYPE_STRUCT:
  case TYPE_UNION:
  case TYPE_CUNION: {
    vec_t afields = actual->impl->struct_type.fields;
    vec_t efields = expected->impl->struct_type.fields;
    size_t afc = afields ? vec_get_size(afields) : 0;
    size_t efc = efields ? vec_get_size(efields) : 0;
    if (afc != efc) return false;

    for (size_t i = 0; i < efc; i++) {
      struct symbol *af = (struct symbol *)vec_get(afields, i);
      struct symbol *ef = (struct symbol *)vec_get(efields, i);
      /* Field names must match */
      if (!af || !ef || !af->name || !ef->name || strcmp(af->name, ef->name) != 0) return false;
      /* Unify field types */
      if (!_type_unify(af->field.type, ef->field.type, bindings, allocator))
        return false;
    }
    return true;
  }

  default:
    return actual->impl->hash == expected->impl->hash;
  }
}

/* ===== internal: collect generic param names from a function type ===== */

/**
 * Extracts generic parameter names from a function type's parameter types.
 * For each parameter that is TYPE_GENERIC_PARAM, records its name.
 * Returns a vec of const char* (names), parallel to the function params.
 */
static vec_t _collect_generic_param_names(semantic_type_t func_type,
                                          allocator_t allocator) {
  vec_t params = func_type->impl->function.params;
  size_t pcount = params ? vec_get_size(params) : 0;
  vec_init_t vi = {.auto_dispose = false};
  vec_t names = (vec_t)allocator_create(allocator, &g_vec_type, &vi);

  /* Collect names from function params */
  for (size_t i = 0; i < pcount; i++) {
    semantic_type_t p = (semantic_type_t)vec_get(params, i);
    if (p && p->impl->kind == TYPE_GENERIC_PARAM)
      vec_push(names, (void *)p->impl->generic_param.name);
    else
      vec_push(names, NULL);
  }

  /* Also check return type */
  semantic_type_t ret = func_type->impl->function.return_type;
  if (ret && ret->impl->kind == TYPE_GENERIC_PARAM)
    vec_push(names, (void *)ret->impl->generic_param.name);
  else
    vec_push(names, NULL);

  return names;
}

/* ===== public API ===== */

vec_t _infer_type_args_from_call(checker_t ctx,
                                  semantic_type_t func_type,
                                  vec_t generic_params,
                                  vec_t arg_types,
                                  vec_t explicit_type_args) {
  if (!func_type || func_type->impl->kind != TYPE_FUNCTION) return NULL;

  size_t gcount = generic_params ? vec_get_size(generic_params) : 0;
  if (gcount == 0) return explicit_type_args;  /* Not generic */

  /* Initialize result vec: fill with NULLs for each generic param */
  vec_init_t vi = {.auto_dispose = false};
  vec_t result = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  for (size_t i = 0; i < gcount; i++)
    vec_push(result, NULL);

  /* Step 1: Fill in explicit type args */
  size_t ecount = explicit_type_args ? vec_get_size(explicit_type_args) : 0;
  for (size_t i = 0; i < ecount && i < gcount; i++) {
    semantic_type_t ta = (semantic_type_t)vec_get(explicit_type_args, i);
    if (ta) {
      vec_set(result, i, ta);
    }
  }

  /* Step 2: Unify call args with func params to infer missing bindings */
  strmap_init_t si = {.value_auto_dispose = false};
  strmap_t bindings = (strmap_t)allocator_create(ctx->allocator, &g_strmap_type, &si);
  vec_t func_params = func_type->impl->function.params;
  size_t pcount = func_params ? vec_get_size(func_params) : 0;
  size_t acount = arg_types ? vec_get_size(arg_types) : 0;

  /* Find pack position in func params */
  size_t pack_pos = pcount;  /* position of first pack param, or pcount if none */
  for (size_t i = 0; i < pcount; i++) {
    semantic_type_t ep = (semantic_type_t)vec_get(func_params, i);
    if (ep && ep->impl->kind == TYPE_GENERIC_PACK) {
      pack_pos = i;
      break;
    }
  }

  if (pack_pos < pcount) {
    /* Elastic matching: params before the pack must match exactly */
    for (size_t i = 0; i < pack_pos && i < acount; i++) {
      semantic_type_t param_type = (semantic_type_t)vec_get(func_params, i);
      semantic_type_t arg_type = (semantic_type_t)vec_get(arg_types, i);
      _type_unify(arg_type, param_type, &bindings, ctx->allocator);
    }

    /* Collect remaining actual args into the pack binding */
    semantic_type_t pack_type = (semantic_type_t)vec_get(func_params, pack_pos);
    const char *pack_name = pack_type->impl->generic_pack.name;

    /* Create a TYPE_GENERIC_PACK with the expanded types */
    semantic_type_t pack_result = semantic_type_create_generic_pack(
        ctx->allocator, pack_name, pack_type->impl->generic_pack.index);
    for (size_t i = pack_pos; i < acount; i++) {
      semantic_type_t ap = (semantic_type_t)vec_get(arg_types, i);
      vec_push(pack_result->impl->generic_pack.expanded_types, ap);
    }
    type_hash_ensure(pack_result);
    vec_push(ctx->all_types, pack_result);

    /* Record binding */
    void *existing = strmap_find(bindings, pack_name);
    if (existing) {
      semantic_type_t prev = (semantic_type_t)existing;
      if (prev->impl->hash != pack_result->impl->hash) {
        /* Conflict — but don't fail, just don't update */
      }
    } else {
      strmap_insert(bindings, pack_name, pack_result);
    }
  } else {
    /* No pack — strict parameter count matching */
    for (size_t i = 0; i < pcount && i < acount; i++) {
      semantic_type_t param_type = (semantic_type_t)vec_get(func_params, i);
      semantic_type_t arg_type = (semantic_type_t)vec_get(arg_types, i);
      _type_unify(arg_type, param_type, &bindings, ctx->allocator);
    }
  }

  /* Also unify return type if it contains a generic param not covered by args */
  semantic_type_t ret_type = func_type->impl->function.return_type;
  if (ret_type && ret_type->impl->kind == TYPE_GENERIC_PARAM) {
    /* Return type param may be inferable if same as a param type */
    /* Already handled: if arg unification bound the same name, it's in bindings */
  }

  /* Step 3: Map bindings to result vec by generic param name.
     generic_params vec contains const char* name strings (passed from caller).
     We look up each generic param name in the bindings map. */
  for (size_t i = 0; i < gcount; i++) {
    /* Already filled by explicit args */
    if (vec_get(result, i) != NULL) continue;

    /* Get param name from generic_params (const char* strings) */
    const char *gp_name = (const char *)vec_get(generic_params, i);
    if (!gp_name) continue;

    void *found = strmap_find(bindings, gp_name);
    if (found) {
      vec_set(result, i, found);
    }
  }

  allocator_free(ctx->allocator, &bindings);
  return result;
}
