#include "engine/ptr.h"
#include "core/allocator.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdio.h>
struct _ptr_meta_t {
  bool vol;
  bool mut;
  type_t type;
};
typedef struct _ptr_meta_t *ptr_meta_t;
static ptr_meta_t create_ptr_meta(allocator_t allocator, type_t type, bool mut,
                                  bool vol) {
  ptr_meta_t self =
      allocator_alloc(allocator, sizeof(struct _ptr_meta_t), NULL);
  self->mut = mut;
  self->vol = vol;
  self->type = type;
  return self;
}
static value_t ptr_eq(value_t self, context_t ctx, value_t another) {
  if (another->type->kind == TYPE_KIND_PTR) {
    if (type_is_equal(ptr_type_get_type(self->type),
                      ptr_type_get_type(another->type))) {
      if (self->comptime && another->comptime) {
        return create_comptime_bool(
            ctx, *(void **)self->data == *(void **)another->data, false, NULL);
      } else {
        return create_bool(ctx, false, NULL);
      }
    }
  }
  return NULL;
}
static value_t ptr_ne(value_t self, context_t ctx, value_t another) {
  if (another->type->kind == TYPE_KIND_PTR) {
    if (type_is_equal(self->type, another->type)) {
      if (self->comptime && another->comptime) {
        return create_comptime_bool(
            ctx, *(void **)self->data != *(void **)another->data, false, NULL);
      } else {
        return create_bool(ctx, false, NULL);
      }
    }
  }
  return NULL;
}
static bool ptr_type_equal(type_t self, type_t another) {
  return another->kind == TYPE_KIND_PTR &&
         type_is_equal(ptr_type_get_type(self), ptr_type_get_type(another));
}
static value_t ptr_get_field(value_t self, context_t ctx, const char *field) {
  type_t ptype = self->type;
  type_t type = ptr_type_get_type(ptype);
  self = value_deref(self, ctx);
  if (type->kind == TYPE_KIND_STRUCT) {
    return value_get_field(self, ctx, field);
  }
  return NULL;
}
static value_t ptr_set_field(value_t self, context_t ctx, const char *field,
                             value_t value) {
  type_t ptype = self->type;
  type_t type = ptr_type_get_type(ptype);
  self = value_deref(self, ctx);
  if (type->kind == TYPE_KIND_STRUCT) {
    return value_set_field(self, ctx, field, value);
  }
  return NULL;
}
static value_t ptr_safe_convert(value_t self, context_t ctx, type_t type) {
  if (type->kind == TYPE_KIND_PTR) {
    ptr_meta_t self_meta = self->type->meta;
    ptr_meta_t dst_meta = type->meta;
    if (!type_is_equal(self_meta->type, dst_meta->type)) {
      return NULL;
    }
    if (self->comptime) {
      return context_create_comptime_value(ctx, type, self->data, self->mut,
                                           NULL);
    } else {
      return context_create_value(ctx, type, self->mut, NULL);
    }
  }
  return NULL;
}
type_t create_ptr_type(context_t ctx, type_t type, bool mut, bool vol) {
  size_t len =
      snprintf(NULL, 0, "P%s%s%s", mut ? "" : "C", !vol ? "" : "V", type->id);
  char id[len + 1];
  sprintf(id, "P%s%s%s", mut ? "" : "C", !vol ? "" : "V", type->id);
  type_t ptype = context_load_type(ctx, id);
  if (!ptype) {
    len = snprintf(NULL, 0, "*%s%s%s", mut ? "" : "const ",
                   !vol ? "" : "volatile ", type->name);
    char name[len + 1];
    sprintf(name, "*%s%s%s", mut ? "" : "const ", !vol ? "" : "volatile ",
            type->name);
    ptr_meta_t meta = create_ptr_meta(ctx->allocator, type, mut, vol);
    struct _type_operator_t opt = {
        .type_equal = ptr_type_equal,
        .opt_eq = ptr_eq,
        .opt_ne = ptr_ne,
        .get_field = ptr_get_field,
        .set_field = ptr_set_field,
        .safe_convert = ptr_safe_convert,
    };
    ptype = create_type(ctx->allocator, TYPE_KIND_PTR, name, id, sizeof(void *),
                        sizeof(void *), &opt, meta, type->comptime);
    context_store_type(ctx, ptype);
  }
  return ptype;
}
type_t create_parray_type(context_t ctx, type_t type, bool mut, bool vol) {
  size_t len =
      snprintf(NULL, 0, "PA%s%s%s", mut ? "" : "C", !vol ? "" : "V", type->id);
  char id[len + 1];
  sprintf(id, "PA%s%s%s", mut ? "" : "C", !vol ? "" : "V", type->id);
  type_t ptype = context_load_type(ctx, id);
  if (!ptype) {
    len = snprintf(NULL, 0, "[*]%s%s%s", mut ? "" : "const ",
                   !vol ? "" : "volatile ", type->name);
    char name[len + 1];
    sprintf(name, "[*]%s%s%s", mut ? "" : "const ", !vol ? "" : "volatile ",
            type->name);
    ptr_meta_t meta = create_ptr_meta(ctx->allocator, type, mut, vol);
    struct _type_operator_t opt = {
        .type_equal = ptr_type_equal,
        .opt_eq = ptr_eq,
        .opt_ne = ptr_ne,
    };
    ptype = create_type(ctx->allocator, TYPE_KIND_PARRAY, name, id,
                        sizeof(void *), sizeof(void *), &opt, meta, false);
    context_store_type(ctx, ptype);
  }
  return ptype;
}
type_t ptr_type_get_type(type_t type) {
  ptr_meta_t meta = type->meta;
  return meta->type;
}
bool ptr_type_is_mut(type_t type) {
  ptr_meta_t meta = type->meta;
  return meta->mut;
}
bool ptr_type_is_vol(type_t type) {
  ptr_meta_t meta = type->meta;
  return meta->vol;
}