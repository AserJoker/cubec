#include "engine/comptime_value.h"
#include "engine/comptime_eval.h"
#include "engine/symbol.h"
#include "engine/type_layout.h"
#include "engine/type_hash.h"
#include "core/string.h"
#include <stdlib.h>
#include <string.h>

/* ===== type vtable ===== */

static void _comptime_value_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  comptime_value_t dst = (comptime_value_t)self;
  comptime_value_t src = (comptime_value_t)arg;
  *dst = *src;
}

/* Iterate string_t fields in a composite value's data buffer.
 * For structs: iterate all fields.
 * For tagged unions: only iterate fields of the active variant
 *   (non-active variant slots contain garbage, not valid string_t pointers). */
static void _foreach_string_field(comptime_value_t v,
                                  bool (*fn)(string_t *slot, void *ctx),
                                  void *ctx) {
  if (!v->type || !v->type->impl || !v->composite.data) return;
  vec_t fields = NULL;
  type_impl_t impl = v->type->impl;
  bool is_union = false;
  if (impl->kind == TYPE_STRUCT) {
    fields = impl->struct_type.fields;
  } else if (impl->kind == TYPE_UNION || impl->kind == TYPE_CUNION) {
    fields = impl->struct_type.fields;
    is_union = true;
  } else if (impl->kind == TYPE_GENERIC_INSTANCE) {
    fields = impl->generic_instance.fields;
    semantic_type_t base = impl->generic_instance.generic_template;
    if (base && base->impl &&
        (base->impl->kind == TYPE_UNION || base->impl->kind == TYPE_CUNION))
      is_union = true;
  }
  if (!fields) return;

  /* For tagged unions, find the active variant's type hash */
  uint64_t active_tag = 0;
  if (is_union && comptime_value_is_tagged_union(v)) {
    active_tag = comptime_value_get_union_tag(v);
  }

  size_t fc = vec_get_size(fields);
  for (size_t i = 0; i < fc; i++) {
    struct symbol *s = (struct symbol *)vec_get(fields, i);
    if (!s || (s->kind != SYMBOL_VARIABLE && s->kind != SYMBOL_FIELD)) continue;
    semantic_type_t ft = s->field.type;
    if (!ft || !ft->impl) continue;
    /* For tagged unions, skip non-active variant fields */
    if (is_union && active_tag != 0) {
      type_hash_ensure(ft);
      if (ft->impl->hash != active_tag) continue;
    }
    enum type_kind fk = ft->impl->kind;
    if (fk == TYPE_STRING || fk == TYPE_STR) {
      string_t *slot = (string_t *)(void *)(v->composite.data + s->field.offset);
      fn(slot, ctx);
    }
  }
}

static bool _dispose_string_slot(string_t *slot, void *ctx) {
  allocator_t allocator = (allocator_t)ctx;
  if (*slot) allocator_free(allocator, (void **)slot);
  return true;
}

static bool _clone_string_slot(string_t *slot, void *ctx) {
  allocator_t allocator = (allocator_t)ctx;
  if (*slot) {
    string_t new_str = (string_t)allocator_create(allocator, &g_string_type, NULL);
    string_set(new_str, string_get(*slot));
    *slot = new_str;
  }
  return true;
}

static void _comptime_value_dispose(void *self, allocator_t allocator) {
  comptime_value_t v = (comptime_value_t)self;
  switch (v->kind) {
  case COMPTIME_VALUE_STRING:
    allocator_free(allocator, &v->string_val);
    break;
  case COMPTIME_VALUE_COMPOSITE:
    if (v->composite.data) {
      /* Dispose string_t objects embedded in composite data before freeing the buffer.
       * For tagged unions, only the active variant's string_t is valid. */
      _foreach_string_field(v, _dispose_string_slot, allocator);
      allocator_free(allocator, (void **)&v->composite.data);
    }
    break;
  case COMPTIME_VALUE_FUNCTION:
    /* captured_env is NOT owned by the function value — it is tracked
       in comptime_eval.captured_envs and disposed during eval teardown.
       This avoids double-free when function values are cloned (the clone
       shares the same captured_env pointer). */
    allocator_free(allocator, &v->function.param_names);
    break;
  case COMPTIME_VALUE_PACK:
    allocator_free(allocator, &v->pack.elements);
    break;
  default:
    break;
  }
}

