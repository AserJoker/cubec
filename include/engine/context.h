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

typedef struct _cubec_static_scope_t *cubec_static_scope_t;
struct _cubec_static_scope_t {
  cubec_value_t binding;
  cubec_static_scope_t parent;
};

typedef struct _cubec_context_t *cubec_context_t;

cubec_context_t cubec_create_context(cubec_allocator_t allocator);

cubec_allocator_t cubec_context_get_allocator(cubec_context_t self);

bool cubec_context_is_comptime(cubec_context_t ctx);

void cubec_context_set_comptime(cubec_context_t ctx, bool comptime);

cubec_value_t cubec_context_load_module(cubec_context_t self,
                                        const char *filename);

cubec_value_t cubec_context_create_value(cubec_context_t self,
                                         cubec_type_t type, bool mutable,
                                         void *data, const char *name);

cubec_value_t cubec_context_create_type(cubec_context_t self,
                                        cubec_type_kind_t kind, size_t size,
                                        size_t align, void *meta,
                                        cubec_type_operator_t opt,
                                        const char *name);
cubec_value_t cubec_context_create_interrupt(cubec_context_t self,
                                             cubec_value_t value);

cubec_value_t cubec_context_load(cubec_context_t self, const char *name);
cubec_type_t cubec_context_load_type(cubec_context_t self, const char *name);
cubec_value_t cubec_context_declar(cubec_context_t self, const char *name,
                                   cubec_value_t value);
void cubec_context_push_static_scope(cubec_context_t self, cubec_value_t value);
void cubec_context_pop_static_scope(cubec_context_t self);
cubec_static_scope_t cubec_context_get_static_scope(cubec_context_t self);

char *const cubec_context_create_cstring(cubec_context_t self, const char *src);

cubec_value_t cubec_context_get_undefined(cubec_context_t self);

void cubec_context_push_scope(cubec_context_t self);
void cubec_context_pop_scope(cubec_context_t self);
cubec_scope_t cubec_context_get_scope(cubec_context_t self);
void cubec_context_set_scope(cubec_context_t self, cubec_scope_t scope);
#ifdef __cplusplus
}
#endif
#endif