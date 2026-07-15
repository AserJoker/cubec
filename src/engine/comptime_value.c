#include "engine/comptime_value.h"
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

static void _comptime_value_dispose(void *self, allocator_t allocator) {
  comptime_value_t v = (comptime_value_t)self;
  switch (v->kind) {
  case COMPTIME_VALUE_STRING:
    allocator_free(allocator, &v->string_val);
    break;
  case COMPTIME_VALUE_COMPOSITE:
    allocator_free(allocator, &v->composite.fields);
    if (v->composite.field_names) {
      for (size_t i = 0; i < v->composite.field_count; i++)
        allocator_free(allocator, (void **)&v->composite.field_names[i]);
      allocator_free(allocator, (void **)&v->composite.field_names);
    }
    break;
  case COMPTIME_VALUE_FUNCTION:
    /* env and body are not owned by the value */
    allocator_free(allocator, &v->function.param_names);
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
    if (src->composite.fields) {
      vec_init_t vi = {.auto_dispose = true};
      dst->composite.fields =
          (vec_t)allocator_create(allocator, &g_vec_type, &vi);
      size_t fc = vec_get_size(src->composite.fields);
      for (size_t i = 0; i < fc; i++) {
        comptime_value_t f = (comptime_value_t)vec_get(src->composite.fields, i);
        comptime_value_t cf = comptime_value_clone(allocator, f);
        vec_push(dst->composite.fields, cf);
      }
      if (src->composite.field_names && src->composite.field_count > 0) {
        dst->composite.field_names =
            (const char **)allocator_alloc(allocator,
                                           sizeof(const char *) * src->composite.field_count);
        for (size_t i = 0; i < src->composite.field_count; i++)
          dst->composite.field_names[i] = src->composite.field_names[i];
      }
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
                                                  vec_t fields,
                                                  const char **field_names,
                                                  size_t field_count) {
  struct comptime_value init = {
      .kind = COMPTIME_VALUE_COMPOSITE,
      .type = type,
      .composite = {.fields = fields,
                     .field_names = field_names,
                     .field_count = field_count}};
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
    if (a->int_val.is_signed && b->int_val.is_signed)
      return a->int_val.s == b->int_val.s;
    if (!a->int_val.is_signed && !b->int_val.is_signed)
      return a->int_val.u == b->int_val.u;
    return false;
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
    if (a->composite.field_count != b->composite.field_count) return false;
    if (!a->composite.fields && !b->composite.fields) return true;
    if (!a->composite.fields || !b->composite.fields) return false;
    size_t fc = vec_get_size(a->composite.fields);
    if (fc != vec_get_size(b->composite.fields)) return false;
    for (size_t i = 0; i < fc; i++) {
      comptime_value_t fa = (comptime_value_t)vec_get(a->composite.fields, i);
      comptime_value_t fb = (comptime_value_t)vec_get(b->composite.fields, i);
      if (!comptime_value_equals(fa, fb)) return false;
    }
    return true;
  }
  case COMPTIME_VALUE_FUNCTION:
    return a->function.body == b->function.body &&
           a->function.captured_env == b->function.captured_env;
  case COMPTIME_VALUE_ERROR:  return true;
  default:                    return false;
  }
}

comptime_value_t comptime_value_clone(allocator_t allocator,
                                      comptime_value_t src) {
  if (!src) return NULL;
  return (comptime_value_t)allocator_create(allocator, &g_comptime_value_type,
                                             src);
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
