#ifndef _H_CUBEC_ENGINE_CONTEXT_
#define _H_CUBEC_ENGINE_CONTEXT_
#include "core/allocator.h"
#include "core/diagnostic.h"
#include "core/node.h"
#include "core/source.h"
#include "core/type.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

struct context {
  allocator_t allocator;
  diagnostic_list_t diagnostics;
  source_cache_t sources;
};

typedef struct context *context_t;

extern type_t g_context_type;

context_t context_create(allocator_t allocator);

void context_dispose(context_t ctx);

int context_get_error_count(context_t ctx);

#ifdef __cplusplus
}
#endif
#endif