static void _comptime_value_clone(void *self, allocator_t allocator,
                                  void *another) {
  comptime_value_t dst = (comptime_value_t)self;
  comptime_value_t src = (comptime_value_t)another;
  *dst = *src;

  switch (dst->kind) {
  case COMPTIME_VALUE_STRING:
    if (src->string_val) {
      dst->string_val = (string_t)allocator_create(allocator, &g_string_type, NULL);
      string_set(dst->string_val, string_get(src->string_val));
    }
    break;
  case COMPTIME_VALUE_COMPOSITE:
    if (src->composite.data && src->composite.data_size > 0) {
      dst->composite.data = (uint8_t *)allocator_alloc(allocator,
                                                        src->composite.data_size);
      memcpy(dst->composite.data, src->composite.data, src->composite.data_size);
      /* Deep-copy string_t fields: memcpy copies the pointer but not the
       * underlying string object. For tagged unions, only the active variant's
       * string_t is valid — non-active slots contain garbage.
       * Use dst (which has the same type and tag as src) to find slots. */
      _foreach_string_field(dst, _clone_string_slot, allocator);
    }
    break;
  case COMPTIME_VALUE_FUNCTION:
    if (src->function.param_names) {
      size_t pc = vec_get_size(src->function.param_names);
      vec_init_t vi = {.auto_dispose = false};
      dst->function.param_names =
          (vec_t)allocator_create(allocator, &g_vec_type, &vi);
      for (size_t i = 0; i < pc; i++)
        vec_push(dst->function.param_names,
                 (void *)vec_get(src->function.param_names, i));
    }
    break;
  case COMPTIME_VALUE_PACK:
    if (src->pack.elements) {
      size_t ec = vec_get_size(src->pack.elements);
      vec_init_t vi = {.auto_dispose = true};
      dst->pack.elements =
          (vec_t)allocator_create(allocator, &g_vec_type, &vi);
      for (size_t i = 0; i < ec; i++) {
        comptime_value_t elem = (comptime_value_t)vec_get(src->pack.elements, i);
        comptime_value_t cloned = comptime_value_clone(allocator, elem);
        vec_push(dst->pack.elements, cloned);
      }
    }
    break;
  default:
    break;
  }
}

type_t g_comptime_value_type = {
    .size = sizeof(struct comptime_value),
    .name = "cubec.engine.comptime_value",
    .init = (type_init_fn_t)_comptime_value_init,
    .dispose = (type_dispose_fn_t)_comptime_value_dispose,
    .clone = (type_clone_fn_t)_comptime_value_clone,
    .move = NULL,
};

/* ===== constructors ===== */

comptime_value_t comptime_value_create_nil(allocator_t allocator,
                                           semantic_type_t type) {
  struct comptime_value init = {.kind = COMPTIME_VALUE_NIL, .type = type};
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             &init);
}

comptime_value_t comptime_value_create_bool(allocator_t allocator, bool val,
                                           semantic_type_t type) {
  struct comptime_value init = {
      .kind = COMPTIME_VALUE_BOOL, .type = type, .bool_val = val};
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             &init);
}

comptime_value_t comptime_value_create_int(allocator_t allocator,
                                           int64_t sval, uint64_t uval,
                                           uint8_t width, bool is_signed,
                                           semantic_type_t type) {
  struct comptime_value init = {
      .kind = COMPTIME_VALUE_INT,
      .type = type,
      .int_val = {.s = sval, .u = uval, .width = width, .is_signed = is_signed}};
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             &init);
}

comptime_value_t comptime_value_create_float(allocator_t allocator,
                                             double val, uint8_t width,
                                             semantic_type_t type) {
  struct comptime_value init = {
      .kind = COMPTIME_VALUE_FLOAT,
      .type = type,
      .float_val = {.value = val, .width = width}};
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             &init);
}

