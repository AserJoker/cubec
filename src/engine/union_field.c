#include "engine/union_field.h"
#include <stdlib.h>
#include <string.h>

union_field_t union_field_create(allocator_t allocator, const char *name,
                                 stype_t type, bool is_pub) {
  union_field_t field = allocator_alloc(allocator, sizeof(struct _union_field_t));
  field->allocator = allocator;
  field->name = strdup(name);
  field->type = type;
  field->is_pub = is_pub;
  return field;
}

void union_field_dispose(union_field_t field) {
  if (!field) return;
  free(field->name);
  field->name = NULL;
  allocator_free(field->allocator, &field);
}
