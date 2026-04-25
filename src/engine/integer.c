#include "engine/integer.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/float.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

int64_t integer_get_value(value_t self) {
  int64_t val = 0;
  type_t value_type = value_get_type(self);
  if (type_get_size(value_type) == sizeof(int8_t)) {
    val = *(int8_t *)value_get_data(self);
  } else if (type_get_size(value_type) == sizeof(int16_t)) {
    val = *(int16_t *)value_get_data(self);
  } else if (type_get_size(value_type) == sizeof(int32_t)) {
    val = *(int32_t *)value_get_data(self);
  } else if (type_get_size(value_type) == sizeof(int64_t)) {
    val = *(int64_t *)value_get_data(self);
  }
  return val;
}
value_t create_comptime_integer(context_t ctx, type_t type, int64_t val) {
  if (type_get_kind(type) == TYPE_KIND_INTEGER) {
    if (type_get_size(type) == sizeof(int8_t)) {
      return create_comptime_i8(ctx, val, false, NULL);
    } else if (type_get_size(type) == sizeof(int16_t)) {
      return create_comptime_i16(ctx, val, false, NULL);
    } else if (type_get_size(type) == sizeof(int32_t)) {
      return create_comptime_i32(ctx, val, false, NULL);
    } else if (type_get_size(type) == sizeof(int64_t)) {
      return create_comptime_i64(ctx, val, false, NULL);
    }
  }
  return create_error(ctx, "invalid type %s", type_get_name(type));
}

value_t create_integer(context_t ctx, type_t type) {
  if (type_get_kind(type) == TYPE_KIND_INTEGER) {
    if (type_get_size(type) == sizeof(int8_t)) {
      return create_i8(ctx, false, NULL);
    } else if (type_get_size(type) == sizeof(int16_t)) {
      return create_i16(ctx, false, NULL);
    } else if (type_get_size(type) == sizeof(int32_t)) {
      return create_i32(ctx, false, NULL);
    } else if (type_get_size(type) == sizeof(int64_t)) {
      return create_i64(ctx, false, NULL);
    }
  }
  return create_error(ctx, "invalid type %s", type_get_name(type));
}

static value_t integer_safe_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (value_is_comptime(self)) {
    int64_t val = integer_get_value(self);
    if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      return create_comptime_integer(ctx, type, val);
    }
  } else {
    if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      return create_integer(ctx, type);
    }
  }
  return create_error(ctx, "cannot convert '%s' to '%s'",
                      type_get_name(value_type), type_get_name(type));
}

