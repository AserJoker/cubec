#include "engine/unsigned.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/float.h"
#include "engine/integer.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

uint64_t unsigned_get_value(value_t self) {
  uint64_t val = 0;
  type_t value_type = value_get_type(self);
  if (type_get_size(value_type) == sizeof(uint8_t)) {
    val = *(uint8_t *)value_get_data(self);
  } else if (type_get_size(value_type) == sizeof(uint16_t)) {
    val = *(uint16_t *)value_get_data(self);
  } else if (type_get_size(value_type) == sizeof(uint32_t)) {
    val = *(uint32_t *)value_get_data(self);
  } else if (type_get_size(value_type) == sizeof(uint64_t)) {
    val = *(uint64_t *)value_get_data(self);
  }
  return val;
}
value_t create_comptime_unsigned(context_t ctx, type_t type, uint64_t val) {
  if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
    if (type_get_size(type) == sizeof(uint8_t)) {
      return create_comptime_u8(ctx, val, false, NULL);
    } else if (type_get_size(type) == sizeof(uint16_t)) {
      return create_comptime_u16(ctx, val, false, NULL);
    } else if (type_get_size(type) == sizeof(uint32_t)) {
      return create_comptime_u32(ctx, val, false, NULL);
    } else if (type_get_size(type) == sizeof(uint64_t)) {
      return create_comptime_u64(ctx, val, false, NULL);
    }
  }
  return create_error(ctx, "invalid type %s", type_get_name(type));
}

value_t create_unsigned(context_t ctx, type_t type) {
  if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
    if (type_get_size(type) == sizeof(int8_t)) {
      return create_u8(ctx, false, NULL);
    } else if (type_get_size(type) == sizeof(int16_t)) {
      return create_u16(ctx, false, NULL);
    } else if (type_get_size(type) == sizeof(int32_t)) {
      return create_u32(ctx, false, NULL);
    } else if (type_get_size(type) == sizeof(int64_t)) {
      return create_u64(ctx, false, NULL);
    }
  }
  return create_error(ctx, "invalid type %s", type_get_name(type));
}

static value_t unsigned_safe_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (value_is_comptime(self)) {
    uint64_t val = unsigned_get_value(self);
    if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
      return create_comptime_unsigned(ctx, type, val);
    }
  } else {
    if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
      return create_unsigned(ctx, type);
    }
  }
  return create_error(ctx, "cannot convert '%s' to '%s'",
                      type_get_name(value_type), type_get_name(type));
}

static char *unsigned_write_ast(value_t self, allocator_t allocator) {
  int64_t val = integer_get_value(self);
  type_t type = value_get_type(self);
  char s[32];
  if (type_get_size(type) == sizeof(uint8_t)) {
    sprintf(s, "%" PRIiPTR "@u8", val);
  } else if (type_get_size(type) == sizeof(uint16_t)) {
    sprintf(s, "%" PRIiPTR "@u16", val);
  } else if (type_get_size(type) == sizeof(uint32_t)) {
    sprintf(s, "%" PRIiPTR "@u32", val);
  } else {
    sprintf(s, "%" PRIiPTR "@u64", val);
  }
  return create_cstring(allocator, s);
}

