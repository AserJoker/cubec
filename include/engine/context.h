#ifndef _H_CUBEC_ENGINE_CONTEXT_
#define _H_CUBEC_ENGINE_CONTEXT_
#include "core/allocator.h"
#include "engine/diagnostic.h"
#include "core/class.h"
#include "engine/vm.h"
#ifdef __cplusplus
extern "C" {
#endif

struct context {
  allocator_t allocator;
  vm_t vm;                   /**< owned: VM instance */
  diagnostic_list_t diagnostics;
};

typedef struct context *context_t;

extern class_t g_context_class;

context_t context_create(allocator_t allocator);

void context_dispose(context_t ctx);

int context_get_error_count(context_t ctx);

#ifdef __cplusplus
}
#endif
#endif
