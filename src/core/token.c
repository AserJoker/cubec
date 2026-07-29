#include "core/token.h"
struct _token_t {
  allocator_t allocator;
  uint32_t kind;
  location_t location;
};

static void _token_init(token_t self, allocator_t allocator,
                        token_init_t *init) {
  self->allocator = allocator;
  self->kind = init->kind;
  self->location = init->location;
}

static void _token_dispose(token_t self, allocator_t allocator) {
  (void)self;
  (void)allocator;
}

static void _token_clone(token_t self, allocator_t allocator, token_t another) {
  self->allocator = allocator;
  self->kind = another->kind;
  self->location = another->location;
}

type_t g_token_type = {
    .name = "cubec.core.token",
    .size = sizeof(struct _token_t),
    .init = (type_init_fn_t)_token_init,
    .dispose = (type_dispose_fn_t)_token_dispose,
    .clone = (type_clone_fn_t)_token_clone,
    .move = NULL,
};

uint32_t token_get_kind(token_t self) { return self->kind; }

location_t *token_get_location(token_t self) { return &self->location; }

bool token_is(token_t self, uint32_t kind, const char *text) {
  return self->kind == kind && (text == NULL || location_is(&self->location, text));
}

const char *token_get_string(token_t self) { return self->location.begin.offset; }

size_t token_get_string_length(token_t self) {
  return self->location.end.offset - self->location.begin.offset;
}