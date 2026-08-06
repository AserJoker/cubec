#include "engine/function.h"

static void _function_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  function_t func = (function_t)self;
  func->header.allocator = allocator;
  func->header.kind = DEF_FUNCTION;
  func->header.node = NULL;
  func->is_export = false;
  func->is_exportlib = false;
  func->is_inline = false;
  func->is_extern = false;
  func->is_builtin = false;
  func->is_comptime = false;
  func->is_c_variadic = false;
  func->params = NULL;
  func->implements = NULL;
}

static void _function_dispose(void *self, allocator_t allocator) {
  function_t func = (function_t)self;
  if (func->params)
    allocator_free(allocator, &func->params);
  if (func->implements)
    allocator_free(allocator, &func->implements);
}

type_t g_function_type = {
    .size = sizeof(struct _function_t),
    .name = "cubec.engine.function",
    .init = (type_init_fn_t)_function_init,
    .dispose = (type_dispose_fn_t)_function_dispose,
};

function_t function_create(allocator_t allocator, node_t node,
                           bool is_export, bool is_exportlib, bool is_inline,
                           bool is_extern, bool is_builtin, bool is_comptime,
                           bool is_c_variadic) {
  function_t func = (function_t)allocator_create(allocator, &g_function_type, NULL);
  func->header.kind = DEF_FUNCTION;
  func->header.node = node;
  func->is_export = is_export;
  func->is_exportlib = is_exportlib;
  func->is_inline = is_inline;
  func->is_extern = is_extern;
  func->is_builtin = is_builtin;
  func->is_comptime = is_comptime;
  func->is_c_variadic = is_c_variadic;
  return func;
}

void function_dispose(function_t func) {
  allocator_free(func->header.allocator, &func);
}
