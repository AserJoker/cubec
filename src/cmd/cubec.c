#include "core/allocator.h"
#include "core/path.h"
#include "engine/array.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char *absolute(cubec_allocator_t allocator, const char *name) {
  cubec_path_t path = cubec_create_path(allocator, name);
  cubec_path_t fullpath = cubec_path_absolute(path, allocator);
  char *result = cubec_path_to_string(fullpath, allocator);
  cubec_allocator_free(allocator, path);
  cubec_allocator_free(allocator, fullpath);
  return result;
}

int main(int argc, char *argv[]) {
  cubec_allocator_t allocator = cubec_create_allocator(NULL);
  cubec_context_t ctx = cubec_create_context(allocator);
  cubec_type_t i8 = cubec_context_load_type(ctx, "i8");
  cubec_type_t type = cubec_create_array_type(ctx, i8, 32);
  char *str = cubec_type_to_string(type, allocator);
  printf("%s\n", str);
  cubec_allocator_free(allocator, str);
  char *filename = absolute(allocator, "./main.cubec");
  cubec_value_t err = cubec_context_load_module(ctx, filename, NULL);
  if (cubec_value_is_error(err)) {
    const char *message = *(const char **)cubec_value_get_data(err);
    fprintf(stderr, "%s\n", message);
  }
  cubec_allocator_free(allocator, filename);
  cubec_allocator_free(allocator, ctx);
  cubec_delete_allocator(allocator);
  return 0;
}