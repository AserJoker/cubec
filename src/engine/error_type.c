#include "engine/error_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "core/string.h"
#include <stdarg.h>
#include <stdio.h>

/* ---- Error type vtable ---- */

static value_t _error_clone(vm_t vm, value_t self) {
  struct error_data_t *src = (struct error_data_t *)value_get_data(self);
  if (src && src->message)
    return create_error_value(vm, "%s", src->message);
  return create_error_value(vm, NULL);
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
          .type_clone = NULL,
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

  /* First pass: compute message length */
  int len = 0;
  if (fmt) {
    va_list args;
    va_start(args, fmt);
    len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
  }

  /* Allocate error_data_t with inline message */
  size_t msg_size = (len > 0) ? (size_t)len + 1 : 1;
  struct error_data_t *data = (struct error_data_t *)allocator_alloc(
      allocator, sizeof(struct error_data_t) + msg_size);

  if (fmt && len > 0) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(data->message, (size_t)len + 1, fmt, args);
    va_end(args);
  } else {
    data->message[0] = '\0';
  }

  type_t error_type = (type_t)value_get_data(vm_get_error_type(vm));
  value_t v = value_create(allocator, error_type, data, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->values, v);
  }
  return v;
}
