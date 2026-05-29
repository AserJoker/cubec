#include "engine/float.h"
#include "engine/bool.h"

static value_t float_add(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_F16 &&
      another->type->kind <= TYPE_KIND_F64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      float64_t lvalue = float_get_value(self);
      float64_t rvalue = float_get_value(another);
      float64_t result = lvalue + rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t float_sub(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_F16 &&
      another->type->kind <= TYPE_KIND_F64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      float64_t lvalue = float_get_value(self);
      float64_t rvalue = float_get_value(another);
      float64_t result = lvalue - rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t float_mul(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_F16 &&
      another->type->kind <= TYPE_KIND_F64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      float64_t lvalue = float_get_value(self);
      float64_t rvalue = float_get_value(another);
      float64_t result = lvalue * rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}
static value_t float_div(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_F16 &&
      another->type->kind <= TYPE_KIND_F64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      float64_t lvalue = float_get_value(self);
      float64_t rvalue = float_get_value(another);
      float64_t result = lvalue / rvalue;
      return context_create_comptime_value(ctx, type, &result, false, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL);
    }
  }
  return NULL;
}

static value_t float_plus(value_t self, context_t ctx) {
  if (self->comptime) {
    float64_t lvalue = +float_get_value(self);
    return context_create_comptime_value(ctx, self->type, &lvalue, false, NULL);
  } else {
    return context_create_value(ctx, self->type, false, NULL);
  }
}
static value_t float_neg(value_t self, context_t ctx) {
  if (self->comptime) {
    float64_t lvalue = -float_get_value(self);
    return context_create_comptime_value(ctx, self->type, &lvalue, false, NULL);
  } else {
    return context_create_value(ctx, self->type, false, NULL);
  }
}

static value_t float_eq(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_F16 &&
      another->type->kind <= TYPE_KIND_F64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      float64_t lvalue = float_get_value(self);
      float64_t rvalue = float_get_value(another);
      return create_comptime_bool(ctx, lvalue == rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t float_ne(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_F16 &&
      another->type->kind <= TYPE_KIND_F64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      float64_t lvalue = float_get_value(self);
      float64_t rvalue = float_get_value(another);
      return create_comptime_bool(ctx, lvalue != rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t float_gt(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_F16 &&
      another->type->kind <= TYPE_KIND_F64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      float64_t lvalue = float_get_value(self);
      float64_t rvalue = float_get_value(another);
      return create_comptime_bool(ctx, lvalue > rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t float_ge(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_F16 &&
      another->type->kind <= TYPE_KIND_F64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      float64_t lvalue = float_get_value(self);
      float64_t rvalue = float_get_value(another);
      return create_comptime_bool(ctx, lvalue >= rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t float_lt(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_F16 &&
      another->type->kind <= TYPE_KIND_F64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      float64_t lvalue = float_get_value(self);
      float64_t rvalue = float_get_value(another);
      return create_comptime_bool(ctx, lvalue < rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t float_le(value_t self, context_t ctx, value_t another) {
  if (another->type->kind >= TYPE_KIND_F16 &&
      another->type->kind <= TYPE_KIND_F64) {
    type_t type =
        self->type->kind > another->type->kind ? self->type : another->type;
    if (self->comptime && another->comptime) {
      float64_t lvalue = float_get_value(self);
      float64_t rvalue = float_get_value(another);
      return create_comptime_bool(ctx, lvalue <= rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}

void init_float_type(context_t ctx) {
  struct _type_operator_t opt = {
      .opt_add = float_add,
      .opt_sub = float_sub,
      .opt_mul = float_mul,
      .opt_div = float_div,
      .opt_plu = float_plus,
      .opt_neg = float_neg,
      .opt_eq = float_eq,
      .opt_ne = float_ne,
      .opt_gt = float_gt,
      .opt_ge = float_ge,
      .opt_lt = float_lt,
      .opt_le = float_le,
  };

  type_t f16_type =
      create_type(ctx->allocator, TYPE_KIND_F16, "f16", "f16", sizeof(int16_t),
                  sizeof(int16_t), &opt, NULL, false);
  context_store_type(ctx, f16_type);
  type_t f32_type =
      create_type(ctx->allocator, TYPE_KIND_F32, "f32", "f32", sizeof(int32_t),
                  sizeof(int32_t), &opt, NULL, false);
  context_store_type(ctx, f32_type);
  type_t f64_type =
      create_type(ctx->allocator, TYPE_KIND_F64, "f64", "f64",
                  sizeof(float64_t), sizeof(float64_t), &opt, NULL, false);
  context_store_type(ctx, f64_type);

  create_type_value(ctx, f16_type, false, "f16");
  create_type_value(ctx, f32_type, false, "f32");
  create_type_value(ctx, f64_type, false, "f64");
}

value_t create_comptime_f16(context_t ctx, float16_t value, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "f16");
  return context_create_comptime_value(ctx, type, &value, mut, name);
}
value_t create_comptime_f32(context_t ctx, float32_t value, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "f32");
  return context_create_comptime_value(ctx, type, &value, mut, name);
}
value_t create_comptime_f64(context_t ctx, float64_t value, bool mut,
                            const char *name) {
  type_t type = context_load_type(ctx, "f64");
  return context_create_comptime_value(ctx, type, &value, mut, name);
}
value_t create_f16(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "f16");
  return context_create_value(ctx, type, mut, name);
}
value_t create_f32(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "f32");
  return context_create_value(ctx, type, mut, name);
}
value_t create_f64(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "f64");
  return context_create_value(ctx, type, mut, name);
}
float64_t float_get_value(value_t value) {
  if (!value->comptime) {
    return 0;
  }
  if (value->type->kind == TYPE_KIND_F16) {
    return *(float16_t *)value->data;
  } else if (value->type->kind == TYPE_KIND_F32) {
    return *(float32_t *)value->data;
  } else if (value->type->kind == TYPE_KIND_F64) {
    return *(float64_t *)value->data;
  }
  return 0;
}