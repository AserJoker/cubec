#include "engine/value.h"
#include "core/allocator.h"
#include "engine/array.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/ptr.h"
#include "engine/str.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct _value_t {
  type_t type;
  bool mutable;
  bool comptime;
  void *data;
  scope_t scope;
};
static void value_dispose(value_t self, allocator_t allocator) {
  allocator_free(allocator, self->data);
}
value_t create_value(allocator_t allocator, type_t type, bool mutable,
                     const void *data) {
  value_t self = allocator_alloc(allocator, sizeof(struct _value_t),
                                 (dispose_fn_t)value_dispose);
  size_t size = type_get_size(type);
  self->mutable = mutable;
  self->type = type;
  if (data) {
    self->data = allocator_alloc(allocator, size, NULL);
    memcpy(self->data, data, size);
  } else {
    self->data = NULL;
  }
  self->comptime = false;
  self->scope = NULL;
  return self;
}
void value_set_comptime(value_t self, bool comptime) {
  self->comptime = comptime;
}
struct _scope_t *value_get_scope(value_t self) { return self->scope; }
void value_set_scope(value_t self, struct _scope_t *scope) {
  self->scope = scope;
}
bool value_is_comptime(value_t self) { return self->comptime; }
type_t value_get_type(value_t value) { return value->type; }
bool value_type_is(value_t value, type_kind_t kind) {
  return type_get_kind(value->type) == kind;
}
bool value_is_mutable(value_t value) { return value->mutable; }
void value_set_mutable(value_t value, bool mutable) {
  value->mutable = mutable;
}
void *value_get_data(value_t value) { return value->data; }
value_t value_clone(allocator_t allocator, value_t value) {
  value_t val =
      create_value(allocator, value->type, value->mutable, value->data);
  value_set_comptime(val, value_is_comptime(value));
  return val;
}
value_t value_assigment(value_t self, context_t ctx, value_t value) {

  if (!self->mutable) {
    return create_error(ctx, "value is not mutable");
  }
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(value);
  if (!type_is_equal(ltype, rtype)) {
    value = value_safe_convert(value, ctx, ltype);
    if (value_is_error(value)) {
      return value;
    }
    if (value_is_interrupt(value)) {
      return value;
    }
  }
  if (self->data && value->data) {
    memcpy(self->data, value->data, type_get_size(ltype));
  }
  return value;
}

value_t value_unref_assigment(value_t self, struct _context_t *ctx,
                              value_t value) {
  if (!self->mutable) {
    return create_error(ctx, "value is not mutable");
  }
  type_t ptr_type = value_get_type(self);
  type_t ltype = ptr_type_get_type(ptr_type);
  type_t rtype = value_get_type(value);
  if (!type_is_equal(ltype, rtype)) {
    value = value_safe_convert(value, ctx, ltype);
    if (value_is_error(value)) {
      return value;
    }
    if (value_is_interrupt(value)) {
      return value;
    }
  }
  void *ptr = self->data;
  if (ptr && value->data) {
    memcpy(*(void **)ptr, value->data, type_get_size(ltype));
  }
  return value;
}
bool value_is_interrupt(value_t value) {
  type_t type = value_get_type(value);
  type_kind_t kind = type_get_kind(type);
  return kind == VALUE_TYPE_INTERRUPT;
}
bool value_is_error(value_t value) {
  type_t type = value_get_type(value);
  type_kind_t kind = type_get_kind(type);
  return kind == VALUE_TYPE_ERROR;
}

