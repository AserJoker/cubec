#include "engine/float.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

double float_get_value(value_t self) {
  double val = 0;
  type_t value_type = value_get_type(self);
  if (type_get_size(value_type) == sizeof(_Float16)) {
    val = *(_Float16 *)value_get_data(self);
  } else if (type_get_size(value_type) == sizeof(float)) {
    val = *(float *)value_get_data(self);
  } else if (type_get_size(value_type) == sizeof(double)) {
    val = *(double *)value_get_data(self);
  }
  return val;
}
value_t create_comptime_float(context_t ctx, type_t type, double val) {
  if (type_get_kind(type) == TYPE_KIND_INTEGER) {
    if (type_get_size(type) == sizeof(_Float16)) {
      return create_comptime_f16(ctx, val, false, NULL);
    } else if (type_get_size(type) == sizeof(float)) {
      return create_comptime_f32(ctx, val, false, NULL);
    } else if (type_get_size(type) == sizeof(double)) {
      return create_comptime_f64(ctx, val, false, NULL);
    }
  }
  return create_error(ctx, "invalid type %s", type_get_name(type));
}

value_t create_float(context_t ctx, type_t type) {
  if (type_get_kind(type) == TYPE_KIND_INTEGER) {
    if (type_get_size(type) == sizeof(_Float16)) {
      return create_f16(ctx, false, NULL);
    } else if (type_get_size(type) == sizeof(float)) {
      return create_f32(ctx, false, NULL);
    } else if (type_get_size(type) == sizeof(double)) {
      return create_f64(ctx, false, NULL);
    }
  }
  return create_error(ctx, "invalid type %s", type_get_name(type));
}

static value_t float_safe_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (value_is_comptime(self)) {
    double val = float_get_value(self);
    if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      return create_comptime_float(ctx, type, val);
    }
  } else {
    if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      return create_float(ctx, type);
    }
  }
  return create_error(ctx, "cannot convert '%s' to '%s'",
                      type_get_name(value_type), type_get_name(type));
}

static value_t float_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (value_is_comptime(self)) {
    double val = float_get_value(self);
    if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      return create_comptime_integer(ctx, type, val);
    } else if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
      return create_comptime_unsigned(ctx, type, val);
    } else if (type_get_kind(type) == TYPE_KIND_FLOAT) {
      return create_comptime_float(ctx, type, val);
    } else if (type_get_kind(type) == TYPE_KIND_BOOL) {
      return create_comptime_bool(ctx, val != 0, false, NULL);
    }
  } else {
    if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      return create_integer(ctx, type);
    } else if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
      return create_unsigned(ctx, type);
    } else if (type_get_kind(type) == TYPE_KIND_FLOAT) {
      return create_float(ctx, type);
    } else if (type_get_kind(type) == TYPE_KIND_BOOL) {
      return create_bool(ctx, false, NULL);
    }
  }
  return create_error(ctx, "cannot convert '%s' to '%s'",
                      type_get_name(value_type), type_get_name(type));
}
static value_t float_add(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_INTEGER) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (type_get_size(left_type) < type_get_size(right_type)) {
    self = value_safe_convert(self, ctx, right_type);
    if (value_is_error(self)) {
      return self;
    }
    left_type = value_get_type(self);
  } else if (type_get_size(left_type) > type_get_size(right_type)) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    double left = float_get_value(self);
    double right = float_get_value(another);
    double val = left + right;
    return create_comptime_float(ctx, left_type, val);
  } else {
    return create_float(ctx, left_type);
  }
}
static value_t float_sub(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_INTEGER) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (type_get_size(left_type) < type_get_size(right_type)) {
    self = value_safe_convert(self, ctx, right_type);
    if (value_is_error(self)) {
      return self;
    }
    left_type = value_get_type(self);
  } else if (type_get_size(left_type) > type_get_size(right_type)) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    double left = float_get_value(self);
    double right = float_get_value(another);
    double val = left - right;
    return create_comptime_float(ctx, left_type, val);
  } else {
    return create_float(ctx, left_type);
  }
}
static value_t float_mul(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_INTEGER) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (type_get_size(left_type) < type_get_size(right_type)) {
    self = value_safe_convert(self, ctx, right_type);
    if (value_is_error(self)) {
      return self;
    }
    left_type = value_get_type(self);
  } else if (type_get_size(left_type) > type_get_size(right_type)) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    double left = float_get_value(self);
    double right = float_get_value(another);
    double val = left * right;
    return create_comptime_float(ctx, left_type, val);
  } else {
    return create_float(ctx, left_type);
  }
}
static value_t float_div(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_INTEGER) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (type_get_size(left_type) < type_get_size(right_type)) {
    self = value_safe_convert(self, ctx, right_type);
    if (value_is_error(self)) {
      return self;
    }
    left_type = value_get_type(self);
  } else if (type_get_size(left_type) > type_get_size(right_type)) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    double left = float_get_value(self);
    double right = float_get_value(another);
    double val = left / right;
    return create_comptime_float(ctx, left_type, val);
  } else {
    return create_float(ctx, left_type);
  }
}
static value_t float_eq(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_INTEGER) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (type_get_size(left_type) < type_get_size(right_type)) {
    self = value_safe_convert(self, ctx, right_type);
    if (value_is_error(self)) {
      return self;
    }
    left_type = value_get_type(self);
  } else if (type_get_size(left_type) > type_get_size(right_type)) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    double left = float_get_value(self);
    double right = float_get_value(another);
    double val = left == right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t float_ne(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_INTEGER) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (type_get_size(left_type) < type_get_size(right_type)) {
    self = value_safe_convert(self, ctx, right_type);
    if (value_is_error(self)) {
      return self;
    }
    left_type = value_get_type(self);
  } else if (type_get_size(left_type) > type_get_size(right_type)) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    double left = float_get_value(self);
    double right = float_get_value(another);
    double val = left != right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t float_gt(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_INTEGER) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (type_get_size(left_type) < type_get_size(right_type)) {
    self = value_safe_convert(self, ctx, right_type);
    if (value_is_error(self)) {
      return self;
    }
    left_type = value_get_type(self);
  } else if (type_get_size(left_type) > type_get_size(right_type)) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    double left = float_get_value(self);
    double right = float_get_value(another);
    double val = left > right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t float_ge(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_INTEGER) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (type_get_size(left_type) < type_get_size(right_type)) {
    self = value_safe_convert(self, ctx, right_type);
    if (value_is_error(self)) {
      return self;
    }
    left_type = value_get_type(self);
  } else if (type_get_size(left_type) > type_get_size(right_type)) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    double left = float_get_value(self);
    double right = float_get_value(another);
    double val = left >= right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t float_lt(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_INTEGER) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (type_get_size(left_type) < type_get_size(right_type)) {
    self = value_safe_convert(self, ctx, right_type);
    if (value_is_error(self)) {
      return self;
    }
    left_type = value_get_type(self);
  } else if (type_get_size(left_type) > type_get_size(right_type)) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    double left = float_get_value(self);
    double right = float_get_value(another);
    double val = left < right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t float_le(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_INTEGER) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (type_get_size(left_type) < type_get_size(right_type)) {
    self = value_safe_convert(self, ctx, right_type);
    if (value_is_error(self)) {
      return self;
    }
    left_type = value_get_type(self);
  } else if (type_get_size(left_type) > type_get_size(right_type)) {
    another = value_safe_convert(another, ctx, left_type);
    if (value_is_error(another)) {
      return another;
    }
    right_type = value_get_type(another);
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    double left = float_get_value(self);
    double right = float_get_value(another);
    double val = left <= right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}