comptime_value_t comptime_value_create_char(allocator_t allocator, char val,
                                            semantic_type_t type) {
  struct comptime_value init = {
      .kind = COMPTIME_VALUE_CHAR, .type = type, .char_val = val};
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             &init);
}

comptime_value_t comptime_value_create_string(allocator_t allocator,
                                              const char *val,
                                              semantic_type_t type) {
  string_t s = (string_t)allocator_create(allocator, &g_string_type, NULL);
  if (val) string_set(s, val);
  struct comptime_value init = {
      .kind = COMPTIME_VALUE_STRING, .type = type, .string_val = s};
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             &init);
}

comptime_value_t comptime_value_create_type(allocator_t allocator,
                                            semantic_type_t val) {
  struct comptime_value init = {
      .kind = COMPTIME_VALUE_TYPE, .type = NULL, .type_val = val};
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             &init);
}

comptime_value_t comptime_value_create_pointer(allocator_t allocator,
                                               uint64_t addr,
                                               semantic_type_t type) {
  struct comptime_value init = {
      .kind = COMPTIME_VALUE_POINTER, .type = type, .pointer = {.addr = addr}};
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             &init);
}

comptime_value_t comptime_value_create_composite(allocator_t allocator,
                                                  semantic_type_t type,
                                                  semantic_type_t element_type,
                                                  size_t data_size) {
  uint8_t *data = NULL;
  if (data_size > 0) {
    data = (uint8_t *)allocator_alloc(allocator, data_size);
    memset(data, 0, data_size);
  }
  struct comptime_value init = {
      .kind = COMPTIME_VALUE_COMPOSITE,
      .type = type,
      .composite = {.data = data, .data_size = data_size,
                     .element_type = element_type}};
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             &init);
}

comptime_value_t comptime_value_create_function(allocator_t allocator,
                                                 comptime_env_t env,
                                                 node_t body,
                                                 vec_t param_names,
                                                 semantic_type_t type) {
  struct comptime_value init = {
      .kind = COMPTIME_VALUE_FUNCTION,
      .type = type,
      .function = {.captured_env = env,
                    .body = body,
                    .param_names = param_names}};
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             &init);
}

comptime_value_t comptime_value_create_error(allocator_t allocator) {
  struct comptime_value init = {.kind = COMPTIME_VALUE_ERROR, .type = NULL};
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             &init);
}

comptime_value_t comptime_value_create_pack(allocator_t allocator,
                                            vec_t elements,
                                            semantic_type_t type) {
  struct comptime_value init = {
      .kind = COMPTIME_VALUE_PACK, .type = type,
      .pack = {.elements = elements}};
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             &init);
}

/* ===== queries ===== */

bool comptime_value_is_truthy(comptime_value_t val) {
  if (!val || val->kind == COMPTIME_VALUE_ERROR) return false;
  switch (val->kind) {
  case COMPTIME_VALUE_NIL:    return false;
  case COMPTIME_VALUE_BOOL:   return val->bool_val;
  case COMPTIME_VALUE_INT:    return val->int_val.is_signed
                                     ? val->int_val.s != 0
                                     : val->int_val.u != 0;
  case COMPTIME_VALUE_FLOAT:  return val->float_val.value != 0.0;
  case COMPTIME_VALUE_CHAR:   return val->char_val != 0;
  case COMPTIME_VALUE_STRING: {
    const char *s = val->string_val ? string_get(val->string_val) : NULL;
    return s && s[0] != '\0';
  }
  case COMPTIME_VALUE_POINTER: return val->pointer.addr != 0;
  default:                    return true;
  }
}