value_t value_to_string(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  allocator_t allocator = context_get_allocator(ctx);
  if (!value_is_comptime(self)) {
    char *type_name = type_to_string(type, allocator);
    size_t len = snprintf(NULL, 0, "%s{<runtime>}", type_name);
    char str[len + 1];
    sprintf(str, "%s{<runtime>}", type_name);
    allocator_free(allocator, type_name);
    return create_str(ctx, str, NULL);
  }
  if (opt->to_string) {
    return opt->to_string(self, ctx);
  }
  char *type_name = type_to_string(type, allocator);
  void *data = value_get_data(self);
  size_t len =
      snprintf(NULL, 0, "%s{0x%" PRIXPTR "}", type_name, (intptr_t)data);
  char str[len + 1];
  sprintf(str, "%s{0x%" PRIXPTR "}", type_name, (intptr_t)data);
  allocator_free(allocator, type_name);
  return create_str(ctx, str, NULL);
}
value_t value_get_index(value_t self, context_t ctx, size_t idx) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  if (opt->get_index) {
    return opt->get_index(self, ctx, idx);
  }
  return create_error(ctx, "value does not support index access");
}
value_t value_set_index(value_t self, struct _context_t *ctx, size_t idx,
                        value_t item) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  if (opt->set_index) {
    return opt->set_index(self, ctx, idx, item);
  }
  return create_error(ctx, "value does not support index access");
}
value_t value_get_field(value_t self, context_t ctx, const char *name) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  if (opt->get_field) {
    return opt->get_field(self, ctx, name);
  }
  return create_error(ctx, "value does not support member access");
}
value_t value_set_field(value_t self, struct _context_t *ctx, const char *name,
                        value_t value) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  if (opt->set_field) {
    return opt->set_field(self, ctx, name, value);
  }
  return create_error(ctx, "value does not support member access");
}
value_t value_get_length(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  if (opt->get_length) {
    return opt->get_length(self, ctx);
  }
  return create_error(ctx, "value does not support get length");
}
value_t value_call(value_t self, context_t ctx, size_t argc, value_t argv[]) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  if (opt->call) {
    return opt->call(self, ctx, argc, argv);
  }
  return create_error(ctx, "value is not callable");
}
value_t value_convert(value_t self, struct _context_t *ctx, type_t type) {
  type_t ltype = value_get_type(self);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  value_t res = opt->convert ? opt->convert(self, ctx, type) : NULL;
  if (!res) {
    char *dst_name = type_to_string(ltype, allocator);
    char *src_name = type_to_string(type, allocator);
    value_t err =
        create_error(ctx, "cannot convert '%s' to '%s'", src_name, dst_name);
    allocator_free(allocator, src_name);
    allocator_free(allocator, dst_name);
    return err;
  }
  return res;
}
value_t value_safe_convert(value_t self, struct _context_t *ctx, type_t type) {
  type_t ltype = value_get_type(self);
  type_t rtype = type;
  if (type_is_safe_convert(ltype, rtype) && type_get_operator(ltype)->convert) {
    return type_get_operator(ltype)->convert(self, ctx, type);
  }
  allocator_t allocator = context_get_allocator(ctx);
  char *dst_name = type_to_string(rtype, allocator);
  char *src_name = type_to_string(ltype, allocator);
  value_t err =
      create_error(ctx, "cannot convert '%s' to '%s'", src_name, dst_name);
  allocator_free(allocator, src_name);
  allocator_free(allocator, dst_name);
  return err;
}
value_t value_add(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->add_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->add_opt(self, ctx, another);
}
value_t value_sub(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->sub_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->sub_opt(self, ctx, another);
}
value_t value_mul(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->mul_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->mul_opt(self, ctx, another);
}
value_t value_div(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->div_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->div_opt(self, ctx, another);
}
value_t value_mod(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->mod_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->mod_opt(self, ctx, another);
}
value_t value_and(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->and_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->and_opt(self, ctx, another);
}
value_t value_or(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->or_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->or_opt(self, ctx, another);
}
value_t value_xor(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->xor_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->xor_opt(self, ctx, another);
}
value_t value_shl(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->shl_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->shl_opt(self, ctx, another);
}
value_t value_shr(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->shr_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->shr_opt(self, ctx, another);
}
value_t value_eq(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->eq_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->eq_opt(self, ctx, another);
}
value_t value_ne(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->ne_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->ne_opt(self, ctx, another);
}
value_t value_lt(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->lt_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->lt_opt(self, ctx, another);
}
value_t value_gt(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->gt_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->gt_opt(self, ctx, another);
}
value_t value_le(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->le_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->le_opt(self, ctx, another);
}
value_t value_ge(value_t self, struct _context_t *ctx, value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->ge_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->ge_opt(self, ctx, another);
}
value_t value_plus(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->plus_opt) {
    char *ltype_name = type_to_string(type, allocator);
    value_t err =
        create_error(ctx, "invalid operands expression ('%s')", ltype_name);
    allocator_free(allocator, ltype_name);
    return err;
  }
  return opt->plus_opt(self, ctx);
}
value_t value_neg(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->neg_opt) {
    char *ltype_name = type_to_string(type, allocator);
    value_t err =
        create_error(ctx, "invalid operands expression ('%s')", ltype_name);
    allocator_free(allocator, ltype_name);
    return err;
  }
  return opt->neg_opt(self, ctx);
}
value_t value_bitwise_not(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->bitwise_not_opt) {
    char *ltype_name = type_to_string(type, allocator);
    value_t err =
        create_error(ctx, "invalid operands expression ('%s')", ltype_name);
    allocator_free(allocator, ltype_name);
    return err;
  }
  return opt->bitwise_not_opt(self, ctx);
}
value_t value_logical_not(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->logical_not_opt) {
    char *ltype_name = type_to_string(type, allocator);
    value_t err =
        create_error(ctx, "invalid operands expression ('%s')", ltype_name);
    allocator_free(allocator, ltype_name);
    return err;
  }
  return opt->logical_not_opt(self, ctx);
}
value_t value_logical_and(value_t self, struct _context_t *ctx,
                          value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->logical_and_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->logical_and_opt(self, ctx, another);
}
value_t value_logical_or(value_t self, struct _context_t *ctx,
                         value_t another) {
  type_t ltype = value_get_type(self);
  type_t rtype = value_get_type(another);
  type_operator_t opt = type_get_operator(ltype);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->logical_or_opt) {
    char *ltype_name = type_to_string(ltype, allocator);
    char *rtype_name = type_to_string(rtype, allocator);
    value_t err = create_error(
        ctx, "invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    allocator_free(allocator, ltype_name);
    allocator_free(allocator, rtype_name);
    return err;
  }
  if (!type_is_equal(ltype, rtype)) {
    type_kind_t lkind = type_get_kind(ltype);
    type_kind_t rkind = type_get_kind(rtype);
    if (lkind > rkind) {
      another = value_safe_convert(another, ctx, ltype);
    }
    if (value_is_error(another)) {
      return another;
    }
    if (value_is_interrupt(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = value_safe_convert(self, ctx, rtype);
    }
    if (value_is_error(self)) {
      return self;
    }
    if (value_is_interrupt(self)) {
      return self;
    }
  }
  return opt->logical_or_opt(self, ctx, another);
}
value_t value_unref(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->unref) {
    char *ltype_name = type_to_string(type, allocator);
    value_t err =
        create_error(ctx, "invalid operands expression ('%s')", ltype_name);
    allocator_free(allocator, ltype_name);
    return err;
  }
  return opt->unref(self, ctx);
}
value_t value_ref(value_t self, struct _context_t *ctx) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  allocator_t allocator = context_get_allocator(ctx);
  if (!opt->ref) {
    char *ltype_name = type_to_string(type, allocator);
    value_t err =
        create_error(ctx, "invalid operands expression ('%s')", ltype_name);
    allocator_free(allocator, ltype_name);
    return err;
  }
  return opt->ref(self, ctx);
}
value_t value_member_ref(value_t self, struct _context_t *ctx,
                         const char *name) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  if (!opt->get_field) {
    return create_error(ctx, "value does not support member access");
  }
  if (type_get_kind(type) == VALUE_TYPE_STRUCT) {
    struct_field_t field = struct_type_get_field(type, name);
    if (!field) {
      return create_error(ctx, "no member %s in value", name);
    }
    void *data = value_get_data(self);
    data = (uint8_t *)data + field->offset;
    type_t ptr = type_get_ptr_type(field->type, ctx);
    value_t res = context_create_value(ctx, ptr, false, &data, NULL);
    value_set_comptime(res, true);
    return res;
  } else if (type_get_kind(type) == VALUE_TYPE_UNION) {
    union_field_t field = union_type_get_field(type, name);
    if (!field) {
      return create_error(ctx, "no member %s in value", name);
    }
    void *data = value_get_data(self);
    data = (uint8_t *)data;
    type_t ptr = type_get_ptr_type(field->type, ctx);
    value_t res = context_create_value(ctx, ptr, false, &data, NULL);
    value_set_comptime(res, true);
    return res;
  }
  value_t val = value_get_field(self, ctx, name);
  if (value_is_error(val)) {
    return val;
  }
  if (value_is_interrupt(val)) {
    return val;
  }
  return value_ref(val, ctx);
}
value_t value_index_ref(value_t self, struct _context_t *ctx, size_t idx) {
  type_t type = value_get_type(self);
  type_operator_t opt = type_get_operator(type);
  if (!opt->get_index) {
    return create_error(ctx, "value does not support member access");
  }
  if (type_get_kind(type) == VALUE_TYPE_ARRAY) {
    if (idx >= array_type_get_length(type)) {
      return create_error(
          ctx, "array index %" PRIdPTR " is past the end of the array", idx);
    }
    type_t item_type = array_type_get_type(type);
    size_t offset = type_get_size(item_type) * idx;
    void *data = value_get_data(self);
    data = (uint8_t *)data + offset;
    type_t ptr = type_get_ptr_type(item_type, ctx);
    value_t res = context_create_value(ctx, ptr, false, &data, NULL);
    value_set_comptime(res, true);
    return res;
  } else if (type_get_kind(type) == VALUE_TYPE_PARRAY) {
    type_t item_type = ptr_type_get_type(type);
    size_t offset = type_get_size(item_type) * idx;
    void *data = *(void **)value_get_data(self);
    data = (uint8_t *)data + offset;
    type_t ptr = type_get_ptr_type(item_type, ctx);
    value_t res = context_create_value(ctx, ptr, false, &data, NULL);
    value_set_comptime(res, true);
    return res;
  }
  value_t val = value_get_index(self, ctx, idx);
  if (value_is_error(val)) {
    return val;
  }
  if (value_is_interrupt(val)) {
    return val;
  }
  return value_ref(val, ctx);
}
value_t value_member_call(value_t value, context_t ctx, const char *name,
                          size_t argc, value_t *argv) {
  value_t self = value;
  if (value_type_is(value, VALUE_TYPE_PTR)) {
    value = value_unref(value, ctx);
    if (value_type_is(value, VALUE_TYPE_ERROR)) {
      return value;
    }
  } else {
    self = value_ref(value, ctx);
    if (value_type_is(self, VALUE_TYPE_ERROR)) {
      return value;
    }
  }
  if (value_type_is(value, VALUE_TYPE_TYPE)) {
    value_t member = value_get_field(value, ctx, name);
    if (value_type_is(member, VALUE_TYPE_ERROR)) {
      return member;
    }
    if (!function_is_comptime(member)) {
      return create_error(ctx, "expression is not comptime");
    }
    return value_call(member, ctx, argc, argv);
  }
  type_t type = value_get_type(value);
  value_t vtype = create_type_value(ctx, type, false, NULL);
  value_t member = value_get_field(vtype, ctx, name);
  if (value_type_is(member, VALUE_TYPE_ERROR)) {
    return member;
  }
  if (!value_is_comptime(member)) {
    return create_error(ctx, "expression is not comptime");
  }
  value_t args[argc + 1];
  for (size_t idx = 0; idx < argc; idx++) {
    args[idx + 1] = argv[idx];
  }
  args[0] = self;
  return value_call(member, ctx, argc + 1, args);
}
value_t value_try(value_t value, context_t ctx) {
  static_scope_t static_scope = context_get_static_scope(ctx);
  type_t ctx_type = value_get_type(static_scope->binding);
  if (type_get_kind(ctx_type) != VALUE_TYPE_FUNCTION) {
    return create_error(ctx, "try expression only used in function");
  }
  value_t is_error = value_member_call(value, ctx, "is_error", 0, NULL);
  if (value_is_error(is_error)) {
    return is_error;
  }
  if (!value_type_is(is_error, VALUE_TYPE_BOOL)) {
    is_error = value_safe_convert(value, ctx, context_load_type(ctx, "bool"));
    if (value_is_error(is_error)) {
      return is_error;
    }
  }
  type_t result_type = function_type_get_type(ctx_type);
  value_t vresult_type = create_type_value(ctx, result_type, false, NULL);
  if (!value_get_data(is_error)) {
    return create_error(ctx, "value is not comptime");
  }
  bool is_err = *(bool *)value_get_data(is_error);
  if (is_err) {
    value_t err = value_member_call(value, ctx, "error", 0, NULL);
    if (value_is_error(err)) {
      return err;
    }
    err = value_member_call(vresult_type, ctx, "of_error", 1, &err);
    if (value_is_error(err)) {
      return err;
    }
    return context_create_interrupt(ctx, err);
  } else {
    return value_member_call(value, ctx, "value", 0, NULL);
  }
}