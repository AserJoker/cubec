#include "engine/bool.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>

static value_t bool_lnot(value_t self, context_t ctx) {
  if (self->comptime) {
    bool lvalue = *(bool *)self->data;
    return create_comptime_bool(ctx, !lvalue, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}
static value_t bool_eq(value_t self, context_t ctx, value_t another) {
  if (another->type->kind == TYPE_KIND_BOOL) {
    if (self->comptime && another->comptime) {
      bool lvalue = *(bool *)self->data;
      bool rvalue = *(bool *)self->data;
      return create_comptime_bool(ctx, lvalue == rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}
static value_t bool_ne(value_t self, context_t ctx, value_t another) {
  if (another->type->kind == TYPE_KIND_BOOL) {
    if (self->comptime && another->comptime) {
      bool lvalue = *(bool *)self->data;
      bool rvalue = *(bool *)self->data;
      return create_comptime_bool(ctx, lvalue != rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}

static value_t bool_and(value_t self, context_t ctx, value_t another) {
  if (another->type->kind == TYPE_KIND_BOOL) {
    if (self->comptime && another->comptime) {
      bool lvalue = *(bool *)self->data;
      bool rvalue = *(bool *)self->data;
      return create_comptime_bool(ctx, lvalue & rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}

static value_t bool_or(value_t self, context_t ctx, value_t another) {
  if (another->type->kind == TYPE_KIND_BOOL) {
    if (self->comptime && another->comptime) {
      bool lvalue = *(bool *)self->data;
      bool rvalue = *(bool *)self->data;
      return create_comptime_bool(ctx, lvalue | rvalue, false, NULL);
    } else {
      return create_bool(ctx, false, NULL);
    }
  }
  return NULL;
}

void init_bool_type(context_t ctx) {
  struct _type_operator_t opt = {
      .opt_lnot = bool_lnot,
      .opt_eq = bool_eq,
      .opt_ne = bool_ne,
      .opt_and = bool_and,
      .opt_or = bool_or,
  };
  type_t type = create_type(ctx->allocator, TYPE_KIND_BOOL, "bool", "bool",
                            sizeof(bool), sizeof(bool), &opt, NULL);
  context_store_type(ctx, type);
}
value_t create_comptime_bool(context_t ctx, bool value, bool mut,
                             const char *name) {
  type_t type = context_load_type(ctx, "bool");
  return context_create_comptime_value(ctx, type, &value, mut, name);
}
value_t create_bool(context_t ctx, bool mut, const char *name) {
  type_t type = context_load_type(ctx, "bool");
  return context_create_value(ctx, type, mut, name);
}