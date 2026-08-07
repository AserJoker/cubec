#include "engine/struct_field.h"
#include <stdlib.h>
#include <string.h>

struct_field_t struct_field_create(allocator_t allocator, const char *name,
                                   stype_t type, uint64_t offset, bool is_pub) {
  struct_field_t field = allocator_alloc(allocator, sizeof(struct _struct_field_t));
  field->allocator = allocator;
  field->name = strdup(name);
  field->type = type;
  field->offset = offset;
  field->is_pub = is_pub;
  return field;
}

void struct_field_dispose(struct_field_t field) {
  if (!field) return;
  free(field->name);
  field->name = NULL;
  allocator_free(field->allocator, &field);
}
