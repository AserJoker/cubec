#include "engine/value.h"

static void _value_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  value_t value = (value_t)self;
  value->header.allocator = allocator;
  value->header.kind = DEF_VALUE;
  value->header.node = NULL;
  value->stype = NULL;
  value->type_hash = 0;
  value->data = NULL;
  value->is_export = false;
  value->is_exportlib = false;
  value->is_extern = false;
  value->is_builtin = false;
  value->is_comptime = false;
  value->is_using = false;
}

static void _value_dispose(void *self, allocator_t allocator) {
  value_t value = (value_t)self;
  /* stype is borrowing — not freed */
  /* data is owned raw buffer allocated via allocator_alloc */
  if (value->data) {
    allocator_free(allocator, &value->data);
  }
}

type_t g_value_type = {
    .size = sizeof(struct _value_t),
    .name = "cubec.engine.value",
    .init = (type_init_fn_t)_value_init,
    .dispose = (type_dispose_fn_t)_value_dispose,
};

value_t value_create(allocator_t allocator, node_t node,
                     bool is_export, bool is_exportlib, bool is_extern,
                     bool is_builtin, bool is_comptime, bool is_using) {
  value_t val = (value_t)allocator_create(allocator, &g_value_type, NULL);
  val->header.node = node;
  val->is_export = is_export;
  val->is_exportlib = is_exportlib;
  val->is_extern = is_extern;
  val->is_builtin = is_builtin;
  val->is_comptime = is_comptime;
  val->is_using = is_using;
  return val;
}
