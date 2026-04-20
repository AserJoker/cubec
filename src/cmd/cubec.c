
#include "core/allocator.h"
#include "core/path.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/ref.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

char *absolute(allocator_t allocator, const char *name) {
  path_t path = create_path(allocator, name);
  path_t fullpath = path_absolute(path, allocator);
  char *result = path_to_string(fullpath, allocator);
  allocator_free(allocator, path);
  allocator_free(allocator, fullpath);
  return result;
}

int main(int argc, char *argv[]) {
  allocator_t allocator = create_allocator(NULL);
  context_t ctx = create_context(allocator);
  value_t val = create_comptime_bool(ctx, true, false, NULL);
  value_t ref = create_ref_value(ctx, val);
  char *filename = absolute(allocator, "./main.cubec");
  value_t err = context_load_module(ctx, filename);
  if (type_get_kind(value_get_type(err)) == TYPE_KIND_ERROR) {
    fprintf(stderr, "%s\n", error_get_message(err));
  }
  allocator_free(allocator, ctx);
  allocator_free(allocator, filename);
  delete_allocator(allocator);
  return 0;
}