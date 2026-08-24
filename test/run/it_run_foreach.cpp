#include "run/run.h"
#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/str_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/interrupt_type.h"
#include "engine/callable_type.h"
#include "engine/struct_type.h"
#include "engine/pointer_type.h"
#include "engine/union_type.h"
#include "engine/error.h"
#include "engine/result_type.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement.h"
#include "cubec/expression.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "cubec/node.h"
#include "core/location.h"
#include "core/string.h"
#include "core/vec.h"
#include "core/class.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ================================================================== *
 *  C functions for iterator protocol                                  *
 * ================================================================== */

/* Global vm pointer for C callback functions (set per test) */
static vm_t g_test_vm = NULL;

/* IterResult struct: { done: bool, value: i32 } */
static value_t g_iter_result_type_val = NULL;

/* RangeIter struct: { current: i32, max: i32 }
 * next() -> IterResult { done: false, value: current }
 *           IterResult { done: true } when current >= max */
static value_t _range_iter_next(vm_t vm, value_t fn, size_t argc,
                                value_t *argv) {
  (void)fn;
  (void)argc;
  /* argv[0] = addrof(self) — pointer to RangeIter */
  value_t self_ptr = argv[0];
  value_t self = value_deref_get(vm, self_ptr);
  if (value_is_abnormal(self)) return self;

  value_t current_val = value_get_field(vm, self, "current");
  if (value_is_abnormal(current_val)) return current_val;
  value_t max_val = value_get_field(vm, self, "max");
  if (value_is_abnormal(max_val)) return max_val;

  int32_t current = *(int32_t *)value_get_data(current_val);
  int32_t max = *(int32_t *)value_get_data(max_val);

  if (current >= max) {
    /* Done: IterResult { done: true, value: 0 } */
    value_t done_field = create_bool_value(vm, true);
    value_t val_field = create_i32_value(vm, 0);
    value_t fields[] = {done_field, val_field};
    value_t result = vm_create_struct_value(vm, g_iter_result_type_val, fields);
    return result;
  }

  /* Not done: IterResult { done: false, value: current }
   * Advance current by writing directly to the struct's data buffer.
   * self is a borrowed reference from deref_get, its data points to
   * the original RangeIter's data buffer. The "current" field offset
   * is 0 (first field), so we can directly write. */
  int32_t next_val = current + 1;
  memcpy(value_get_data(self), &next_val, sizeof(int32_t));

  value_t done_field = create_bool_value(vm, false);
  value_t val_field = create_i32_value(vm, current);
  value_t fields[] = {done_field, val_field};
  value_t result = vm_create_struct_value(vm, g_iter_result_type_val, fields);
  return result;
}

/* Range struct: { start: i32, end: i32 }
 * __iter__() -> RangeIter { current: start, max: end } */
static value_t g_range_iter_type_val = NULL;

static value_t _range_iter(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  (void)fn;
  (void)argc;
  value_t self_ptr = argv[0];
  value_t self = value_deref_get(vm, self_ptr);
  if (value_is_abnormal(self)) return self;

  value_t start_val = value_get_field(vm, self, "start");
  if (value_is_abnormal(start_val)) return start_val;
  value_t end_val = value_get_field(vm, self, "end");
  if (value_is_abnormal(end_val)) return end_val;

  int32_t start = *(int32_t *)value_get_data(start_val);
  int32_t end = *(int32_t *)value_get_data(end_val);

  /* Create RangeIter { current: start, max: end } */
  value_t current_field = create_i32_value(vm, start);
  value_t max_field = create_i32_value(vm, end);
  value_t fields[] = {current_field, max_field};
  value_t iter = vm_create_struct_value(vm, g_range_iter_type_val, fields);
  return iter;
}

/* ================================================================== *
 *  Test fixture                                                       *
 * ================================================================== */

class it_run_foreach : public CubecTest {
protected:
  value_t g_range_type_val = NULL;

  void SetUp() override {
    CubecTest::SetUp();
    g_test_vm = vm;
    _setup_iterator_types();
  }