bool comptime_value_equals(comptime_value_t a, comptime_value_t b) {
  if (!a && !b) return true;
  if (!a || !b) return false;
  if (a->kind != b->kind) return false;
  switch (a->kind) {
  case COMPTIME_VALUE_NIL:    return true;
  case COMPTIME_VALUE_BOOL:   return a->bool_val == b->bool_val;
  case COMPTIME_VALUE_INT:
    if (a->int_val.is_signed == b->int_val.is_signed) {
      return a->int_val.is_signed ? a->int_val.s == b->int_val.s
                                  : a->int_val.u == b->int_val.u;
    }
    /* Cross signed/unsigned: compare as unsigned if both values are
     * non-negative in their respective representation */
    if (a->int_val.is_signed && a->int_val.s < 0) return false;
    if (b->int_val.is_signed && b->int_val.s < 0) return false;
    return a->int_val.is_signed ? (uint64_t)a->int_val.s == b->int_val.u
                                : a->int_val.u == (uint64_t)b->int_val.s;
  case COMPTIME_VALUE_FLOAT:  return a->float_val.value == b->float_val.value;
  case COMPTIME_VALUE_CHAR:   return a->char_val == b->char_val;
  case COMPTIME_VALUE_STRING: {
    const char *sa = a->string_val ? string_get(a->string_val) : NULL;
    const char *sb = b->string_val ? string_get(b->string_val) : NULL;
    if (!sa && !sb) return true;
    if (!sa || !sb) return false;
    return strcmp(sa, sb) == 0;
  }
  case COMPTIME_VALUE_TYPE:   return a->type_val == b->type_val;
  case COMPTIME_VALUE_POINTER: return a->pointer.addr == b->pointer.addr;
  case COMPTIME_VALUE_COMPOSITE: {
    if (a->composite.data_size != b->composite.data_size) return false;
    if (!a->composite.data && !b->composite.data) return true;
    if (!a->composite.data || !b->composite.data) return false;
    /* If same type, memcmp the raw bytes */
    if (a->type && a->type == b->type)
      return memcmp(a->composite.data, b->composite.data,
                    a->composite.data_size) == 0;
    /* Fallback: if same element_type (arrays), memcmp raw bytes.
     * Same element_type means same layout, so memcmp is correct and avoids
     * needing an allocator for comptime_value_read_field. */
    if (a->composite.element_type && a->composite.element_type == b->composite.element_type) {
      return memcmp(a->composite.data, b->composite.data,
                    a->composite.data_size) == 0;
    }
    /* Different types with raw data: memcmp as last resort */
    return memcmp(a->composite.data, b->composite.data,
                  a->composite.data_size) == 0;
  }
  case COMPTIME_VALUE_FUNCTION:
    return a->function.body == b->function.body &&
           a->function.captured_env == b->function.captured_env;
  case COMPTIME_VALUE_PACK: {
    size_t ac = a->pack.elements ? vec_get_size(a->pack.elements) : 0;
    size_t bc = b->pack.elements ? vec_get_size(b->pack.elements) : 0;
    if (ac != bc) return false;
    for (size_t i = 0; i < ac; i++) {
      comptime_value_t ea = (comptime_value_t)vec_get(a->pack.elements, i);
      comptime_value_t eb = (comptime_value_t)vec_get(b->pack.elements, i);
      if (!comptime_value_equals(ea, eb)) return false;
    }
    return true;
  }
  case COMPTIME_VALUE_ERROR:  return true;
  default:                    return false;
  }
}

comptime_value_t comptime_value_clone(allocator_t allocator,
                                      comptime_value_t src) {
  if (!src) return NULL;
  return (comptime_value_t)value_clone(allocator, src);
}

/* ===== conversions ===== */

int64_t comptime_value_as_i64(comptime_value_t val) {
  if (!val) return 0;
  switch (val->kind) {
  case COMPTIME_VALUE_INT:   return val->int_val.is_signed ? val->int_val.s
                                                         : (int64_t)val->int_val.u;
  case COMPTIME_VALUE_FLOAT: return (int64_t)val->float_val.value;
  case COMPTIME_VALUE_BOOL:  return val->bool_val ? 1 : 0;
  case COMPTIME_VALUE_CHAR:  return (int64_t)val->char_val;
  default:                   return 0;
  }
}

