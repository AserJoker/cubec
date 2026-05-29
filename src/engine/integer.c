#include "engine/integer.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>

static value_t integer_add(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      int64_t result = lvalue + rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_sub(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      int64_t result = lvalue - rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_mul(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      int64_t result = lvalue * rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_div(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      int64_t result = lvalue / rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_mod(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      int64_t result = lvalue % rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_and(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      int64_t result = lvalue & rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_or(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      int64_t result = lvalue | rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_xor(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      int64_t result = lvalue ^ rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_shl(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      int64_t result = lvalue << rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_shr(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      int64_t result = lvalue >> rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_plus(value_t self, context_t ctx) {
  if (self->comptime) {
    int64_t lvalue = +integer_get_value(self);
    return context_create_comptime_value(ctx, self->type, &lvalue, false, NULL);
  } else {
    return context_create_value(ctx, self->type, false, NULL);
  }
}
static value_t integer_neg(value_t self, context_t ctx) {
  if (self->comptime) {
    int64_t lvalue = -integer_get_value(self);
    return context_create_comptime_value(ctx, self->type, &lvalue, false, NULL);
  } else {
    return context_create_value(ctx, self->type, false, NULL);
  }
}
static value_t integer_not(value_t self, context_t ctx) {
  if (self->comptime) {
    int64_t lvalue = ~integer_get_value(self);
    return context_create_comptime_value(ctx, self->type, &lvalue, false, NULL);
  } else {
    return context_create_value(ctx, self->type, false, NULL);
  }
}

static value_t integer_eq(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      return create_comptime_bool(ctx, lvalue == rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_ne(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      return create_comptime_bool(ctx, lvalue != rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_gt(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      return create_comptime_bool(ctx, lvalue > rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_ge(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      return create_comptime_bool(ctx, lvalue >= rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_lt(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      return create_comptime_bool(ctx, lvalue < rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t integer_le(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_I8 &&
      another->type->kind <= TYPE_KIND_I64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      int64_t lvalue = integer_get_value(self);
      int64_t rvalue = integer_get_value(another);
      return create_comptime_bool(ctx, lvalue <= rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}

void init_integer_type(context_t ctx) {
  struct _type_operator_t opt = {
      .opt_add = integer_add,
      .opt_sub = integer_sub,
      .opt_mul = integer_mul,
      .opt_div = integer_div,
      .opt_mod = integer_mod,
      .opt_and = integer_and,
      .opt_or = integer_or,
      .opt_xor = integer_xor,
      .opt_shl = integer_shl,
      .opt_shr = integer_shr,
      .opt_plu = integer_plus,
      .opt_neg = integer_neg,
      .opt_not = integer_not,
      .opt_eq = integer_eq,
      .opt_ne = integer_ne,
      .opt_gt = integer_gt,
      .opt_ge = integer_ge,
      .opt_lt = integer_lt,
      .opt_le = integer_le,
  };
  type_t i8_type =
      create_type(ctx->allocator, TYPE_KIND_I8, "i8", "i8", sizeof(int8_t),
                  sizeof(int8_t), &opt, NULL, false);
  context_store_type(ctx, i8_type);
  type_t i16_type =
      create_type(ctx->allocator, TYPE_KIND_I16, "i16", "i16", sizeof(int16_t),
                  sizeof(int16_t), &opt, NULL, false);
  context_store_type(ctx, i16_type);
  type_t i32_type =
      create_type(ctx->allocator, TYPE_KIND_I32, "i32", "i32", sizeof(int32_t),
                  sizeof(int32_t), &opt, NULL, false);
  context_store_type(ctx, i32_type);
  type_t i64_type =
      create_type(ctx->allocator, TYPE_KIND_I64, "i64", "i64", sizeof(int64_t),
                  sizeof(int64_t), &opt, NULL, false);
  context_store_type(ctx, i64_type);

  create_type_value(ctx, i8_type, false, "i8");
  create_type_value(ctx, i16_type, false, "i16");
  create_type_value(ctx, i32_type, false, "i32");
  create_type_value(ctx, i64_type, false, "i64");
}
value_t create_comptime_i8(context_t ctx, int8_t value, bool mut,
                           const char *name) {
  type_t type = context_load_type(ctx, "i8");
  return context_create_comptime_value(ctx, type, &value, mut, name);
}
value_t create_comptime_i16(context_t ctx, int16_t value, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "i16");
  return context_create_comptime_value(ctx, type, &value, mut, name);
}
value_t create_comptime_i32(context_t ctx, int32_t value, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "i32");
  return context_create_comptime_value(ctx, type, &value, mut, name);
}
value_t create_comptime_i64(context_t ctx, int64_t value, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "i64");
  return context_create_comptime_value(ctx, type, &value, mut, name);
}
value_t create_i8(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "i8");
  return context_create_value(ctx, type, mut, name);
}
value_t create_i16(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "i16");
  return context_create_value(ctx, type, mut, name);
}
value_t create_i32(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "i32");
  return context_create_value(ctx, type, mut, name);
}
value_t create_i64(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "i64");
  return context_create_value(ctx, type, mut, name);
}
int64_t integer_get_value(value_t value) {
  if (!value->comptime) {
    return 0;
  }
  if (value->type->kind == TYPE_KIND_I8) {
    return *(int8_t *)value->data;
  } else if (value->type->kind == TYPE_KIND_I16) {
    return *(int16_t *)value->data;
  } else if (value->type->kind == TYPE_KIND_I32) {
    return *(int32_t *)value->data;
  } else if (value->type->kind == TYPE_KIND_I64) {
    return *(int64_t *)value->data;
  }
  return 0;
}