#include "core/allocator.h"
#include "core/path.h"
#include "engine/builtin.h"
#include "engine/context.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

char *absolute(cubec_allocator_t allocator, const char *name) {
  cubec_path_t path = cubec_create_path(allocator, name);
  cubec_path_t fullpath = cubec_path_absolute(path, allocator);
  char *result = cubec_path_to_string(fullpath, allocator);
  cubec_allocator_free(allocator, path);
  cubec_allocator_free(allocator, fullpath);
  return result;
}

cubec_value_t print(cubec_context_t ctx, size_t argc, cubec_value_t argv[]) {
  for (size_t idx = 0; idx < argc; idx++) {
    if (idx != 0) {
      printf(", ");
    }
    cubec_value_t str = cubec_value_to_string(argv[idx], ctx);
    const char *cstr = *(const char **)cubec_value_get_data(str);
    printf("%s", cstr);
  }
  printf("\n");
  return cubec_context_get_undefined(ctx);
}

int main(int argc, char *argv[]) {
  cubec_allocator_t allocator = cubec_create_allocator(NULL);
  cubec_context_t ctx = cubec_create_context(allocator);

  cubec_create_builtin(ctx, print, "print");

  char *filename = absolute(allocator, "./main.cubec");
  cubec_value_t err = cubec_context_load_module(ctx, filename);
  cubec_context_write_module(ctx, filename, "./main.resolved.cubec");
  if (cubec_value_is_error(err)) {
    const char *message = *(const char **)cubec_value_get_data(err);
    fprintf(stderr, "%s\n", message);
  }
  cubec_allocator_free(allocator, filename);
  cubec_allocator_free(allocator, ctx);
  cubec_delete_allocator(allocator);
  return 0;
}