uint64_t comptime_value_as_u64(comptime_value_t val) {
  if (!val) return 0;
  switch (val->kind) {
  case COMPTIME_VALUE_INT:   return val->int_val.is_signed ? (uint64_t)val->int_val.s
                                                         : val->int_val.u;
  case COMPTIME_VALUE_FLOAT: return (uint64_t)val->float_val.value;
  case COMPTIME_VALUE_BOOL:  return val->bool_val ? 1 : 0;
  case COMPTIME_VALUE_CHAR:  return (uint64_t)val->char_val;
  default:                   return 0;
  }
}

double comptime_value_as_f64(comptime_value_t val) {
  if (!val) return 0.0;
  switch (val->kind) {
  case COMPTIME_VALUE_INT:
    return val->int_val.is_signed ? (double)val->int_val.s
                               : (double)val->int_val.u;
  case COMPTIME_VALUE_FLOAT: return val->float_val.value;
  case COMPTIME_VALUE_BOOL:  return val->bool_val ? 1.0 : 0.0;
  default:                   return 0.0;
  }
}

const char *comptime_value_get_string(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_STRING || !val->string_val)
    return NULL;
  return string_get(val->string_val);
}

/* ===== union tag helpers ===== */

bool comptime_value_is_tagged_union(comptime_value_t comp) {
  if (!comp || !comp->type) return false;
  semantic_type_t unq = semantic_type_strip_qualifier(comp->type);
  if (unq->impl->kind == TYPE_UNION) return true;
  if (unq->impl->kind == TYPE_GENERIC_INSTANCE) {
    semantic_type_t base = unq->impl->generic_instance.generic_template;
    if (base && base->impl->kind == TYPE_UNION) return true;
  }
  return false;
}

/**
 * @brief Read the union tag from a composite value's data buffer.
 *        Tag is stored at offset 0 as a uint64_t.
 *        Returns 0 if the composite is not a tagged union or has no data.
 */
uint64_t comptime_value_get_union_tag(comptime_value_t comp) {
  if (!comptime_value_is_tagged_union(comp)) return 0;
  if (!comp->composite.data || comp->composite.data_size < 8) return 0;
  uint64_t tag;
  memcpy(&tag, comp->composite.data, 8);
  return tag;
}

/**
 * @brief Write the union tag into a composite value's data buffer.
 *        Tag is stored at offset 0 as a uint64_t.
 */
bool comptime_value_set_union_tag(comptime_value_t comp, uint64_t tag) {
  if (!comptime_value_is_tagged_union(comp)) return false;
  if (!comp->composite.data || comp->composite.data_size < 8) return false;
  memcpy(comp->composite.data, &tag, 8);
  return true;
}

/* ===== raw byte field read/write ===== */

