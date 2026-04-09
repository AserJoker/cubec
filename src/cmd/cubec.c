#include "core/allocator.h"
#include "core/path.h"
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

int main(int argc, char *argv[]) {
  cubec_allocator_t allocator = cubec_create_allocator(NULL);
  cubec_context_t ctx = cubec_create_context(allocator);
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