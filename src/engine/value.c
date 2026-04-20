#include "engine/value.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include <stdbool.h>
#include <string.h>
struct _value_t {
  type_t type;
  bool mut;
  bool comptime;
  void *data;
};
static void value_dispose(value_t self, allocator_t allocator) {
  allocator_free(allocator, self->data);
}
value_t create_value(allocator_t allocator, type_t type, bool mut,
                     const void *data, bool comptime) {
  value_t self = allocator_alloc(allocator, sizeof(struct _value_t),
                                 (dispose_fn_t)value_dispose);
  self->type = type;
  self->mut = mut;
  size_t size = type_get_size(type);
  self->data = allocator_alloc(allocator, size, NULL);
  if (data) {
    memcpy(self->data, data, size);
  } else {
    memset(self->data, 0, size);
  }
  self->comptime = comptime;
  return self;
}
value_t create_weak_value(allocator_t allocator, type_t type, bool mut,
                          void *data) {
  value_t self = allocator_alloc(allocator, sizeof(struct _value_t), NULL);
  self->type = type;
  self->mut = mut;
  size_t size = type_get_size(type);
  self->data = data;
  self->comptime = true;
  return self;
}
bool value_is_mutable(value_t value) { return value->mut; }
bool value_is_comptime(value_t value) { return value->comptime; }
const void *value_get_data(value_t value) { return value->data; }
type_t value_get_type(value_t value) { return value->type; }
value_t value_clone(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  const void *data = value_get_data(self);
  bool mut = value_is_mutable(self);
  bool comptime = value_is_comptime(self);
  return context_create_value(ctx, type, data, mut, comptime, NULL);
}
value_t value_convert(value_t self, struct _context_t *ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (strcmp(type_get_id(value_type), type_get_id(type)) == 0) {
    return value_clone(self, ctx);
  }
  const type_operator_t *opt = type_get_operator(value_type);
  if (opt->convert) {
    return opt->convert(self, ctx, type);
  }
  return create_error(ctx, "cannot convert '%s' to '%s'",
                      type_get_name(value_type), type_get_name(type));
}
value_t value_safe_convert(value_t self, struct _context_t *ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (strcmp(type_get_id(value_type), type_get_id(type)) == 0) {
    return value_clone(self, ctx);
  }
  const type_operator_t *opt = type_get_operator(value_type);
  if (opt->safe_convert) {
    return opt->safe_convert(self, ctx, type);
  }
  return create_error(ctx, "cannot convert '%s' to '%s'",
                      type_get_name(value_type), type_get_name(type));
}
value_t value_addr_of(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->addr_of) {
    return opt->addr_of(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}
value_t value_ref(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->ref) {
    return opt->ref(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}
value_t value_deref(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->deref) {
    return opt->deref(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}
value_t value_plus(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->plus) {
    return opt->plus(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}
value_t value_neg(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->neg) {
    return opt->neg(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}
value_t value_logical_not(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->logical_not) {
    return opt->logical_not(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}
value_t value_bitwise_not(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->bitwise_not) {
    return opt->bitwise_not(self, ctx);
  }
  return create_error(ctx, "unsupport operator for type '%s'",
                      type_get_name(type));
}

value_t value_add(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->add) {
    return opt->add(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_sub(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->sub) {
    return opt->sub(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_mul(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->mul) {
    return opt->mul(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_div(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->div) {
    return opt->div(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_mod(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->mod) {
    return opt->mod(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_and(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->and_) {
    return opt->and_(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_or(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->or_) {
    return opt->or_(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_xor(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->xor_) {
    return opt->xor_(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_shl(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->shl) {
    return opt->shl(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_shr(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->shr) {
    return opt->shr(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_eq(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->eq) {
    return opt->eq(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_ne(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->ne) {
    return opt->ne(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_gt(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->gt) {
    return opt->gt(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_ge(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->ge) {
    return opt->ge(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_lt(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->lt) {
    return opt->lt(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_le(value_t self, struct _context_t *ctx, value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->le) {
    return opt->le(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_logical_and(value_t self, struct _context_t *ctx,
                          value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->logical_and) {
    return opt->logical_and(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}
value_t value_logical_or(value_t self, struct _context_t *ctx,
                         value_t another) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(another);
  if (opt->logical_or) {
    return opt->logical_or(self, ctx, another);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}

value_t value_get_field(value_t self, struct _context_t *ctx,
                        const char *name) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->get_field) {
    return opt->get_field(self, ctx, name);
  }
  return create_error(ctx, "type '%s' does not support field access",
                      type_get_name(type));
}
value_t value_set_field(value_t self, struct _context_t *ctx, const char *name,
                        value_t value) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->set_field) {
    return opt->set_field(self, ctx, name, value);
  }
  return create_error(ctx, "type '%s' does not support field access",
                      type_get_name(type));
}

value_t value_get_index(value_t self, struct _context_t *ctx, size_t idx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->get_index) {
    return opt->get_index(self, ctx, idx);
  }
  return create_error(ctx, "type '%s' does not support field access",
                      type_get_name(type));
}
value_t value_set_index(value_t self, struct _context_t *ctx, size_t idx,
                        value_t value) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->set_index) {
    return opt->set_index(self, ctx, idx, value);
  }
  return create_error(ctx, "type '%s' does not support field access",
                      type_get_name(type));
}

value_t value_get_length(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->get_length) {
    return opt->get_length(self, ctx);
  }
  return create_error(ctx, "type '%s' does not support length access",
                      type_get_name(type));
}

value_t value_call(value_t self, struct _context_t *ctx, size_t argc,
                   value_t argv[]) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  if (opt->call) {
    return opt->call(self, ctx, argc, argv);
  }
  return create_error(ctx, "type '%s' is not callable", type_get_name(type));
}
value_t value_assigment(value_t self, struct _context_t *ctx, value_t value) {
  type_t type = value_get_type(self);
  const type_operator_t *opt = type_get_operator(type);
  type_t right_type = value_get_type(value);
  if (opt->assigment) {
    return opt->assigment(self, ctx, value);
  }
  return create_error(ctx,
                      "invalid operands to binary expression ('%s' and '%s')",
                      type_get_name(type), type_get_name(right_type));
}