#ifndef _H_CUBEC_ENGINE_CONTEXT_
#define _H_CUBEC_ENGINE_CONTEXT_
#include "core/allocator.h"
#include "engine/scope.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _static_scope_t *static_scope_t;
struct _static_scope_t {
  value_t binding;
  static_scope_t parent;
};

typedef struct _context_t *context_t;

context_t create_context(allocator_t allocator);

allocator_t context_get_allocator(context_t self);

bool context_is_comptime(context_t ctx);

bool context_set_comptime(context_t ctx, bool comptime);

value_t context_load_module(context_t self, const char *filename);

value_t context_create_value(context_t self, type_t type, bool mutable,
                             void *data, const char *name);

value_t context_create_type(context_t self, type_kind_t kind, size_t size,
                            size_t align, void *meta, type_operator_t opt,
                            const char *name);
value_t context_create_interrupt(context_t self, value_t value);

value_t context_load(context_t self, const char *name);
type_t context_load_type(context_t self, const char *name);
value_t context_declar(context_t self, const char *name, value_t value);
void context_push_static_scope(context_t self, value_t value);
void context_pop_static_scope(context_t self);
static_scope_t context_get_static_scope(context_t self);

char *const context_create_cstring(context_t self, const char *src);

value_t context_get_undefined(context_t self);

void context_push_scope(context_t self);
void context_pop_scope(context_t self);
scope_t context_get_scope(context_t self);
void context_set_scope(context_t self, scope_t scope);
#ifdef __cplusplus
}
#endif
#endif