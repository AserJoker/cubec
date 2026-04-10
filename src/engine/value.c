#include "engine/value.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/str.h"
#include "engine/type.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct _cubec_value_t {
  cubec_type_t type;
  bool mutable;
  void *data;
};
static void cubec_value_dispose(cubec_value_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->data);
}
cubec_value_t cubec_create_value(cubec_allocator_t allocator, cubec_type_t type,
                                 bool mutable, const void *data) {
  cubec_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_value_t),
                            (cubec_dispose_fn_t)cubec_value_dispose);
  size_t size = cubec_type_get_size(type);
  self->mutable = mutable;
  self->type = type;
  if (data) {
    self->data = cubec_allocator_alloc(allocator, size, NULL);
    memcpy(self->data, data, size);
  } else {
    self->data = NULL;
  }
  return self;
}
cubec_type_t cubec_value_get_type(cubec_value_t value) { return value->type; }
bool cubec_value_type_is(cubec_value_t value, cubec_type_kind_t kind) {
  return cubec_type_get_kind(value->type) == kind;
}
bool cubec_value_is_mutable(cubec_value_t value) { return value->mutable; }
void cubec_value_set_mutable(cubec_value_t value, bool mutable) {
  value->mutable = mutable;
}
void *cubec_value_get_data(cubec_value_t value) { return value->data; }
cubec_value_t cubec_value_clone(cubec_allocator_t allocator,
                                cubec_value_t value) {
  return cubec_create_value(allocator, value->type, value->mutable,
                            value->type);
}
cubec_value_t cubec_value_assigment(cubec_value_t self, cubec_context_t ctx,
                                    cubec_value_t value) {

  if (!self->mutable) {
    return cubec_create_error(ctx, "value is not mutable");
  }
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(value);
  if (!cubec_type_is_equal(ltype, rtype)) {
    value = cubec_value_safe_convert(value, ctx, ltype);
    if (cubec_value_is_error(value)) {
      return value;
    }
  }
  memcpy(self->data, value->data, cubec_type_get_size(ltype));
  return self;
}
bool cubec_value_is_error(cubec_value_t value) {
  cubec_type_t type = cubec_value_get_type(value);
  cubec_type_kind_t kind = cubec_type_get_kind(type);
  return kind == CUBEC_VALUE_TYPE_ERROR;
}

