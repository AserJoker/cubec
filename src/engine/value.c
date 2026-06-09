#include "engine/value.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/ptr.h"
#include "engine/type.h"
#include "engine/void.h"
#include <iso646.h>
#include <stdbool.h>
#include <string.h>

static void value_dispose(value_t self, allocator_t allocator) {
  allocator_free(allocator, self->data);
}

value_t create_value(allocator_t allocator, type_t type, bool mut) {
  value_t self = allocator_alloc(allocator, sizeof(struct _value_t),
                                 (dispose_fn_t)(value_dispose));
  self->comptime = false;
  self->mut = mut;
  self->type = type;
  self->data = NULL;
  return self;
}
value_t create_comptime_value(allocator_t allocator, type_t type,
                              const void *data, bool mut) {
  value_t self = allocator_alloc(allocator, sizeof(struct _value_t),
                                 (dispose_fn_t)(value_dispose));
  self->comptime = true;
  self->mut = mut;
  self->type = type;
  self->data = allocator_alloc(allocator, type->size, NULL);
  if (data) {
    memcpy(self->data, data, type->size);
  } else {
    memset(self->data, 0, type->size);
  }
  return self;
}
value_t create_weak_value(allocator_t allocator, type_t type, void *data,
                          bool mut) {
  value_t self = allocator_alloc(allocator, sizeof(struct _value_t), NULL);
  self->comptime = true;
  self->mut = mut;
  self->type = type;
  self->data = data;
  return self;
}

value_t value_clone(value_t self, allocator_t allcoator) {
  if (self->comptime) {
    return create_comptime_value(allcoator, self->type, self->data, self->mut);
  } else {
    return create_value(allcoator, self->type, self->mut);
  }
}
value_t value_ref(value_t self, allocator_t allcoator) {
  if (self->comptime) {
    return create_weak_value(allcoator, self->type, self->data, self->mut);
  }
  return create_value(allcoator, self->type, self->mut);
}

