
#include "core/allocator.h"
#include "core/path.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
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
  char *filename = absolute(allocator, "./main.cubec");
  value_t err = context_load_module(ctx, filename);
  if (type_get_kind(value_get_type(err)) == TYPE_KIND_ERROR) {
    fprintf(stderr, "%s\n", error_get_message(err));
  } else {
    string_t out = context_write_module(ctx, filename);
    char *dst_filename = absolute(allocator, "./main.resolved.cubec");
    const char *str = string_get(out);
    FILE *fp = fopen(dst_filename, "w");
    fwrite(str, string_len(out), 1, fp);
    fclose(fp);
    allocator_free(allocator, dst_filename);
    allocator_free(allocator, out);
  }
  allocator_free(allocator, filename);
  allocator_free(allocator, ctx);
  delete_allocator(allocator);
  return 0;
}