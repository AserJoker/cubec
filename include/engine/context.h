#ifndef _H_CUBEC_ENGINE_CONTEXT_
#define _H_CUBEC_ENGINE_CONTEXT_
#include "core/allocator.h"
#include "core/diagnostic.h"
#include "core/strmap.h"
#include "core/type.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

struct context {
  allocator_t allocator;
  diagnostic_list_t diagnostics;
  strmap_t modules;    /* absolute path (string key) → module_t */
};

typedef struct context *context_t;

extern type_t g_context_type;

context_t context_create(allocator_t allocator);

void context_dispose(context_t ctx);

int context_get_error_count(context_t ctx);

/* Module registry */
struct _module_t;
typedef struct _module_t *module_t;

module_t context_get_module(context_t ctx, const char *abs_path);

#ifdef __cplusplus
}
#endif
#endif