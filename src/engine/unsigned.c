#include "engine/bool.h"
#include "engine/context.h"
#include "engine/unsigned.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>

static value_t unsigned_add(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      uint64_t result = lvalue + rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_sub(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      uint64_t result = lvalue - rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_mul(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      uint64_t result = lvalue * rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_div(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      uint64_t result = lvalue / rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_mod(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      uint64_t result = lvalue % rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_and(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      uint64_t result = lvalue & rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_or(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      uint64_t result = lvalue | rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_xor(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      uint64_t result = lvalue ^ rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_shl(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      uint64_t result = lvalue << rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_shr(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      uint64_t result = lvalue >> rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_plus(value_t self, context_t ctx) {
  if (self->comptime) {
    uint64_t lvalue = +unsigned_get_value(self);
    return context_create_comptime_value(ctx, self->type, &lvalue, false, NULL);
  } else {
    return context_create_value(ctx, self->type, false, NULL);
  }
}
static value_t unsigned_neg(value_t self, context_t ctx) {
  if (self->comptime) {
    uint64_t lvalue = -unsigned_get_value(self);
    return context_create_comptime_value(ctx, self->type, &lvalue, false, NULL);
  } else {
    return context_create_value(ctx, self->type, false, NULL);
  }
}
static value_t unsigned_not(value_t self, context_t ctx) {
  if (self->comptime) {
    uint64_t lvalue = ~unsigned_get_value(self);
    return context_create_comptime_value(ctx, self->type, &lvalue, false, NULL);
  } else {
    return context_create_value(ctx, self->type, false, NULL);
  }
}

static value_t unsigned_eq(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      return create_comptime_bool(ctx, lvalue == rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_ne(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      return create_comptime_bool(ctx, lvalue != rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_gt(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      return create_comptime_bool(ctx, lvalue > rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_ge(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      return create_comptime_bool(ctx, lvalue >= rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_lt(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      return create_comptime_bool(ctx, lvalue < rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t unsigned_le(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_U8 &&
      another->type->kind <= TYPE_KIND_U64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      uint64_t lvalue = unsigned_get_value(self);
      uint64_t rvalue = unsigned_get_value(another);
      return create_comptime_bool(ctx, lvalue <= rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}

void init_unsigned_type(context_t ctx) {
  struct _type_operator_t opt = {
      .opt_add = unsigned_add,
      .opt_sub = unsigned_sub,
      .opt_mul = unsigned_mul,
      .opt_div = unsigned_div,
      .opt_mod = unsigned_mod,
      .opt_and = unsigned_and,
      .opt_or = unsigned_or,
      .opt_xor = unsigned_xor,
      .opt_shl = unsigned_shl,
      .opt_shr = unsigned_shr,
      .opt_plu = unsigned_plus,
      .opt_neg = unsigned_neg,
      .opt_not = unsigned_not,
      .opt_eq = unsigned_eq,
      .opt_ne = unsigned_ne,
      .opt_gt = unsigned_gt,
      .opt_ge = unsigned_ge,
      .opt_lt = unsigned_lt,
      .opt_le = unsigned_le,
  };
  type_t u8_type = create_type(ctx->allocator, TYPE_KIND_U8, "u8", "u8",
                               sizeof(uint8_t), sizeof(uint8_t), &opt, NULL);
  context_store_type(ctx, u8_type);
  type_t u16_type = create_type(ctx->allocator, TYPE_KIND_U16, "u16", "u16",
                                sizeof(uint16_t), sizeof(uint16_t), &opt, NULL);
  context_store_type(ctx, u16_type);
  type_t u32_type = create_type(ctx->allocator, TYPE_KIND_U32, "u32", "u32",
                                sizeof(uint32_t), sizeof(uint32_t), &opt, NULL);
  context_store_type(ctx, u32_type);
  type_t u64_type = create_type(ctx->allocator, TYPE_KIND_U64, "u64", "u64",
                                sizeof(uint64_t), sizeof(uint64_t), &opt, NULL);
  context_store_type(ctx, u64_type);
}
value_t create_comptime_u8(context_t ctx, uint8_t value, bool mut,
                           const char *name) {
  type_t type = context_load_type(ctx, "u8");
  return context_create_comptime_value(ctx, type, &value, mut, name);
}
value_t create_comptime_u16(context_t ctx, uint16_t value, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "u16");
  return context_create_comptime_value(ctx, type, &value, mut, name);
}
value_t create_comptime_u32(context_t ctx, uint32_t value, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "u32");
  return context_create_comptime_value(ctx, type, &value, mut, name);
}
value_t create_comptime_u64(context_t ctx, uint64_t value, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "u64");
  return context_create_comptime_value(ctx, type, &value, mut, name);
}
value_t create_u8(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "u8");
  return context_create_value(ctx, type, mut, name);
}
value_t create_u16(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "u16");
  return context_create_value(ctx, type, mut, name);
}
value_t create_u32(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "u32");
  return context_create_value(ctx, type, mut, name);
}
value_t create_u64(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "u64");
  return context_create_value(ctx, type, mut, name);
}
uint64_t unsigned_get_value(value_t value) {
  if (!value->comptime) {
    return 0;
  }
  if (value->type->kind == TYPE_KIND_U8) {
    return *(uint8_t *)value->data;
  } else if (value->type->kind == TYPE_KIND_U16) {
    return *(uint16_t *)value->data;
  } else if (value->type->kind == TYPE_KIND_U32) {
    return *(uint32_t *)value->data;
  } else if (value->type->kind == TYPE_KIND_U64) {
    return *(uint64_t *)value->data;
  }
  return 0;
}