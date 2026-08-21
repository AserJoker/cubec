#include "engine/wildcard_type.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/bool_type.h"

static value_t _wildcard_clone(vm_t vm, value_t self) {
  type_t wt = value_get_type(self);
  allocator_t alloc = vm_get_allocator(vm);
  value_t v = value_create(alloc, wt, NULL, false);
  value_set_initialized(v, value_is_initialized(self));
  scope_t scope = vm_get_current_scope(vm);
  if (scope) vec_push(scope->values, v);
  return v;
}

static value_t _wildcard_equal(vm_t vm, value_t a, value_t b) {
  (void)vm;
  (void)a;
  /* wildcard value only equal to wildcard value;
   * concrete values match wildcard via their own equal (checking b's type) */
  return create_bool_value(vm, type_get_kind(value_get_type(b)) == TYPE_KIND_WILDCARD);
}

static value_t _wildcard_type_equal(vm_t vm, type_t a, type_t b) {
  (void)vm;
  (void)a;
  /* wildcard type only equal to wildcard type itself;
   * concrete types match wildcard via their own type_equal (b==WILDCARD → true) */
  return create_bool_value(vm, b->kind == TYPE_KIND_WILDCARD);
}

static value_t _wildcard_type_extends(vm_t vm, type_t sub, type_t super) {
  (void)vm;
  (void)sub;
  /* wildcard extends wildcard only; concrete types extend wildcard via their own type_extends */
  return create_bool_value(vm, super->kind == TYPE_KIND_WILDCARD);
}

static vtable_t _make_wildcard_vtable(void) {
  return (vtable_t){
      .clone        = _wildcard_clone,
      .equal        = _wildcard_equal,
      .type_equal   = _wildcard_type_equal,
      .type_extends = _wildcard_type_extends,
  };
}

type_t type_get_wildcard_type(allocator_t allocator) {
  type_init_t init = {
      .kind  = TYPE_KIND_WILDCARD,
      .name  = "?",
      .size  = 0,
      .align = 0,
      .mut   = false,
      .vtable = _make_wildcard_vtable(),
  };
  return (type_t)allocator_create(allocator, &g_type_class, &init);
}