static value_t float_plus(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  if (value_is_comptime(self)) {
    double val = float_get_value(self);
    return create_comptime_float(ctx, type, +val);
  } else {
    return create_float(ctx, type);
  }
}
static value_t float_negtive(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  if (value_is_comptime(self)) {
    double val = float_get_value(self);
    return create_comptime_float(ctx, type, -val);
  } else {
    return create_float(ctx, type);
  }
}

static char *float_write_ast(value_t self, allocator_t allocator) {
  type_t type = value_get_type(self);
  double val = float_get_value(self);
  char s[32];
  if (type_get_size(type) == sizeof(_Float16)) {
    sprintf(s, "%g@f16", val);
  } else if (type_get_size(type) == sizeof(float)) {
    sprintf(s, "%g@f32", val);
  } else {
    sprintf(s, "%g@f32", val);
  }
  return create_cstring(allocator, s);
}

void float_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  struct _type_operator_t opt = {
      .convert = float_convert,
      .safe_convert = float_safe_convert,
      .add = float_add,
      .sub = float_sub,
      .mul = float_mul,
      .div = float_div,
      .eq = float_eq,
      .ne = float_ne,
      .gt = float_gt,
      .ge = float_ge,
      .lt = float_lt,
      .le = float_le,
      .plus = float_plus,
      .neg = float_negtive,
      .addr_of = value_default_address_of,
      .assigment = value_default_assigment,
      .write_ast = float_write_ast,
  };
  type_t f16_t = create_type(allocator, TYPE_KIND_INTEGER, sizeof(_Float16),
                             sizeof(_Float16), "f16", "f16", &opt, NULL);
  create_type_value(ctx, f16_t, false, true, "f16");
  context_store_type(ctx, f16_t);
  type_t f32_t = create_type(allocator, TYPE_KIND_INTEGER, sizeof(float),
                             sizeof(float), "f32", "f32", &opt, NULL);
  create_type_value(ctx, f32_t, false, true, "f32");
  context_store_type(ctx, f32_t);
  type_t f64_t = create_type(allocator, TYPE_KIND_INTEGER, sizeof(double),
                             sizeof(double), "f64", "f64", &opt, NULL);
  create_type_value(ctx, f64_t, false, true, "f64");
  context_store_type(ctx, f64_t);
}
value_t create_f16(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "f16");
  return context_create_value(ctx, type, NULL, mut, false, name);
}
value_t create_comptime_f16(context_t ctx, _Float16 val, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "f16");
  return context_create_value(ctx, type, &val, mut, true, name);
}
value_t create_f32(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "f32");
  return context_create_value(ctx, type, NULL, mut, false, name);
}
value_t create_comptime_f32(context_t ctx, float val, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "f32");
  return context_create_value(ctx, type, &val, mut, true, name);
}
value_t create_f64(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "f64");
  return context_create_value(ctx, type, NULL, mut, false, name);
}
value_t create_comptime_f64(context_t ctx, double val, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "f64");
  return context_create_value(ctx, type, &val, mut, true, name);
}