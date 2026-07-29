#include "c/c_type.h"
#include <stdio.h>
#include <string.h>

c_type_t c_type_create(allocator_t allocator, const char *left,
                        const char *right) {
  c_type_t type = allocator_alloc(allocator, sizeof(struct _c_type_t));
  string_init_t si = {0};
  si.str = left ? left : "";
  type->left = allocator_create(allocator, &g_string_type, &si);
  si.str = right ? right : "";
  type->right = allocator_create(allocator, &g_string_type, &si);
  return type;
}

c_type_t c_type_primitive(allocator_t allocator, const char *name) {
  return c_type_create(allocator, name, "");
}

c_type_t c_type_pointer(allocator_t allocator, c_type_t base) {
  (void)allocator;
  string_concat(base->right, "*");
  return base;
}

c_type_t c_type_const(allocator_t allocator, c_type_t base) {
  /* Build "const " + current left, then set */
  string_t tmp = allocator_create(allocator, &g_string_type,
                                   &(string_init_t){.str = "const "});
  string_concat(tmp, string_get(base->left));
  string_set(base->left, string_get(tmp));
  allocator_free(allocator, &tmp);
  return base;
}

c_type_t c_type_volatile(allocator_t allocator, c_type_t base) {
  string_t tmp = allocator_create(allocator, &g_string_type,
                                   &(string_init_t){.str = "volatile "});
  string_concat(tmp, string_get(base->left));
  string_set(base->left, string_get(tmp));
  allocator_free(allocator, &tmp);
  return base;
}

c_type_t c_type_function_ptr(allocator_t allocator, c_type_t return_type,
                               vec_t param_types, bool variadic) {
  /* Build: left = return_type.left, right = "(*" + return_type.right + ")("
   *        + param1_left param1_right, param2_left param2_right, ... + ")" */
  string_t left = allocator_create(allocator, &g_string_type,
                                    &(string_init_t){.str = string_get(return_type->left)});
  string_t right = allocator_create(allocator, &g_string_type,
                                     &(string_init_t){.str = "(*"});

  /* Append return type's right part (for function returning pointer, e.g.
   * return_type = {left="void", right="*"} → right becomes "(*" + "*" */
  if (string_get_length(return_type->right) > 0) {
    string_concat(right, string_get(return_type->right));
  }
  string_concat(right, ")(");

  /* Append parameter types */
  size_t param_count = param_types ? vec_get_size(param_types) : 0;
  for (size_t i = 0; i < param_count; i++) {
    c_type_t param = vec_get(param_types, i);
    if (i > 0) string_concat(right, ", ");
    string_concat(right, string_get(param->left));
    if (string_get_length(param->right) > 0) {
      string_concat(right, " ");
      string_concat(right, string_get(param->right));
    }
  }
  if (variadic) {
    if (param_count > 0) string_concat(right, ", ");
    string_concat(right, "...");
  }
  string_concat(right, ")");

  c_type_t type = allocator_alloc(allocator, sizeof(struct _c_type_t));
  type->left = left;
  type->right = right;
  return type;
}

c_type_t c_type_array(allocator_t allocator, c_type_t base, size_t length) {
  (void)allocator;
  char buf[32];
  snprintf(buf, sizeof(buf), "[%zu]", length);
  string_concat(base->right, buf);
  return base;
}

void c_type_dispose(allocator_t allocator, c_type_t *type) {
  if (!type || !*type) return;
  c_type_t t = *type;
  allocator_free(allocator, &t->left);
  allocator_free(allocator, &t->right);
  allocator_free(allocator, type);
}
