#ifndef _H_CUBEC_ENGINE_CONTEXT_
#define _H_CUBEC_ENGINE_CONTEXT_
#include "core/allocator.h"
#include "engine/scope.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_context_t *cubec_context_t;
struct _cubec_context_t {
  cubec_scope_t root;
  cubec_scope_t current;
};
cubec_context_t cubec_create_context(cubec_allocator_t allocator);
#ifdef __cplusplus
}
#endif
#endif