static value_t integer_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (value_is_comptime(self)) {
    int64_t val = integer_get_value(self);
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
static value_t integer_add(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left + right;
    return create_comptime_integer(ctx, left_type, val);
  } else {
    return create_integer(ctx, left_type);
  }
}
static value_t integer_sub(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left - right;
    return create_comptime_integer(ctx, left_type, val);
  } else {
    return create_integer(ctx, left_type);
  }
}
static value_t integer_mul(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left * right;
    return create_comptime_integer(ctx, left_type, val);
  } else {
    return create_integer(ctx, left_type);
  }
}
static value_t integer_div(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left / right;
    return create_comptime_integer(ctx, left_type, val);
  } else {
    return create_integer(ctx, left_type);
  }
}
static value_t integer_mod(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left % right;
    return create_comptime_integer(ctx, left_type, val);
  } else {
    return create_integer(ctx, left_type);
  }
}
static value_t integer_and(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left & right;
    return create_comptime_integer(ctx, left_type, val);
  } else {
    return create_integer(ctx, left_type);
  }
}
static value_t integer_or(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left | right;
    return create_comptime_integer(ctx, left_type, val);
  } else {
    return create_integer(ctx, left_type);
  }
}
static value_t integer_xor(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left ^ right;
    return create_comptime_integer(ctx, left_type, val);
  } else {
    return create_integer(ctx, left_type);
  }
}
static value_t integer_shl(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left << right;
    return create_comptime_integer(ctx, left_type, val);
  } else {
    return create_integer(ctx, left_type);
  }
}
static value_t integer_shr(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left >> right;
    return create_comptime_integer(ctx, left_type, val);
  } else {
    return create_integer(ctx, left_type);
  }
}
static value_t integer_eq(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left == right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t integer_ne(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left != right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t integer_gt(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left > right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t integer_ge(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left >= right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t integer_lt(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left < right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t integer_le(value_t self, context_t ctx, value_t another) {
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
    int64_t left = integer_get_value(self);
    int64_t right = integer_get_value(another);
    int64_t val = left <= right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}
static value_t integer_bitwise_not(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  if (value_is_comptime(self)) {
    int64_t val = integer_get_value(self);
    return create_comptime_integer(ctx, type, ~val);
  } else {
    return create_integer(ctx, type);
  }
}
static value_t integer_plus(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  if (value_is_comptime(self)) {
    int64_t val = integer_get_value(self);
    return create_comptime_integer(ctx, type, +val);
  } else {
    return create_integer(ctx, type);
  }
}
static value_t integer_negtive(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  if (value_is_comptime(self)) {
    int64_t val = integer_get_value(self);
    return create_comptime_integer(ctx, type, -val);
  } else {
    return create_integer(ctx, type);
  }
}

static char *integer_write_ast(value_t self, allocator_t allocator) {
  int64_t val = integer_get_value(self);
  type_t type = value_get_type(self);
  char s[32];
  if (type_get_size(type) == sizeof(int8_t)) {
    sprintf(s, "%" PRIiPTR "@i8", val);
  } else if (type_get_size(type) == sizeof(int16_t)) {
    sprintf(s, "%" PRIiPTR "@i16", val);
  } else if (type_get_size(type) == sizeof(int32_t)) {
    sprintf(s, "%" PRIiPTR "@i32", val);
  } else {
    sprintf(s, "%" PRIiPTR "@i64", val);
  }
  return create_cstring(allocator, s);
}

void integer_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  struct _type_operator_t opt = {
      .type_eq = type_default_eq,
      .convert = integer_convert,
      .safe_convert = integer_safe_convert,
      .add = integer_add,
      .sub = integer_sub,
      .mul = integer_mul,
      .div = integer_div,
      .mod = integer_mod,
      .and_ = integer_and,
      .or_ = integer_or,
      .xor_ = integer_xor,
      .shl = integer_shl,
      .shr = integer_shr,
      .eq = integer_eq,
      .ne = integer_ne,
      .gt = integer_gt,
      .ge = integer_ge,
      .lt = integer_lt,
      .le = integer_le,
      .bitwise_not = integer_bitwise_not,
      .plus = integer_plus,
      .neg = integer_negtive,
      .addr_of = value_default_address_of,
      .assigment = value_default_assigment,
      .write_ast = integer_write_ast,
  };
  type_t i8_t = create_type(allocator, TYPE_KIND_INTEGER, sizeof(int8_t),
                            sizeof(int8_t), "i8", "i8", &opt, NULL);
  create_type_value(ctx, i8_t, false, "i8");
  context_store_type(ctx, i8_t);
  type_t i16_t = create_type(allocator, TYPE_KIND_INTEGER, sizeof(int16_t),
                             sizeof(int16_t), "i16", "i16", &opt, NULL);
  create_type_value(ctx, i16_t, false, "i16");
  context_store_type(ctx, i16_t);
  type_t i32_t = create_type(allocator, TYPE_KIND_INTEGER, sizeof(int32_t),
                             sizeof(int32_t), "i32", "i32", &opt, NULL);
  create_type_value(ctx, i32_t, false, "i32");
  context_store_type(ctx, i32_t);
  type_t i64_t = create_type(allocator, TYPE_KIND_INTEGER, sizeof(int64_t),
                             sizeof(int64_t), "i64", "i64", &opt, NULL);
  create_type_value(ctx, i64_t, false, "i64");
  context_store_type(ctx, i64_t);
}
value_t create_i8(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "i8");
  return context_create_value(ctx, type, NULL, mut, false, name);
}
value_t create_comptime_i8(context_t ctx, int8_t val, bool mut,
                           const char *name) {
  type_t type = context_load_type(ctx, "i8");
  return context_create_value(ctx, type, &val, mut, true, name);
}
value_t create_i16(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "i16");
  return context_create_value(ctx, type, NULL, mut, false, name);
}
value_t create_comptime_i16(context_t ctx, int16_t val, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "i16");
  return context_create_value(ctx, type, &val, mut, true, name);
}
value_t create_i32(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "i32");
  return context_create_value(ctx, type, NULL, mut, false, name);
}
value_t create_comptime_i32(context_t ctx, int32_t val, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "i32");
  return context_create_value(ctx, type, &val, mut, true, name);
}
value_t create_i64(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "i64");
  return context_create_value(ctx, type, NULL, mut, false, name);
}
value_t create_comptime_i64(context_t ctx, int64_t val, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "i64");
  return context_create_value(ctx, type, &val, mut, true, name);
}