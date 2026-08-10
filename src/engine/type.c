#include "engine/type.h"

type_kind_t type_get_kind(const type_t *self) { return self->kind; }
const char *type_get_name(const type_t *self) { return self->name; }
uint64_t    type_get_size(const type_t *self) { return self->size; }
uint64_t    type_get_align(const type_t *self) { return self->align; }
