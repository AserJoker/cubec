#include "engine/error_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "core/string.h"
#include <stdarg.h>
#include <stdio.h>

/* ---- Error type vtable ---- */

static value_t _error_clone(allocator_t allocator, value_t self) {
  struct error_data_t *src = (struct error_data_t *)value_get_data(self);
  struct error_data_t *copy = (struct error_data_t *)allocator_alloc(
      allocator, sizeof(struct error_data_t));
  copy->message = src->message ? cstring_clone(allocator, src->message) : NULL;
  return value_create(allocator, value_get_type(self), copy, true);
}

static void _error_dispose(allocator_t allocator, value_t self) {
  struct error_data_t *d = (struct error_data_t *)value_get_data(self);
  if (d) {
    allocator_free(allocator, &d->message);
    allocator_free(allocator, &d);
  }
}

type_t type_get_error_type(allocator_t allocator) {
  (void)allocator;
  static struct _type_t error_type = {
      .kind  = TYPE_KIND_ERROR,
      .name  = (char *)"error",
      .size  = sizeof(struct error_data_t),
      .align = _Alignof(struct error_data_t),
      .mut   = false,
      .vtable = {
          .clone = _error_clone,
          .dispose = _error_dispose,
          .equal = NULL,
          .extends = NULL,
          .type_equal = NULL,
          .type_extends = NULL,
      },
  };
  return &error_type;
}

value_t create_error_value(vm_t vm, const char *fmt, ...) {
  allocator_t allocator = vm_get_allocator(vm);

  struct error_data_t *data = (struct error_data_t *)allocator_alloc(
      allocator, sizeof(struct error_data_t));

  if (fmt) {
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (len > 0) {
      char *buf = (char *)allocator_alloc(allocator, (size_t)len + 1);
      va_start(args, fmt);
      vsnprintf(buf, (size_t)len + 1, fmt, args);
      va_end(args);
      buf[len] = '\0';
      data->message = buf;
    } else {
      data->message = NULL;
    }
  } else {
    data->message = NULL;
  }

  type_t error_type = (type_t)value_get_data(vm_get_error_type(vm));
  value_t v = value_create(allocator, error_type, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->values, v);
  }
  return v;
}
