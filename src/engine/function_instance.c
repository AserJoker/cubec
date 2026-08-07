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

/* ---- Function comptime value operations ---- */

static void _dispose_value_vec(vec_t vec, allocator_t allocator) {
  if (!vec) return;
  size_t n = vec_get_size(vec);
  for (size_t i = 0; i < n; i++)
    comptime_value_dispose((comptime_value_t)vec_get(vec, i));
  allocator_free(allocator, &vec);
}

static vec_t _clone_value_vec(allocator_t allocator, vec_t src) {
  if (!src) return NULL;
  vec_init_t vi = {.auto_dispose = true};
  vec_t dst = (vec_t)allocator_create(allocator, &g_vec_type, &vi);
  size_t n = vec_get_size(src);
  for (size_t i = 0; i < n; i++) {
    comptime_value_t f = comptime_value_clone(allocator, vec_get(src, i));
    vec_push(dst, f);
  }
  return dst;
}

void function_instance_dispose_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_FUNCTION) return;
  comptime_function_t v = (comptime_function_t)val;
  /* function_ref is borrowing */
  if (v->captures)
    _dispose_value_vec(v->captures, val->allocator);
  allocator_free(val->allocator, &val);
}

comptime_value_t function_instance_clone_value(allocator_t allocator,
                                               comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_FUNCTION) return NULL;
  comptime_function_t src = (comptime_function_t)val;
  comptime_function_t dst = allocator_alloc(allocator, sizeof(struct _comptime_function_t));
  dst->header = src->header;
  dst->header.allocator = allocator;
  dst->function_ref = src->function_ref;  /* borrowing */
  dst->instance_hash = src->instance_hash;
  dst->captures = src->captures ? _clone_value_vec(allocator, src->captures) : NULL;
  return (comptime_value_t)dst;
}

uint64_t function_instance_hash_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_FUNCTION) return 0;
  comptime_function_t v = (comptime_function_t)val;
  uint64_t h = stype_compute_primitive_hash(TYPE_CALLABLE);
  if (val->type)
    h = stype_hash_mix_u64(h, val->type->instance.hash);
  h = stype_hash_mix_u64(h, (uint64_t)(uintptr_t)v->function_ref);
  h = stype_hash_mix_u64(h, v->instance_hash);
  return h;
}