cubec_value_t cubec_value_to_string(cubec_value_t self, cubec_context_t ctx) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!cubec_value_get_data(self)) {
    char *type_name = cubec_type_to_string(type, allocator);
    size_t len = snprintf(NULL, 0, "%s{<runtime>}", type_name);
    char str[len + 1];
    sprintf(str, "%s{<runtime>}", type_name);
    cubec_allocator_free(allocator, type_name);
    return cubec_create_str(ctx, str, NULL);
  }
  if (opt->to_string) {
    return opt->to_string(self, ctx);
  }
  char *type_name = cubec_type_to_string(type, allocator);
  void *data = cubec_value_get_data(self);
  size_t len =
      snprintf(NULL, 0, "%s{0x%" PRIXPTR "}", type_name, (intptr_t)data);
  char str[len + 1];
  sprintf(str, "%s{0x%" PRIXPTR "}", type_name, (intptr_t)data);
  cubec_allocator_free(allocator, type_name);
  return cubec_create_str(ctx, str, NULL);
}
cubec_value_t cubec_value_get_index(cubec_value_t self, cubec_context_t ctx,
                                    size_t idx) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->get_index) {
    return opt->get_index(self, ctx, idx);
  }
  return cubec_create_error(ctx, "value does not support index access");
}
cubec_value_t cubec_value_set_index(cubec_value_t self,
                                    struct _cubec_context_t *ctx, size_t idx,
                                    cubec_value_t item) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->set_index) {
    return opt->set_index(self, ctx, idx, item);
  }
  return cubec_create_error(ctx, "value does not support index access");
}
cubec_value_t cubec_value_get_field(cubec_value_t self, cubec_context_t ctx,
                                    const char *name) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->get_field) {
    return opt->get_field(self, ctx, name);
  }
  return cubec_create_error(ctx, "value does not support member access");
}
cubec_value_t cubec_value_set_field(cubec_value_t self,
                                    struct _cubec_context_t *ctx,
                                    const char *name, cubec_value_t value) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->set_field) {
    return opt->set_field(self, ctx, name, value);
  }
  return cubec_create_error(ctx, "value does not support member access");
}
cubec_value_t cubec_value_get_length(cubec_value_t self,
                                     struct _cubec_context_t *ctx) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->get_length) {
    return opt->get_length(self, ctx);
  }
  return cubec_create_error(ctx, "value does not support get length");
}
cubec_value_t cubec_value_call(cubec_value_t self, cubec_context_t ctx,
                               size_t argc, cubec_value_t argv[]) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->call) {
    return opt->call(self, ctx, argc, argv);
  }
  return cubec_create_error(ctx, "value is not callable");
}
cubec_value_t cubec_value_convert(cubec_value_t self,
                                  struct _cubec_context_t *ctx,
                                  cubec_type_t type) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  cubec_value_t res = opt->convert ? opt->convert(self, ctx, type) : NULL;
  if (!res) {
    char *dst_name = cubec_type_to_string(ltype, allocator);
    char *src_name = cubec_type_to_string(type, allocator);
    cubec_value_t err = cubec_create_error(ctx, "Cannot convert '%s' to '%s'",
                                           src_name, dst_name);
    cubec_allocator_free(allocator, src_name);
    cubec_allocator_free(allocator, dst_name);
    return err;
  }
  return res;
}
cubec_value_t cubec_value_safe_convert(cubec_value_t self,
                                       struct _cubec_context_t *ctx,
                                       cubec_type_t type) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = type;
  cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
  cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
  if (lkind >= CUBEC_VALUE_TYPE_INT8 && lkind <= CUBEC_VALUE_TYPE_INT64) {
    if (rkind >= CUBEC_VALUE_TYPE_INT8 && rkind <= CUBEC_VALUE_TYPE_INT64) {
      return cubec_value_convert(self, ctx, type);
    }
  }
  if (lkind >= CUBEC_VALUE_TYPE_UINT8 && lkind <= CUBEC_VALUE_TYPE_UINT64) {
    if (rkind >= CUBEC_VALUE_TYPE_UINT8 && rkind <= CUBEC_VALUE_TYPE_UINT64) {
      return cubec_value_convert(self, ctx, type);
    }
  }
  if (lkind >= CUBEC_VALUE_TYPE_FLOAT32 && lkind <= CUBEC_VALUE_TYPE_FLOAT64) {
    if (rkind >= CUBEC_VALUE_TYPE_FLOAT32 &&
        rkind <= CUBEC_VALUE_TYPE_FLOAT64) {
      return cubec_value_convert(self, ctx, type);
    }
  }
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  char *dst_name = cubec_type_to_string(rtype, allocator);
  char *src_name = cubec_type_to_string(ltype, allocator);
  cubec_value_t err = cubec_create_error(ctx, "Cannot convert '%s' to '%s'",
                                         src_name, dst_name);
  cubec_allocator_free(allocator, src_name);
  cubec_allocator_free(allocator, dst_name);
  return err;
}
cubec_value_t cubec_value_add(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->add_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->add_opt(self, ctx, another);
}
cubec_value_t cubec_value_sub(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->sub_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->sub_opt(self, ctx, another);
}
cubec_value_t cubec_value_mul(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->mul_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->mul_opt(self, ctx, another);
}
cubec_value_t cubec_value_div(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->div_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->div_opt(self, ctx, another);
}
cubec_value_t cubec_value_mod(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->mod_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->mod_opt(self, ctx, another);
}
cubec_value_t cubec_value_and(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->and_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->and_opt(self, ctx, another);
}
cubec_value_t cubec_value_or(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->or_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->or_opt(self, ctx, another);
}
cubec_value_t cubec_value_xor(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->xor_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->xor_opt(self, ctx, another);
}
cubec_value_t cubec_value_shl(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->shl_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->shl_opt(self, ctx, another);
}
cubec_value_t cubec_value_shr(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->shr_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->shr_opt(self, ctx, another);
}
cubec_value_t cubec_value_eq(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->eq_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->eq_opt(self, ctx, another);
}
cubec_value_t cubec_value_ne(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->ne_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->ne_opt(self, ctx, another);
}
cubec_value_t cubec_value_lt(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->lt_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->lt_opt(self, ctx, another);
}
cubec_value_t cubec_value_gt(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->gt_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->gt_opt(self, ctx, another);
}
cubec_value_t cubec_value_le(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->le_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->le_opt(self, ctx, another);
}
cubec_value_t cubec_value_ge(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another) {
  cubec_type_t ltype = cubec_value_get_type(self);
  cubec_type_t rtype = cubec_value_get_type(another);
  cubec_type_operator_t opt = cubec_type_get_operator(ltype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!opt->ge_opt) {
    char *ltype_name = cubec_type_to_string(ltype, allocator);
    char *rtype_name = cubec_type_to_string(rtype, allocator);
    cubec_value_t err = cubec_create_error(
        ctx, "Invalid operands to binary expression ('%s' and '%s')",
        ltype_name, rtype_name);
    cubec_allocator_free(allocator, ltype_name);
    cubec_allocator_free(allocator, rtype_name);
    return err;
  }
  if (!cubec_type_is_equal(ltype, rtype)) {
    cubec_type_kind_t lkind = cubec_type_get_kind(ltype);
    cubec_type_kind_t rkind = cubec_type_get_kind(rtype);
    if (lkind > rkind) {
      another = cubec_value_safe_convert(another, ctx, ltype);
    }
    if (cubec_value_is_error(another)) {
      return another;
    }
    if (lkind < rkind) {
      self = cubec_value_safe_convert(self, ctx, rtype);
    }
    if (cubec_value_is_error(self)) {
      return self;
    }
  }
  return opt->ge_opt(self, ctx, another);
}