comptime_value_t comptime_value_read_field(comptime_value_t composite,
                                           size_t offset,
                                           semantic_type_t field_type,
                                           allocator_t allocator) {
  if (!composite || !composite->composite.data || !field_type)
    return NULL;
  if (offset + field_type->impl->size > composite->composite.data_size)
    return NULL;
  uint8_t *ptr = composite->composite.data + offset;
  enum type_kind fk = field_type->impl->kind;
  switch (fk) {
  case TYPE_BOOL:
    return comptime_value_create_bool(allocator, *(bool *)ptr, field_type);
  case TYPE_I8:
    return comptime_value_create_int(allocator, (int64_t)(*(int8_t *)ptr),
                                     (uint64_t)(*(uint8_t *)ptr), 8, true, field_type);
  case TYPE_I16:
    return comptime_value_create_int(allocator, (int64_t)(*(int16_t *)ptr),
                                     (uint64_t)(*(uint16_t *)ptr), 16, true, field_type);
  case TYPE_I32:
    return comptime_value_create_int(allocator, (int64_t)(*(int32_t *)ptr),
                                     (uint64_t)(*(uint32_t *)ptr), 32, true, field_type);
  case TYPE_I64:
    return comptime_value_create_int(allocator, *(int64_t *)ptr,
                                     *(uint64_t *)ptr, 64, true, field_type);
  case TYPE_U8:
    return comptime_value_create_int(allocator, (int64_t)(*(uint8_t *)ptr),
                                     (uint64_t)(*(uint8_t *)ptr), 8, false, field_type);
  case TYPE_U16:
    return comptime_value_create_int(allocator, (int64_t)(*(uint16_t *)ptr),
                                     (uint64_t)(*(uint16_t *)ptr), 16, false, field_type);
  case TYPE_U32:
    return comptime_value_create_int(allocator, (int64_t)(*(uint32_t *)ptr),
                                     (uint64_t)(*(uint32_t *)ptr), 32, false, field_type);
  case TYPE_U64:
    return comptime_value_create_int(allocator, (int64_t)(*(uint64_t *)ptr),
                                     *(uint64_t *)ptr, 64, false, field_type);
  case TYPE_F32:
    return comptime_value_create_float(allocator, (double)(*(float *)ptr),
                                       32, field_type);
  case TYPE_F64:
    return comptime_value_create_float(allocator, *(double *)ptr, 64, field_type);
  case TYPE_CHAR:
    return comptime_value_create_char(allocator, *(char *)ptr, field_type);
  case TYPE_POINTER:
    return comptime_value_create_pointer(allocator, *(uint64_t *)ptr, field_type);
  case TYPE_STRING:
  case TYPE_STR: {
    /* string_t stored as pointer in 8 bytes */
    string_t s = *(string_t *)(void *)ptr;
    return comptime_value_create_string(allocator,
                                        s ? string_get(s) : NULL, field_type);
  }
  default:
    return NULL;
  }
}

bool comptime_value_write_field(comptime_value_t composite,
                                size_t offset,
                                semantic_type_t field_type,
                                comptime_value_t value,
                                allocator_t allocator) {
  if (!composite || !composite->composite.data || !value)
    return false;

  /* Determine write size from the target field type */
  size_t field_size = 0;
  if (field_type && field_type->impl) {
    type_layout_compute(field_type, 8);
    field_size = field_type->impl->size;
  }

  /* Boundary check */
  if (field_size > 0 && offset + field_size > composite->composite.data_size)
    return false;

  uint8_t *ptr = composite->composite.data + offset;
  switch (value->kind) {
  case COMPTIME_VALUE_BOOL:
    *(bool *)ptr = value->bool_val;
    break;
  case COMPTIME_VALUE_INT: {
    size_t sz = field_size > 0 ? field_size : (value->int_val.width / 8);
    if (sz == 0) sz = 1;
    if (sz > 8) sz = 8;
    if (value->int_val.is_signed) {
      int64_t sv = value->int_val.s;
      memcpy(ptr, &sv, sz);
    } else {
      uint64_t uv = value->int_val.u;
      memcpy(ptr, &uv, sz);
    }
    break;
  }
  case COMPTIME_VALUE_FLOAT: {
    if (field_size == 4 || value->float_val.width == 32) {
      float f = (float)value->float_val.value;
      memcpy(ptr, &f, 4);
    } else {
      memcpy(ptr, &value->float_val.value, 8);
    }
    break;
  }
  case COMPTIME_VALUE_CHAR:
    *(char *)ptr = value->char_val;
    break;
  case COMPTIME_VALUE_POINTER:
    *(uint64_t *)ptr = value->pointer.addr;
    break;
  case COMPTIME_VALUE_STRING: {
    /* Deep-copy the string_t object: allocate a new string_t and copy content,
     * then store the new pointer. This avoids sharing string_t between composites,
     * which would cause use-after-free during disposal. */
    string_t new_str = NULL;
    if (value->string_val && allocator) {
      new_str = (string_t)allocator_create(allocator, &g_string_type, NULL);
      string_set(new_str, string_get(value->string_val));
    }
    *(string_t *)(void *)ptr = new_str;
    break;
  }
  case COMPTIME_VALUE_NIL:
    memset(ptr, 0, field_size > 0 ? field_size : 8);
    break;
  case COMPTIME_VALUE_COMPOSITE:
    if (value->composite.data && value->composite.data_size > 0) {
      size_t copy_sz = value->composite.data_size;
      if (field_size > 0 && copy_sz > field_size) copy_sz = field_size;
      memcpy(ptr, value->composite.data, copy_sz);
    }
    break;
  default:
    return false;
  }

  /* After successful write, update union tag if writing to a tagged union.
   * The tag is the type hash of the field type being written, stored at
   * offset 0 in the data buffer. */
  if (comptime_value_is_tagged_union(composite) && field_type) {
    type_hash_ensure(field_type);
    comptime_value_set_union_tag(composite, field_type->impl->hash);
  }

  return true;
}

