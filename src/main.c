
#include "core/allocator.h"
#include "core/error.h"
#include <inttypes.h>
#include <stdio.h>

int _main(int argc, char *argv[]) {
  allocator_t allocator = create_allocator(NULL, NULL);
  delete_allocator(allocator);
  return 0;
onerror:
  delete_allocator(allocator);
  return -1;
}

int main(int argc, char *argv[]) {
  int res = _main(argc, argv);
  if (g_error) {
    char *err = error_to_string(g_error, NULL);
    fprintf(stderr, "%s", err);
    free(err);
    error_clear();
  }
  return res;
}