value_t value_safe_convert(value_t self, struct _context_t *ctx, type_t type) {
  type_t self_type = self->type;
  if (self_type->opt.safe_convert) {
    value_t value = self_type->opt.safe_convert(self, ctx, type);
    if (value) {
      return value;
    }
  }
  if (type_is_equal(self_type, type)) {
    value_t value = value_clone(self, ctx->allocator);
    context_declar(ctx, NULL, value);
    return value;
  }
  return create_error(ctx, "cannot convert '%s' to '%s'", self_type->name,
                      type->name);
}
value_t value_addr(value_t self, struct _context_t *ctx) {
  type_t ptr_type = create_ptr_type(ctx, self->type, true, false);
  if (self->comptime) {
    return context_create_comptime_value(ctx, ptr_type, &self->data, self->mut,
                                         NULL);
  }
  return context_create_value(ctx, ptr_type, self->mut, NULL);
}
value_t value_deref(value_t self, struct _context_t *ctx) {
  type_t self_type = self->type;
  if (self_type->kind == TYPE_KIND_PTR) {
    type_t type = ptr_type_get_type(self_type);
    if (self->comptime) {
      return context_create_weak_value(ctx, type, *(void **)self->data,
                                       self->mut, NULL);
    } else {
      return context_create_value(ctx, type, self->mut, NULL);
    }
  }
  return create_error(ctx, "cannot deref of '%s'", self_type->name);
}
value_t value_len(value_t self, struct _context_t *ctx) {
  type_t self_type = self->type;
  if (self_type->opt.len) {
    value_t value = self_type->opt.len(self, ctx);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "cannot get length of '%s'", self_type->name);
}
value_t value_slice(value_t self, struct _context_t *ctx, value_t start,
                    value_t end) {
  type_t self_type = self->type;
  if (self_type->opt.slice) {
    value_t value = self_type->opt.slice(self, ctx, start, end);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "cannot get slice of '%s'", self_type->name);
}
value_t value_call(value_t self, struct _context_t *ctx, size_t argc,
                   value_t *argv) {
  type_t self_type = self->type;
  if (self_type->opt.call) {
    value_t value = self_type->opt.call(self, ctx, argc, argv);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "type '%s' is not function-like", self_type->name);
}
value_t value_get(value_t self, struct _context_t *ctx, value_t field) {
  type_t self_type = self->type;
  if (self_type->opt.get) {
    value_t res = self_type->opt.get(self, ctx, field);
    if (res) {
      return res;
    }
  }
  return create_error(ctx, "cannot get field of '%s'", self_type->name);
}
value_t value_set(value_t self, struct _context_t *ctx, value_t field,
                  value_t value) {
  type_t self_type = self->type;
  if (self_type->opt.set) {
    value_t res = self_type->opt.set(self, ctx, field, value);
    if (res) {
      return res;
    }
  }
  return create_error(ctx, "cannot set field of '%s'", self_type->name);
}
value_t value_get_field(value_t self, struct _context_t *ctx,
                        const char *field) {
  type_t self_type = self->type;
  if (self_type->opt.get_field) {
    value_t res = self_type->opt.get_field(self, ctx, field);
    if (res) {
      return res;
    }
  }
  return create_error(ctx, "cannot get field of '%s'", self_type->name);
}
value_t value_set_field(value_t self, struct _context_t *ctx, const char *field,
                        value_t value) {
  type_t self_type = self->type;
  if (self_type->opt.set_field) {
    value_t res = self_type->opt.set_field(self, ctx, field, value);
    if (res) {
      return res;
    }
  }
  return create_error(ctx, "cannot set field of '%s'", self_type->name);
}
value_t value_iterator(value_t self, struct _context_t *ctx) {
  type_t self_type = self->type;
  if (self_type->opt.iterator) {
    value_t value = self_type->opt.iterator(self, ctx);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "cannot get iterator of '%s'", self_type->name);
}
value_t value_opt_add(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_add) {
    value_t value = self_type->opt.opt_add(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '+' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_sub(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_sub) {
    value_t value = self_type->opt.opt_sub(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '-' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_mod(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_sub) {
    value_t value = self_type->opt.opt_sub(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '%' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_mul(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_mul) {
    value_t value = self_type->opt.opt_mul(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '*' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_div(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_div) {
    value_t value = self_type->opt.opt_div(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '/' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_shr(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_mod) {
    value_t value = self_type->opt.opt_mod(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '>>' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_shl(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_shl) {
    value_t value = self_type->opt.opt_shl(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '<<' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_and(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_shr) {
    value_t value = self_type->opt.opt_shr(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '&' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_or(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_or) {
    value_t value = self_type->opt.opt_or(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '|' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_xor(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_xor) {
    value_t value = self_type->opt.opt_xor(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '^' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_eq(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_eq) {
    value_t value = self_type->opt.opt_eq(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '==' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_ne(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_ne) {
    value_t value = self_type->opt.opt_ne(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '!=' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_gt(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_gt) {
    value_t value = self_type->opt.opt_gt(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '>' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_ge(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_ge) {
    value_t value = self_type->opt.opt_ge(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '>=' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_lt(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_lt) {
    value_t value = self_type->opt.opt_lt(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '<' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_le(value_t self, struct _context_t *ctx, value_t another) {
  type_t self_type = self->type;
  if (self_type->opt.opt_le) {
    value_t value = self_type->opt.opt_le(self, ctx, another);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid binary operator '<=' with '%s' and '%s'",
                      self_type->name, another->type->name);
}
value_t value_opt_plu(value_t self, struct _context_t *ctx) {
  type_t self_type = self->type;
  if (self_type->opt.opt_plu) {
    value_t value = self_type->opt.opt_plu(self, ctx);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid operator '+' with '%s'", self_type->name);
}
value_t value_opt_neg(value_t self, struct _context_t *ctx) {
  type_t self_type = self->type;
  if (self_type->opt.opt_neg) {
    value_t value = self_type->opt.opt_neg(self, ctx);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid operator '-' with '%s'", self_type->name);
}
value_t value_opt_lnot(value_t self, struct _context_t *ctx) {
  type_t self_type = self->type;
  if (self_type->opt.opt_lnot) {
    value_t value = self_type->opt.opt_lnot(self, ctx);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid operator '!' with '%s'", self_type->name);
}
value_t value_opt_not(value_t self, struct _context_t *ctx) {
  type_t self_type = self->type;
  if (self_type->opt.opt_not) {
    value_t value = self_type->opt.opt_not(self, ctx);
    if (value) {
      return value;
    }
  }
  return create_error(ctx, "invalid operator '~' with '%s'", self_type->name);
}
value_t value_assigment(value_t self, struct _context_t *ctx, value_t value) {
  if (self->type->kind == TYPE_KIND_PTR) {
    if (!ptr_type_is_mut(self->type)) {
      return create_error(ctx, "assignment to constant ptr");
    }
  } else if (!self->mut) {
    return create_error(ctx, "assignment to constant variable");
  }
  value = value_safe_convert(value, ctx, self->type);
  if (value->type->kind == TYPE_KIND_ERROR) {
    return value;
  }
  if (self->comptime && value->comptime) {
    memcpy(self->data, value->data, self->type->size);
  } else if (self->comptime && !value->comptime) {
    return create_error(ctx, "value is not comptime");
  }
  return create_comptime_void(ctx);
}