  void _setup_iterator_types() {
    /* Create IterResult struct type: { done: bool, value: i32 } */
    g_iter_result_type_val =
        vm_create_struct_type_value(vm, "IterResult", true, "<test>");
    vm_struct_add_field(vm, g_iter_result_type_val, "done",
                        vm_get_bool_type(vm), true);
    vm_struct_add_field(vm, g_iter_result_type_val, "value",
                        vm_get_i32_type(vm), true);
    vm_struct_seal(vm, g_iter_result_type_val);

    /* Create RangeIter struct type: { current: i32, max: i32 }
     * with next() method */
    g_range_iter_type_val =
        vm_create_struct_type_value(vm, "RangeIter", true, "<test>");
    vm_struct_add_field(vm, g_range_iter_type_val, "current",
                        vm_get_i32_type(vm), true);
    vm_struct_add_field(vm, g_range_iter_type_val, "max",
                        vm_get_i32_type(vm), true);
    vm_struct_seal(vm, g_range_iter_type_val);

    /* next() method: (*RangeIter) -> IterResult
     * Struct member_call inserts addrof(self) as argv[0] */
    {
      allocator_t alloc = vm_get_allocator(vm);
      vec_init_t pvi = {.auto_dispose = false};
      vec_t param_types = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
      /* param[0] = *RangeIter (self pointer) */
      type_t range_iter_type =
          (type_t)value_get_data(g_range_iter_type_val);
      pointer_type_t ptr_type = pointer_type_create(alloc, range_iter_type,
                                                      true, false);
      vec_push(vm_get_types(vm), ptr_type);
      vec_push(param_types, (type_t)ptr_type);
      type_t result_struct_type =
          (type_t)value_get_data(g_iter_result_type_val);
      callable_type_t ct = callable_type_create(alloc, param_types,
                                                 result_struct_type,
                                                 false, true, "<test>");
      allocator_free(alloc, &param_types);
      vec_push(vm_get_types(vm), ct);
      value_t next_fn = create_callable_value(vm, ct, _range_iter_next, "next");
      vm_struct_add_prop(vm, g_range_iter_type_val, "next", next_fn, true,
                         true);
    }

    /* Create Range struct type: { start: i32, end: i32 }
     * with __iter__() method */
    g_range_type_val =
        vm_create_struct_type_value(vm, "Range", true, "<test>");
    vm_struct_add_field(vm, g_range_type_val, "start", vm_get_i32_type(vm),
                        true);
    vm_struct_add_field(vm, g_range_type_val, "end", vm_get_i32_type(vm),
                        true);
    vm_struct_seal(vm, g_range_type_val);

    /* __iter__() method: (*Range) -> RangeIter
     * Struct member_call inserts addrof(self) as argv[0] */
    {
      allocator_t alloc = vm_get_allocator(vm);
      vec_init_t pvi = {.auto_dispose = false};
      vec_t param_types = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
      /* param[0] = *Range (self pointer) */
      type_t range_type = (type_t)value_get_data(g_range_type_val);
      pointer_type_t ptr_type = pointer_type_create(alloc, range_type,
                                                      true, false);
      vec_push(vm_get_types(vm), ptr_type);
      vec_push(param_types, (type_t)ptr_type);
      type_t iter_struct_type =
          (type_t)value_get_data(g_range_iter_type_val);
      callable_type_t ct = callable_type_create(alloc, param_types,
                                                 iter_struct_type,
                                                 false, true, "<test>");
      allocator_free(alloc, &param_types);
      vec_push(vm_get_types(vm), ct);
      value_t iter_fn = create_callable_value(vm, ct, _range_iter, "__iter__");
      vm_struct_add_prop(vm, g_range_type_val, "__iter__", iter_fn, true,
                         true);
    }
  }

  value_t _make_range(int32_t start, int32_t end) {
    value_t start_val = create_i32_value(vm, start);
    value_t end_val = create_i32_value(vm, end);
    value_t fields[] = {start_val, end_val};
    return vm_create_struct_value(vm, g_range_type_val, fields);
  }

  void free_node(node_t &node) {
    if (node) allocator_free(vm_get_allocator(vm), &node);
  }

