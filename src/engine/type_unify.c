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
    /* Parameter count must match */
    size_t apcount = vec_get_size(actual->impl->function.params);
    size_t epcount = vec_get_size(expected->impl->function.params);
    if (apcount != epcount) return false;

    /* Unify each parameter */
    for (size_t i = 0; i < epcount; i++) {
      semantic_type_t ap = (semantic_type_t)vec_get(actual->impl->function.params, i);
      semantic_type_t ep = (semantic_type_t)vec_get(expected->impl->function.params, i);
      if (!_type_unify(ap, ep, bindings, allocator)) return false;
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

    /* Unify type args */
    size_t acount = vec_get_size(actual->impl->generic_instance.type_args);
    size_t ecount = vec_get_size(expected->impl->generic_instance.type_args);
    if (acount != ecount) return false;

    for (size_t i = 0; i < ecount; i++) {
      semantic_type_t aa = (semantic_type_t)vec_get(actual->impl->generic_instance.type_args, i);
      semantic_type_t ea = (semantic_type_t)vec_get(expected->impl->generic_instance.type_args, i);
      if (!_type_unify(aa, ea, bindings, allocator)) return false;
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
      if (!af->name || !ef->name || strcmp(af->name, ef->name) != 0) return false;
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

  for (size_t i = 0; i < pcount && i < acount; i++) {
    semantic_type_t param_type = (semantic_type_t)vec_get(func_params, i);
    semantic_type_t arg_type = (semantic_type_t)vec_get(arg_types, i);
    _type_unify(arg_type, param_type, &bindings, ctx->allocator);
  }

  /* Step 3: Map bindings to result vec by generic param name/index */
  for (size_t i = 0; i < gcount; i++) {
    /* Already filled by explicit args */
    if (vec_get(result, i) != NULL) continue;

    /* Look up binding by param name */
    struct symbol *gp_sym = (struct symbol *)vec_get(generic_params, i);
    const char *gp_name = gp_sym ? gp_sym->name : NULL;
    if (!gp_name) continue;

    void *found = strmap_find(bindings, gp_name);
    if (found) {
      vec_set(result, i, found);
    }
  }

  allocator_free(ctx->allocator, &bindings);
  return result;
}
