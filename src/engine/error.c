#include "engine/error.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
void cubec_init_error_type(cubec_context_t ctx) {
  struct _cubec_type_operator_t opt = {};
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_ERROR, sizeof(const char **),
                            sizeof(const char **), NULL, &opt, "error");
}
cubec_value_t cubec_create_error(cubec_context_t ctx, const char *fmt, ...) {
  cubec_value_t vtype = cubec_context_load(ctx, "error");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char message[len + 1];
  va_start(args, fmt);
  vsprintf(message, fmt, args);
  va_end(args);
  const char *str = cubec_context_create_cstring(ctx, message);
  return cubec_context_create_value(ctx, type, false, &str, NULL);
}