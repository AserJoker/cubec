#include "engine/function_instance.h"
#include <stdlib.h>

static void _function_instance_init(void *self, allocator_t allocator,
                                    void *arg) {
  (void)arg;
  function_instance_t inst = (function_instance_t)self;
  inst->allocator = allocator;
  inst->arguments = NULL;
  inst->return_type = NULL;
  inst->body = NULL;
  inst->captures = NULL;
}

static void _function_instance_dispose(void *self, allocator_t allocator) {
  function_instance_t inst = (function_instance_t)self;
  (void)allocator;
  /* arguments are borrowing — do not free elements */
  if (inst->arguments)
    allocator_free(inst->allocator, &inst->arguments);
  /* captures are owned */
  if (inst->captures)
    allocator_free(inst->allocator, &inst->captures);
}

type_t g_function_instance_type = {
    .size = sizeof(struct _function_instance_t),
    .name = "cubec.engine.function_instance",
    .init = (type_init_fn_t)_function_instance_init,
    .dispose = (type_dispose_fn_t)_function_instance_dispose,
};

function_instance_t function_instance_create(allocator_t allocator,
                                             vec_t arguments,
                                             stype_t return_type,
                                             node_t body,
                                             vec_t captures) {
  function_instance_t inst =
      (function_instance_t)allocator_create(allocator,
                                            &g_function_instance_type, NULL);
  inst->arguments = arguments;
  inst->return_type = return_type;
  inst->body = body;
  inst->captures = captures;
  return inst;
}

void function_instance_dispose(function_instance_t inst) {
  allocator_free(inst->allocator, &inst);
}
