#include "core/allocator.h"
#include "core/path.h"
#include "engine/builtin.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char *absolute(allocator_t allocator, const char *name) {
  path_t path = create_path(allocator, name);
  path_t fullpath = path_absolute(path, allocator);
  char *result = path_to_string(fullpath, allocator);
  allocator_free(allocator, path);
  allocator_free(allocator, fullpath);
  return result;
}

value_t print(context_t ctx, size_t argc, value_t argv[]) {
  for (size_t idx = 0; idx < argc; idx++) {
    if (idx != 0) {
      printf(", ");
    }
    value_t str = value_to_string(argv[idx], ctx);
    const char *cstr = *(const char **)value_get_data(str);
    printf("%s", cstr);
  }
  printf("\n");
  return context_get_undefined(ctx);
}

int main(int argc, char *argv[]) {
  allocator_t allocator = create_allocator(NULL);
  context_t ctx = create_context(allocator);
  create_builtin(ctx, print, "print");
  char *filename = absolute(allocator, "./main.cubec");
  value_t err = context_load_module(ctx, filename);
  if (value_type_is(err, VALUE_TYPE_ERROR)) {
    const char *message = *(const char **)value_get_data(err);
    fprintf(stderr, "%s\n", message);
  }
  allocator_free(allocator, filename);
  allocator_free(allocator, ctx);
  delete_allocator(allocator);
  return 0;
}