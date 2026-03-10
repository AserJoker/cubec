#ifndef _H_CUBEC_RUNTIME_VM_
#define _H_CUBEC_RUNTIME_VM_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/list.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_vm_t *cubec_vm_t;
struct _cubec_vm_t {
  cubec_list_t stack;
};
cubec_vm_t cubec_create_vm(cubec_allocator_t allocator);
cubec_value_t cubec_vm_run(cubec_vm_t self, cubec_context_t ctx,
                            cubec_ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif