
#include "core/allocator.h"
#include "core/path.h"
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
  delete_allocator(allocator);
  return 0;
}