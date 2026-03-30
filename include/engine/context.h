#ifndef _H_CUBEC_ENGINE_CONTEXT_
#define _H_CUBEC_ENGINE_CONTEXT_
#include "ast/node.h"
#include "core/allocator.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_context_t *cubec_context_t;
cubec_context_t cubec_create_context(cubec_allocator_t allocator);
void cubec_context_add_visit(cubec_context_t self, cubec_visit_ast_fn_t visit);
cubec_value_t cubec_context_load_module(cubec_context_t self,
                                        const char *filename, const char *name);
cubec_value_t cubec_context_create_value(cubec_context_t self,
                                         cubec_type_t type, bool mutable,
                                         void *data, const char *name);
cubec_type_t cubec_context_create_type(cubec_context_t self,
                                       cubec_type_kind_t kind, size_t size,
                                       void *meta);
cubec_value_t cubec_context_create_error(cubec_context_t self, const char *fmt,
                                         ...);
cubec_value_t cubec_context_load(cubec_context_t self, const char *name);
#ifdef __cplusplus
}
#endif
#endif