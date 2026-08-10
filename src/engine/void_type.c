#include "engine/void_type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/bool_type.h"
#include "engine/type.h"

/* ---- Void type vtable ---- */

static value_t _void_type_equal(vm_t vm, type_t a, type_t b) {
  (void)a;
  if (b->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, b->kind == TYPE_KIND_VOID);
}

static value_t _void_type_extends(vm_t vm, type_t sub, type_t super) {
  (void)sub;
  if (super->kind == TYPE_KIND_WILDCARD)
    return create_bool_value(vm, true);
  return create_bool_value(vm, super->kind == TYPE_KIND_VOID);
}

type_t type_get_void_type(allocator_t allocator) {
  (void)allocator;
  static struct _type_t void_type = {
      .kind  = TYPE_KIND_VOID,
      .name  = (char *)"void",
      .size  = 0,
      .align = 0,
      .vtable = {
          .clone = NULL,
          .dispose = NULL,
          .equal = NULL,
          .extends = NULL,
          .type_equal = _void_type_equal,
          .type_extends = _void_type_extends,
      },
  };
  return &void_type;
}
