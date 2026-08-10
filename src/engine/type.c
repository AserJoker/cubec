#include "engine/type.h"

type_kind_t type_get_kind(type_t self) { return self->kind; }
const char *type_get_name(type_t self) { return self->name; }
uint64_t    type_get_size(type_t self) { return self->size; }
uint64_t    type_get_align(type_t self) { return self->align; }
vtable_t    type_get_vtable(type_t self) { return self->vtable; }
