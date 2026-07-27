#include "c/mangle.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* FNV-1a hash — 32-bit */
static uint32_t fnv1a_32(const char *data, size_t len) {
  uint32_t hash = 0x811c9dc5;
  for (size_t i = 0; i < len; i++) {
    hash ^= (uint8_t)data[i];
    hash *= 0x01000193;
  }
  return hash;
}

string_t mangle_module_hash(allocator_t allocator, const char *path) {
  uint32_t hash = fnv1a_32(path, strlen(path));
  /* Take lower 16 bits as 4-char hex, prefix with 'm' to ensure valid C ident */
  char buf[8];
  snprintf(buf, sizeof(buf), "m%04x", hash & 0xFFFF);
  return allocator_create(allocator, &g_string_type,
                           &(string_init_t){.str = buf});
}

string_t mangle_name(allocator_t allocator, const char *module_hash,
                      const char *name) {
  string_t result = allocator_create(allocator, &g_string_type,
                                      &(string_init_t){.str = module_hash});
  string_concat(result, "_");
  string_concat(result, name);
  return result;
}

string_t mangle_method(allocator_t allocator, const char *module_hash,
                        const char *type_name, const char *method_name) {
  string_t result = allocator_create(allocator, &g_string_type,
                                      &(string_init_t){.str = module_hash});
  string_concat(result, "_");
  string_concat(result, type_name);
  string_concat(result, "_");
  string_concat(result, method_name);
  return result;
}

string_t mangle_generic(allocator_t allocator, const char *module_hash,
                         const char *name, vec_t type_arg_names) {
  string_t result = allocator_create(allocator, &g_string_type,
                                      &(string_init_t){.str = module_hash});
  string_concat(result, "_");
  string_concat(result, name);
  size_t count = type_arg_names ? vec_get_size(type_arg_names) : 0;
  for (size_t i = 0; i < count; i++) {
    const char *arg = (const char *)vec_get(type_arg_names, i);
    string_concat(result, "_");
    string_concat(result, arg);
  }
  return result;
}

string_t mangle_enum_item(allocator_t allocator, const char *module_hash,
                           const char *type_name, const char *item_name) {
  string_t result = allocator_create(allocator, &g_string_type,
                                      &(string_init_t){.str = module_hash});
  string_concat(result, "_");
  string_concat(result, type_name);
  string_concat(result, "_");
  string_concat(result, item_name);
  return result;
}

string_t mangle_static_field(allocator_t allocator, const char *module_hash,
                              const char *type_name, const char *field_name) {
  string_t result = allocator_create(allocator, &g_string_type,
                                      &(string_init_t){.str = module_hash});
  string_concat(result, "_");
  string_concat(result, type_name);
  string_concat(result, "_");
  string_concat(result, field_name);
  return result;
}

string_t mangle_nested_func(allocator_t allocator, const char *module_hash,
                             const char *parent_name, int index) {
  string_t result = allocator_create(allocator, &g_string_type,
                                      &(string_init_t){.str = module_hash});
  string_concat(result, "_");
  string_concat(result, parent_name);
  string_concat(result, "__");
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", index);
  string_concat(result, buf);
  return result;
}