static value_t unsigned_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (value_is_comptime(self)) {
    uint64_t val = unsigned_get_value(self);
    if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
      return create_comptime_unsigned(ctx, type, val);
    } else if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      return create_comptime_integer(ctx, type, val);
    } else if (type_get_kind(type) == TYPE_KIND_FLOAT) {
      return create_comptime_float(ctx, type, val);
    } else if (type_get_kind(type) == TYPE_KIND_BOOL) {
      return create_comptime_bool(ctx, val != 0, false, NULL);
    }
  } else {
    if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
      return create_unsigned(ctx, type);
    } else if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      return create_integer(ctx, type);
    } else if (type_get_kind(type) == TYPE_KIND_FLOAT) {
      return create_float(ctx, type);
    } else if (type_get_kind(type) == TYPE_KIND_BOOL) {
      return create_bool(ctx, false, NULL);
    }
  }
  return create_error(ctx, "cannot convert '%s' to '%s'",
                      type_get_name(value_type), type_get_name(type));
}
static value_t unsigned_add(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left + right;
    return create_comptime_unsigned(ctx, left_type, val);
  } else {
    return create_unsigned(ctx, left_type);
  }
}
static value_t unsigned_sub(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left - right;
    return create_comptime_unsigned(ctx, left_type, val);
  } else {
    return create_unsigned(ctx, left_type);
  }
}
static value_t unsigned_mul(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left * right;
    return create_comptime_unsigned(ctx, left_type, val);
  } else {
    return create_unsigned(ctx, left_type);
  }
}
static value_t unsigned_div(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left / right;
    return create_comptime_unsigned(ctx, left_type, val);
  } else {
    return create_unsigned(ctx, left_type);
  }
}
static value_t unsigned_mod(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left % right;
    return create_comptime_unsigned(ctx, left_type, val);
  } else {
    return create_unsigned(ctx, left_type);
  }
}
static value_t unsigned_and(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left & right;
    return create_comptime_unsigned(ctx, left_type, val);
  } else {
    return create_unsigned(ctx, left_type);
  }
}
static value_t unsigned_or(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left | right;
    return create_comptime_unsigned(ctx, left_type, val);
  } else {
    return create_unsigned(ctx, left_type);
  }
}
static value_t unsigned_xor(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left ^ right;
    return create_comptime_unsigned(ctx, left_type, val);
  } else {
    return create_unsigned(ctx, left_type);
  }
}
static value_t unsigned_shl(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left << right;
    return create_comptime_unsigned(ctx, left_type, val);
  } else {
    return create_unsigned(ctx, left_type);
  }
}
static value_t unsigned_shr(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left >> right;
    return create_comptime_unsigned(ctx, left_type, val);
  } else {
    return create_unsigned(ctx, left_type);
  }
}
static value_t unsigned_eq(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left == right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t unsigned_ne(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left != right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t unsigned_gt(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left > right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t unsigned_ge(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left >= right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t unsigned_lt(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left < right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}

static value_t unsigned_le(value_t self, context_t ctx, value_t another) {
  type_t left_type = value_get_type(self);
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_UNSIGNED) {
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
    int64_t left = unsigned_get_value(self);
    int64_t right = unsigned_get_value(another);
    int64_t val = left <= right;
    return create_comptime_bool(ctx, val, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}
static value_t unsigned_bitwise_not(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  if (value_is_comptime(self)) {
    int64_t val = unsigned_get_value(self);
    return create_comptime_unsigned(ctx, type, ~val);
  } else {
    return create_unsigned(ctx, type);
  }
}
static value_t unsigned_plus(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  if (value_is_comptime(self)) {
    int64_t val = unsigned_get_value(self);
    return create_comptime_unsigned(ctx, type, +val);
  } else {
    return create_unsigned(ctx, type);
  }
}

void unsigned_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  struct _type_operator_t opt = {
      .convert = unsigned_convert,
      .safe_convert = unsigned_safe_convert,
      .add = unsigned_add,
      .sub = unsigned_sub,
      .mul = unsigned_mul,
      .div = unsigned_div,
      .mod = unsigned_mod,
      .and_ = unsigned_and,
      .or_ = unsigned_or,
      .xor_ = unsigned_xor,
      .shl = unsigned_shl,
      .shr = unsigned_shr,
      .eq = unsigned_eq,
      .ne = unsigned_ne,
      .gt = unsigned_gt,
      .ge = unsigned_ge,
      .lt = unsigned_lt,
      .le = unsigned_le,
      .bitwise_not = unsigned_bitwise_not,
      .plus = unsigned_plus,
      .addr_of = value_default_address_of,
      .assigment = value_default_assigment,
  };
  type_t u8_t = create_type(allocator, TYPE_KIND_UNSIGNED, sizeof(int8_t),
                            sizeof(int8_t), "u8", "u8", &opt, NULL);
  create_type_value(ctx, u8_t, false, true, "u8");
  context_store_type(ctx, u8_t);
  type_t u16_t = create_type(allocator, TYPE_KIND_UNSIGNED, sizeof(int16_t),
                             sizeof(int16_t), "u16", "u16", &opt, NULL);
  create_type_value(ctx, u16_t, false, true, "u16");
  context_store_type(ctx, u16_t);
  type_t u32_t = create_type(allocator, TYPE_KIND_UNSIGNED, sizeof(int32_t),
                             sizeof(int32_t), "u32", "u32", &opt, NULL);
  create_type_value(ctx, u32_t, false, true, "u32");
  context_store_type(ctx, u32_t);
  type_t u64_t = create_type(allocator, TYPE_KIND_UNSIGNED, sizeof(int64_t),
                             sizeof(int64_t), "u64", "u64", &opt, NULL);
  create_type_value(ctx, u64_t, false, true, "u64");
  context_store_type(ctx, u64_t);
}
value_t create_u8(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "u8");
  return context_create_value(ctx, type, NULL, mut, false, name);
}
value_t create_comptime_u8(context_t ctx, uint8_t val, bool mut,
                           const char *name) {
  type_t type = context_load_type(ctx, "u8");
  return context_create_value(ctx, type, &val, mut, true, name);
}
value_t create_u16(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "u16");
  return context_create_value(ctx, type, NULL, mut, false, name);
}
value_t create_comptime_u16(context_t ctx, uint16_t val, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "u16");
  return context_create_value(ctx, type, &val, mut, true, name);
}
value_t create_u32(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "u32");
  return context_create_value(ctx, type, NULL, mut, false, name);
}
value_t create_comptime_u32(context_t ctx, uint32_t val, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "u32");
  return context_create_value(ctx, type, &val, mut, true, name);
}
value_t create_u64(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "u64");
  return context_create_value(ctx, type, NULL, mut, false, name);
}
value_t create_comptime_u64(context_t ctx, uint64_t val, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "u64");
  return context_create_value(ctx, type, &val, mut, true, name);
}