  void free_tokens(vec_t &tokens) {
    if (tokens) allocator_free(vm_get_allocator(vm), &tokens);
  }

  void _bind(const char *name, value_t val) {
    scope_t scope = vm_get_current_scope(vm);
    name_t n = name_create(scope->allocator, val);
    char *owned = cstring_clone(scope->allocator, name);
    strmap_insert(scope->names, owned, n);
    allocator_free(scope->allocator, &owned);
  }

  node_t _parse_stmt(const char *source) {
    vec_t tokens = resolve_token_list(vm, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_statement(vm, tokens, &position, "test.cubec");
    free_tokens(tokens);
    return node;
  }

  value_t _run_stmt(const char *source, bool shadow = false) {
    node_t node = _parse_stmt(source);
    value_t v = run_statement(vm, node, shadow);
    free_node(node);
    return v;
  }

  node_t _parse_expr(const char *source) {
    vec_t tokens = resolve_token_list(vm, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_expression(vm, tokens, &position, "test.cubec");
    free_tokens(tokens);
    return node;
  }

  value_t _run_expr(const char *source, bool shadow = false) {
    node_t node = _parse_expr(source);
    value_t v = run_expression(vm, node, shadow);
    free_node(node);
    return v;
  }

  value_t _make_func_with_return_type(type_t ret_type,
                                       const char *name = "test_fn") {
    allocator_t alloc = vm_get_allocator(vm);
    vec_init_t pvi = {.auto_dispose = false};
    vec_t param_types = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
    callable_type_t ct = callable_type_create(alloc, param_types, ret_type,
                                               false, true, "<module>");
    allocator_free(alloc, &param_types);
    vec_push(vm_get_types(vm), ct);
    return create_callable_value(vm, ct,
        (value_t (*)(vm_t, value_t, size_t, value_t *))NULL, name);
  }
};

/* ================================================================== *
 *  Basic foreach with var                                             *
 * ================================================================== */

TEST_F(it_run_foreach, var_basic_iteration) {
  _bind("range", _make_range(0, 5));
  _bind("sum", create_i32_value(vm, 0));
  value_t v = _run_stmt("foreach(var item of range) { sum = sum + item; }");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  value_t sum_val = _run_expr("sum");
  ASSERT_NE(sum_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(sum_val), 10);  /* 0+1+2+3+4 */
}

TEST_F(it_run_foreach, var_empty_range) {
  _bind("range", _make_range(3, 3));
  _bind("count", create_i32_value(vm, 0));
  value_t v = _run_stmt("foreach(var item of range) { count = count + 1; }");
  ASSERT_NE(v, nullptr);

  value_t count_val = _run_expr("count");
  ASSERT_NE(count_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(count_val), 0);
}

TEST_F(it_run_foreach, var_single_element) {
  _bind("range", _make_range(7, 8));
  _bind("result", create_i32_value(vm, 0));
  value_t v = _run_stmt("foreach(var item of range) { result = item; }");
  ASSERT_NE(v, nullptr);

  value_t result_val = _run_expr("result");
  ASSERT_NE(result_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(result_val), 7);
}

/* ================================================================== *
 *  foreach without var (lvalue mode)                                  *
 * ================================================================== */

TEST_F(it_run_foreach, lvalue_basic_iteration) {
  _bind("range", _make_range(0, 3));
  _bind("item", create_i32_value(vm, 0));
  _bind("sum", create_i32_value(vm, 0));
  value_t v = _run_stmt("foreach(item of range) { sum = sum + item; }");
  ASSERT_NE(v, nullptr);

  value_t sum_val = _run_expr("sum");
  ASSERT_NE(sum_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(sum_val), 3);  /* 0+1+2 */
}

/* ================================================================== *
 *  Break in foreach                                                   *
 * ================================================================== */

TEST_F(it_run_foreach, break_exits_early) {
  _bind("range", _make_range(0, 100));
  _bind("sum", create_i32_value(vm, 0));
  value_t v = _run_stmt("foreach(var item of range) { if(item == 5) { break; } sum = sum + item; }");
  ASSERT_NE(v, nullptr);

  value_t sum_val = _run_expr("sum");
  ASSERT_NE(sum_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(sum_val), 10);  /* 0+1+2+3+4 */
}

TEST_F(it_run_foreach, break_does_not_propagate_past_loop) {
  _bind("range", _make_range(0, 5));
  value_t v = _run_stmt("foreach(var item of range) { break; }");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ================================================================== *
 *  Continue in foreach                                                *
 * ================================================================== */

TEST_F(it_run_foreach, continue_skips_body_rest) {
  _bind("range", _make_range(0, 6));
  _bind("sum", create_i32_value(vm, 0));
  /* Skip even: sum odd 1..5 */
  value_t v = _run_stmt("foreach(var item of range) { if(item - item / 2 * 2 == 0) { continue; } sum = sum + item; }");
  ASSERT_NE(v, nullptr);

  value_t sum_val = _run_expr("sum");
  ASSERT_NE(sum_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(sum_val), 9);  /* 1+3+5 */
}

TEST_F(it_run_foreach, continue_does_not_propagate_past_loop) {
  _bind("range", _make_range(0, 5));
  value_t v = _run_stmt("foreach(var item of range) { continue; }");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ================================================================== *
 *  Return in foreach                                                  *
 * ================================================================== */

TEST_F(it_run_foreach, return_propagates_out_of_loop) {
  _bind("range", _make_range(0, 5));
  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t func_val = _make_func_with_return_type(i32_type);

  value_t prev_func = vm_set_current_func(vm, func_val);
  value_t v = _run_stmt("foreach(var item of range) { if(item == 3) { return item; } }");
  vm_set_current_func(vm, prev_func);

  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_INTERRUPT);
  EXPECT_EQ(interrupt_get_kind(v), INTERRUPT_KIND_RETURN);
  value_t inner = interrupt_get_value(v);
  ASSERT_NE(inner, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(inner), 3);
}

/* ================================================================== *
 *  Exception propagation                                              *
 * ================================================================== */

TEST_F(it_run_foreach, exception_in_body_propagates) {
  _bind("range", _make_range(0, 5));
  _bind("x", create_i32_value(vm, 42));
  value_t v = _run_stmt("foreach(var item of range) { x.!; }");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

TEST_F(it_run_foreach, exception_in_iterator_propagates) {
  _bind("x", create_i32_value(vm, 42));
  /* x has no __iter__ method → exception */
  value_t v = _run_stmt("foreach(var item of x) {}");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ================================================================== *
 *  Nested foreach                                                     *
 * ================================================================== */

TEST_F(it_run_foreach, nested_foreach_counts_correctly) {
  _bind("outer", _make_range(0, 3));
  _bind("inner", _make_range(0, 2));
  _bind("count", create_i32_value(vm, 0));
  value_t v = _run_stmt("foreach(var i of outer) { foreach(var j of inner) { count = count + 1; } }");
  ASSERT_NE(v, nullptr);

  value_t count_val = _run_expr("count");
  ASSERT_NE(count_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(count_val), 6);  /* 3*2 */
}

/* ================================================================== *
 *  Shadow mode                                                        *
 * ================================================================== */

TEST_F(it_run_foreach, shadow_basic) {
  _bind("range", _make_range(0, 5));
  node_t node = _parse_stmt("foreach(var item of range) {}");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_foreach, shadow_break_consumed) {
  _bind("range", _make_range(0, 5));
  node_t node = _parse_stmt("foreach(var item of range) { break; }");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_foreach, shadow_continue_consumed) {
  _bind("range", _make_range(0, 5));
  node_t node = _parse_stmt("foreach(var item of range) { continue; }");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_foreach, shadow_return_propagates) {
  _bind("range", _make_range(0, 5));
  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t func_val = _make_func_with_return_type(i32_type);

  value_t prev_func = vm_set_current_func(vm, func_val);
  node_t node = _parse_stmt("foreach(var item of range) { return 42; }");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  vm_set_current_func(vm, prev_func);

  ASSERT_NE(v, nullptr);
  if (value_is_interrupt(v)) {
    EXPECT_EQ(interrupt_get_kind(v), INTERRUPT_KIND_RETURN);
  }
}