/* ===== named field access (struct) ===== */

comptime_value_t comptime_value_get_field(comptime_value_t composite,
                                          const char *field_name,
                                          allocator_t allocator) {
  if (!composite || !composite->type || !field_name) return NULL;
  type_impl_t impl = composite->type->impl;
  if (!impl) return NULL;
  vec_t fields = NULL;
  if (impl->kind == TYPE_STRUCT || impl->kind == TYPE_UNION ||
      impl->kind == TYPE_CUNION)
    fields = impl->struct_type.fields;
  else if (impl->kind == TYPE_GENERIC_INSTANCE)
    fields = impl->generic_instance.fields;
  if (!fields) return NULL;
  size_t fc = vec_get_size(fields);
  for (size_t i = 0; i < fc; i++) {
    struct symbol *s = (struct symbol *)vec_get(fields, i);
    if (s && s->name && strcmp(s->name, field_name) == 0)
      return comptime_value_read_field(composite, s->field.offset,
                                       s->field.type, allocator);
  }
  return NULL;
}

bool comptime_value_set_field(comptime_value_t composite,
                              const char *field_name,
                              comptime_value_t value,
                              allocator_t allocator) {
  if (!composite || !composite->type || !field_name) return false;
  type_impl_t impl = composite->type->impl;
  if (!impl) return false;
  vec_t fields = NULL;
  if (impl->kind == TYPE_STRUCT || impl->kind == TYPE_UNION ||
      impl->kind == TYPE_CUNION)
    fields = impl->struct_type.fields;
  else if (impl->kind == TYPE_GENERIC_INSTANCE)
    fields = impl->generic_instance.fields;
  if (!fields) return false;
  size_t fc = vec_get_size(fields);
  for (size_t i = 0; i < fc; i++) {
    struct symbol *s = (struct symbol *)vec_get(fields, i);
    if (s && s->name && strcmp(s->name, field_name) == 0)
      return comptime_value_write_field(composite, s->field.offset, s->field.type, value, allocator);
  }
  return false;
}

/* ===== index access (array) ===== */

comptime_value_t comptime_value_get_index(comptime_value_t composite,
                                          size_t index,
                                          allocator_t allocator) {
  if (!composite || !composite->composite.data) return NULL;
  semantic_type_t elem_type = composite->composite.element_type;
  if (!elem_type) return NULL;
  size_t elem_size = elem_type->impl ? elem_type->impl->size : 0;
  if (elem_size == 0) return NULL;
  size_t offset = index * elem_size;
  if (offset + elem_size > composite->composite.data_size) return NULL;
  return comptime_value_read_field(composite, offset, elem_type, allocator);
}

bool comptime_value_set_index(comptime_value_t composite,
                              size_t index,
                              comptime_value_t value,
                              allocator_t allocator) {
  if (!composite || !composite->composite.data) return false;
  semantic_type_t elem_type = composite->composite.element_type;
  if (!elem_type) return false;
  size_t elem_size = elem_type->impl ? elem_type->impl->size : 0;
  if (elem_size == 0) return false;
  size_t offset = index * elem_size;
  if (offset + elem_size > composite->composite.data_size) return false;
  return comptime_value_write_field(composite, offset, elem_type, value, allocator);
}
