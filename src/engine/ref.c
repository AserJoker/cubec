#include "engine/ref.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
typedef struct _ref_meta_t *ref_meta_t;
struct _ref_meta_t {
  type_t type;
};
static ref_meta_t create_ref_meta(allocator_t allocator, type_t type) {
  ref_meta_t self =
      allocator_alloc(allocator, sizeof(struct _ref_meta_t), NULL);
  self->type = type;
  return self;
}

static value_t ref_addr_of(value_t self, context_t ctx) {
  value_t base = ref_get_value(ctx, self);
  return value_addr_of(base, ctx);
}

static value_t ref_ref(value_t self, context_t ctx) {
  value_t base = ref_get_value(ctx, self);
  return value_ref(base, ctx);
}
static value_t ref_deref(value_t self, context_t ctx) {
  value_t base = ref_get_value(ctx, self);
  return value_deref(base, ctx);
}

static value_t ref_plus(value_t self, context_t ctx) {
  value_t base = ref_get_value(ctx, self);
  return value_plus(base, ctx);
}
static value_t ref_neg(value_t self, context_t ctx) {
  value_t base = ref_get_value(ctx, self);
  return value_neg(base, ctx);
}
static value_t ref_logical_not(value_t self, context_t ctx) {
  value_t base = ref_get_value(ctx, self);
  return value_logical_not(base, ctx);
}
static value_t ref_bitwise_not(value_t self, context_t ctx) {
  value_t base = ref_get_value(ctx, self);
  return value_bitwise_not(base, ctx);
}

static value_t ref_add(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_add(base, ctx, another);
}
static value_t ref_sub(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_sub(base, ctx, another);
}
static value_t ref_mul(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_mul(base, ctx, another);
}
static value_t ref_div(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_div(base, ctx, another);
}
static value_t ref_mod(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_mod(base, ctx, another);
}
static value_t ref_and(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_and(base, ctx, another);
}
static value_t ref_or(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_or(base, ctx, another);
}
static value_t ref_xor(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_xor(base, ctx, another);
}
static value_t ref_shl(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_shl(base, ctx, another);
}
static value_t ref_shr(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_shr(base, ctx, another);
}
static value_t ref_eq(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_eq(base, ctx, another);
}
static value_t ref_ne(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_ne(base, ctx, another);
}
static value_t ref_gt(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_gt(base, ctx, another);
}
static value_t ref_ge(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_ge(base, ctx, another);
}
static value_t ref_lt(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_lt(base, ctx, another);
}
static value_t ref_le(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_le(base, ctx, another);
}
static value_t ref_logical_and(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_logical_and(base, ctx, another);
}
static value_t ref_logical_or(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_logical_or(base, ctx, another);
}
static value_t ref_assigment(value_t self, context_t ctx, value_t another) {
  value_t base = ref_get_value(ctx, self);
  return value_assigment(base, ctx, another);
}

static value_t ref_get_field(value_t self, context_t ctx, const char *name) {
  value_t base = ref_get_value(ctx, self);
  return value_get_field(base, ctx, name);
}
static value_t ref_set_field(value_t self, context_t ctx, const char *name,
                             value_t value) {
  value_t base = ref_get_value(ctx, self);
  return value_set_field(base, ctx, name, value);
}

static value_t ref_get_index(value_t self, context_t ctx, size_t idx) {
  value_t base = ref_get_value(ctx, self);
  return value_get_index(base, ctx, idx);
}
static value_t ref_set_index(value_t self, context_t ctx, size_t idx,
                             value_t value) {
  value_t base = ref_get_value(ctx, self);
  return value_set_index(base, ctx, idx, value);
}

static value_t ref_get_length(value_t self, context_t ctx) {
  value_t base = ref_get_value(ctx, self);
  return value_get_length(base, ctx);
}

static value_t ref_call(value_t self, context_t ctx, size_t argc,
                        value_t argv[]) {
  value_t base = ref_get_value(ctx, self);
  return value_call(base, ctx, argc, argv);
}

static value_t ref_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (strcmp(type_get_id(value_type), type_get_id(type)) == 0) {
    return value_clone(self, ctx);
  }
  value_t base = ref_get_value(ctx, self);
  return value_convert(base, ctx, type);
}
static value_t ref_safe_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (strcmp(type_get_id(value_type), type_get_id(type)) == 0) {
    return value_clone(self, ctx);
  }
  value_t base = ref_get_value(ctx, self);
  return value_safe_convert(base, ctx, type);
}

type_t create_ref_type(context_t ctx, type_t type) {
  if (type_get_kind(type) == TYPE_KIND_REF) {
    return type;
  }
  const char *base_name = type_get_name(type);
  size_t len = snprintf(NULL, 0, "&%s", base_name);
  char buf[len + 1];
  sprintf(buf, "&%s", base_name);
  type_t ref_type = context_load_type(ctx, buf);
  if (!ref_type) {
    allocator_t allocator = context_get_allocator(ctx);
    ref_meta_t meta = create_ref_meta(allocator, type);
    type_operator_t opt = {
        .addr_of = ref_addr_of,
        .ref = ref_ref,
        .deref = ref_deref,
        .plus = ref_plus,
        .neg = ref_neg,
        .logical_not = ref_logical_not,
        .bitwise_not = ref_bitwise_not,

        .add = ref_add,
        .sub = ref_sub,
        .mul = ref_mul,
        .div = ref_div,
        .mod = ref_mod,
        .and_ = ref_and,
        .or_ = ref_or,
        .xor_ = ref_xor,
        .shl = ref_shl,
        .shr = ref_shr,
        .eq = ref_eq,
        .ne = ref_ne,
        .gt = ref_gt,
        .ge = ref_ge,
        .lt = ref_lt,
        .le = ref_le,
        .logical_and = ref_logical_and,
        .logical_or = ref_logical_or,
        .assigment = ref_assigment,
        .get_field = ref_get_field,
        .set_field = ref_set_field,
        .get_index = ref_get_index,
        .set_index = ref_set_index,
        .get_length = ref_get_length,
        .call = ref_call,
        .convert = ref_convert,
        .safe_convert = ref_safe_convert,
    };
    ref_type = create_type(allocator, TYPE_KIND_REF, sizeof(void *),
                           sizeof(void *), buf, buf, &opt, meta);
    context_store_type(ctx, ref_type);
  }
  return ref_type;
}
type_t ref_type_get_type(type_t self) {
  ref_meta_t meta = type_get_meta(self);
  return meta->type;
}

value_t create_ref_value(context_t ctx, value_t value) {
  type_t type = value_get_type(value);
  if (type_get_kind(type) == TYPE_KIND_REF) {
    return value_clone(value, ctx);
  }
  allocator_t allocator = context_get_allocator(ctx);
  type_t ref_type = create_ref_type(ctx, type);
  bool mut = value_is_mutable(value);
  bool comptime = value_is_comptime(value);
  if (value_is_comptime(value)) {
    const void *data = value_get_data(value);
    return context_create_value(ctx, ref_type, &data, mut, comptime, NULL);
  } else {
    return context_create_value(ctx, ref_type, NULL, mut, comptime, NULL);
  }
}
value_t ref_get_value(context_t ctx, value_t self) {
  type_t ref_type = value_get_type(self);
  type_t type = ref_type_get_type(ref_type);
  bool mut = value_is_mutable(self);
  if (value_is_comptime(self)) {
    void *data = *(void **)value_get_data(self);
    return context_create_weak_value(ctx, type, data, mut, NULL);
  } else {
    return context_create_value(ctx, type, NULL, mut, false, NULL);
  }
}