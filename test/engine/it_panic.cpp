#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/exception_type.h"
#include "engine/str_type.h"
#include "engine/integer_type.h"
#include "engine/callable_type.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_panic : public CubecTest {
protected:
};

TEST_F(it_panic, panic_is_registered_in_global_scope) {
  vm_t vm = vm_create(allocator);
  scope_t global = vm_get_global_scope(vm);

  name_t n = scope_lookup(global, "panic");
  ASSERT_NE(n, nullptr);
  value_t pv = n->ref;
  ASSERT_NE(pv, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(pv)), TYPE_KIND_CALLABLE);

  vm_dispose(vm, allocator);
}

TEST_F(it_panic, panic_returns_exception) {
  vm_t vm = vm_create(allocator);

  /* find panic in global scope */
  name_t n = scope_lookup(vm_get_global_scope(vm), "panic");
  ASSERT_NE(n, nullptr);
  value_t panic_fn = n->ref;

  /* call panic("something went wrong") */
  value_t msg = create_str_value(vm, "something went wrong");
  value_t argv[] = {msg};
  value_t result = value_call(vm, panic_fn, 1, argv);

  /* result should be an exception */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

TEST_F(it_panic, panic_message_contains_input) {
  vm_t vm = vm_create(allocator);

  name_t n = scope_lookup(vm_get_global_scope(vm), "panic");
  ASSERT_NE(n, nullptr);
  value_t panic_fn = n->ref;

  value_t msg = create_str_value(vm, "test error message");
  value_t argv[] = {msg};
  value_t result = value_call(vm, panic_fn, 1, argv);

  ASSERT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);
  const char *emsg = ((struct exception_data_t *)value_get_data(result))->message;
  EXPECT_NE(emsg, nullptr);
  EXPECT_NE(strstr(emsg, "test error message"), nullptr);
  EXPECT_NE(strstr(emsg, "panic"), nullptr);

  vm_dispose(vm, allocator);
}

TEST_F(it_panic, panic_no_args_returns_exception) {
  vm_t vm = vm_create(allocator);

  name_t n = scope_lookup(vm_get_global_scope(vm), "panic");
  ASSERT_NE(n, nullptr);
  value_t panic_fn = n->ref;

  /* call panic() with no args 鈫?argc check in _callable_call rejects */
  value_t result = value_call(vm, panic_fn, 0, NULL);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

TEST_F(it_panic, panic_non_str_arg_returns_exception) {
  vm_t vm = vm_create(allocator);

  name_t n = scope_lookup(vm_get_global_scope(vm), "panic");
  ASSERT_NE(n, nullptr);
  value_t panic_fn = n->ref;

  /* call panic(42) 鈥?wrong type 鈫?safe_cast to str fails 鈫?exception */
  value_t i32_val = create_i32_value(vm, 42);
  value_t argv[] = {i32_val};
  value_t result = value_call(vm, panic_fn, 1, argv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

TEST_F(it_panic, panic_callable_type_signature) {
  vm_t vm = vm_create(allocator);

  name_t n = scope_lookup(vm_get_global_scope(vm), "panic");
  ASSERT_NE(n, nullptr);
  value_t panic_fn = n->ref;

  callable_type_t ct = (callable_type_t)value_get_type(panic_fn);
  EXPECT_EQ(callable_type_get_param_count(ct), 1u);
  EXPECT_FALSE(callable_type_is_variadic(ct));

  /* parameter type is str */
  type_t param_t = callable_type_get_param_type(ct, 0);
  EXPECT_EQ(type_get_kind(param_t), TYPE_KIND_STR);

  /* return type is void */
  type_t ret_t = callable_type_get_return_type(ct);
  EXPECT_EQ(type_get_kind(ret_t), TYPE_KIND_VOID);

  vm_dispose(vm, allocator);
}

/* ---- builtins strmap ---- */

TEST_F(it_panic, panic_available_via_vm_get_builtin) {
  vm_t vm = vm_create(allocator);

  value_t panic_fn = vm_get_builtin(vm, "panic");
  ASSERT_NE(panic_fn, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(panic_fn)), TYPE_KIND_CALLABLE);

  /* same value as scope_lookup */
  name_t n = scope_lookup(vm_get_global_scope(vm), "panic");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(panic_fn, n->ref);

  vm_dispose(vm, allocator);
}

TEST_F(it_panic, vm_get_builtin_unknown_returns_null) {
  vm_t vm = vm_create(allocator);

  value_t v = vm_get_builtin(vm, "nonexistent");
  EXPECT_EQ(v, nullptr);

  vm_dispose(vm, allocator);
}

TEST_F(it_panic, vm_add_builtin_clones_into_global_scope) {
  vm_t vm = vm_create(allocator);
  scope_t global = vm_get_global_scope(vm);

  /* create a value and add it as a builtin */
  value_t i32_val = create_i32_value(vm, 99);
  /* switch away from global scope to prove clone lands in global */
  scope_t child = scope_create(allocator, SCOPE_BLOCK, global, NULL);
  vm_set_scope(vm, child);

  size_t before = vec_get_size(global->values);

  value_t cloned = vm_add_builtin(vm, "my_const", i32_val);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(cloned)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(cloned), 99);

  /* cloned value registered in global scope (+1) */
  EXPECT_EQ(vec_get_size(global->values), before + 1);

  /* retrievable via vm_get_builtin */
  value_t found = vm_get_builtin(vm, "my_const");
  EXPECT_EQ(found, cloned);

  /* not the same pointer as original (it's a clone) */
  EXPECT_NE(cloned, i32_val);

  vm_set_scope(vm, global);
  vm_dispose(vm, allocator);
}
