#include "engine/error.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/struct_type.h"
#include "engine/type.h"
#include "core/string.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define ERROR_MESSAGE_SIZE 128

value_t create_error_value(vm_t vm, uint64_t error_code, const char *fmt, ...) {
  type_t error_type = (type_t)value_get_data(vm_get_error_type(vm));
  struct_type_t st = (struct_type_t)error_type;
  allocator_t alloc = vm_get_allocator(vm);

  /* zero-initialize the struct buffer */
  uint64_t sz = type_get_size(error_type);
  void *data = allocator_alloc(alloc, sz);
  if (!data) return NULL;
  memset(data, 0, (size_t)sz);

  /* fill message field (field 0) */
  vec_t fields = struct_type_get_fields(st);
  if (!fields || vec_get_size(fields) < 2) {
    allocator_free(alloc, &data);
    return NULL;
  }
  field_info_t msg_fi = (field_info_t)vec_get(fields, 0);
  uint64_t msg_offset = field_info_get_offset(msg_fi);
  char *msg_buf = (char *)((uint8_t *)data + msg_offset);
  if (fmt) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg_buf, ERROR_MESSAGE_SIZE, fmt, ap);
    va_end(ap);
  }

  /* fill error_code field (field 1) */
  field_info_t code_fi = (field_info_t)vec_get(fields, 1);
  uint64_t code_offset = field_info_get_offset(code_fi);
  memcpy((uint8_t *)data + code_offset, &error_code, sizeof(uint64_t));

  /* backtrace and backtrace_count are already zeroed by memset */

  value_t v = value_create(alloc, error_type, data, true);
  value_set_initialized(v, true);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}
