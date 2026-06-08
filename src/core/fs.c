#include "core/fs.h"
#include "core/allocator.h"
#include <stdio.h>

char *fs_read_file(allocator_t allocator, const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    return NULL;
  }
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char *buf = allocator_alloc(allocator, len + 1, NULL);
  fread(buf, len, 1, fp);
  fclose(fp);
  buf[len] = 0;
  return buf;
}
bool fs_is_exists(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    return false;
  } else {
    fclose(file);
    return true;
  }
}