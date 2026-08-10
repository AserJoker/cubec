#include "engine/stype.h"

type_kind_t stype_get_kind(const stype_t *self) { return self->kind; }
const char *stype_get_name(const stype_t *self) { return self->name; }
uint64_t    stype_get_size(const stype_t *self) { return self->size; }
uint64_t    stype_get_align(const stype_t *self) { return self->align; }
