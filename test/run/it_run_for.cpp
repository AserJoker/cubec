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
#include "engine/union_type.h"
#include "engine/error.h"
#include "engine/result_type.h"
#include "cubec/statement_for.h"
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

class it_run_for : public CubecTest {
protected:
  location_t _loc() {
    location_t loc;
    memset(&loc, 0, sizeof(loc));
    loc.filename = "test";
    return loc;
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

  value_t _make_func_with_return_type(type_t ret_type, const char *name = "test_fn") {
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
 *  Basic for loop                                                     *
 * ================================================================== */

TEST_F(it_run_for, basic_counted_loop) {
  _bind("sum", create_i32_value(vm, 0));
  _bind("i", create_i32_value(vm, 0));
  value_t v = _run_stmt("for(i = 1; i <= 5; i = i + 1) { sum = sum + i; }");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  value_t sum_val = _run_expr("sum");
  ASSERT_NE(sum_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(sum_val), 15);
}

TEST_F(it_run_for, loop_never_enters_when_condition_false) {
  _bind("i", create_i32_value(vm, 0));
  _bind("count", create_i32_value(vm, 0));
  value_t v = _run_stmt("for(i = 0; i < 0; i = i + 1) { count = count + 1; }");
  ASSERT_NE(v, nullptr);

  value_t count_val = _run_expr("count");
  ASSERT_NE(count_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(count_val), 0);
}

TEST_F(it_run_for, no_condition_is_infinite_with_break) {
  /* for(;;) with break */
  _bind("i", create_i32_value(vm, 0));
  value_t v = _run_stmt("for(;;) { i = i + 1; if(i == 5) { break; } }");
  ASSERT_NE(v, nullptr);

  value_t i_val = _run_expr("i");
  ASSERT_NE(i_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(i_val), 5);
}

TEST_F(it_run_for, no_increment) {
  /* Increment done manually in body */
  _bind("i", create_i32_value(vm, 0));
  _bind("sum", create_i32_value(vm, 0));
  value_t v = _run_stmt("for(i = 0; i < 5;) { sum = sum + i; i = i + 1; }");
  ASSERT_NE(v, nullptr);

  value_t sum_val = _run_expr("sum");
  ASSERT_NE(sum_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(sum_val), 10);
}

TEST_F(it_run_for, no_init) {
  _bind("i", create_i32_value(vm, 0));
  _bind("sum", create_i32_value(vm, 0));
  /* Init done before loop */
  value_t v = _run_stmt("for(; i < 3; i = i + 1) { sum = sum + i; }");
  ASSERT_NE(v, nullptr);

  value_t sum_val = _run_expr("sum");
  ASSERT_NE(sum_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(sum_val), 3);  /* 0+1+2 */
}

TEST_F(it_run_for, minimal_for_loop) {
  /* for(;;) {} — infinite empty loop, needs break */
  _bind("x", create_i32_value(vm, 0));
  value_t v = _run_stmt("for(;;) { x = 1; break; }");
  ASSERT_NE(v, nullptr);

  value_t x_val = _run_expr("x");
  ASSERT_NE(x_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(x_val), 1);
}

/* ================================================================== *
 *  Break in for loop                                                  *
 * ================================================================== */

TEST_F(it_run_for, break_exits_early) {
  _bind("sum", create_i32_value(vm, 0));
  _bind("i", create_i32_value(vm, 0));
  value_t v = _run_stmt("for(i = 0; i < 100; i = i + 1) { if(i == 5) { break; } sum = sum + i; }");
  ASSERT_NE(v, nullptr);

  value_t sum_val = _run_expr("sum");
  ASSERT_NE(sum_val, nullptr);
  /* sum = 0+1+2+3+4 = 10 */
  EXPECT_EQ(*(int32_t *)value_get_data(sum_val), 10);
}

TEST_F(it_run_for, break_does_not_propagate_past_loop) {
  _bind("i", create_i32_value(vm, 0));
  value_t v = _run_stmt("for(i = 0; i < 10; i = i + 1) { break; }");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ================================================================== *
 *  Continue in for loop — runs increment then re-evals condition      *
 * ================================================================== */

TEST_F(it_run_for, continue_runs_increment) {
  /* Key difference from while: continue in for runs increment first */
  _bind("sum", create_i32_value(vm, 0));
  _bind("i", create_i32_value(vm, 0));
  /* Skip even numbers: sum odd 1..5 */
  value_t v = _run_stmt("for(i = 0; i < 6; i = i + 1) { if(i - i / 2 * 2 == 0) { continue; } sum = sum + i; }");
  ASSERT_NE(v, nullptr);

  value_t sum_val = _run_expr("sum");
  ASSERT_NE(sum_val, nullptr);
  /* Odd 1..5: 1+3+5 = 9 */
  EXPECT_EQ(*(int32_t *)value_get_data(sum_val), 9);
}

TEST_F(it_run_for, continue_with_increment_side_effect) {
  /* Verify that increment runs after continue before condition */
  _bind("count", create_i32_value(vm, 0));
  _bind("i", create_i32_value(vm, 0));
  /* i starts at 0, continue when i==2 (skip body rest, but i still increments) */
  value_t v = _run_stmt("for(i = 0; i < 5; i = i + 1) { if(i == 2) { continue; } count = count + 1; }");
  ASSERT_NE(v, nullptr);

  /* count incremented for i=0,1,3,4 = 4 times */
  value_t count_val = _run_expr("count");
  ASSERT_NE(count_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(count_val), 4);
}

TEST_F(it_run_for, continue_does_not_propagate_past_loop) {
  _bind("i", create_i32_value(vm, 0));
  value_t v = _run_stmt("for(i = 0; i < 3; i = i + 1) { continue; }");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  value_t i_val = _run_expr("i");
  ASSERT_NE(i_val, nullptr);
  /* Increment still ran: i = 3 */
  EXPECT_EQ(*(int32_t *)value_get_data(i_val), 3);
}

/* ================================================================== *
 *  Return in for loop                                                 *
 * ================================================================== */

TEST_F(it_run_for, return_propagates_out_of_loop) {
  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t func_val = _make_func_with_return_type(i32_type);

  value_t prev_func = vm_set_current_func(vm, func_val);
  value_t v = _run_stmt("for(;;) { return 42; }");
  vm_set_current_func(vm, prev_func);

  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_INTERRUPT);
  EXPECT_EQ(interrupt_get_kind(v), INTERRUPT_KIND_RETURN);
  value_t inner = interrupt_get_value(v);
  ASSERT_NE(inner, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(inner), 42);
}

TEST_F(it_run_for, return_skips_increment) {
  /* Return exits immediately — increment not executed */
  _bind("i", create_i32_value(vm, 0));
  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t func_val = _make_func_with_return_type(i32_type);

  value_t prev_func = vm_set_current_func(vm, func_val);
  value_t v = _run_stmt("for(i = 0; i < 10; i = i + 1) { if(i == 3) { return i; } }");
  vm_set_current_func(vm, prev_func);

  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_INTERRUPT);
  value_t inner = interrupt_get_value(v);
  ASSERT_NE(inner, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(inner), 3);

  /* i should still be 3 — increment never ran for i==3 */
  value_t i_val = _run_expr("i");
  ASSERT_NE(i_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(i_val), 3);
}

/* ================================================================== *
 *  Exception propagation                                              *
 * ================================================================== */

TEST_F(it_run_for, exception_in_condition_propagates) {
  _bind("x", create_i32_value(vm, 42));
  value_t v = _run_stmt("for(;x.!;) {}");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

TEST_F(it_run_for, exception_in_body_propagates) {
  _bind("i", create_i32_value(vm, 0));
  _bind("x", create_i32_value(vm, 42));
  value_t v = _run_stmt("for(i = 0; i < 5; i = i + 1) { x.!; }");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

TEST_F(it_run_for, exception_in_increment_propagates) {
  _bind("i", create_i32_value(vm, 0));
  _bind("x", create_i32_value(vm, 42));
  /* First iteration body ok, then increment fails */
  value_t v = _run_stmt("for(i = 0; i < 5; x.!) { i = i + 1; }");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

TEST_F(it_run_for, exception_in_init_propagates) {
  _bind("x", create_i32_value(vm, 42));
  value_t v = _run_stmt("for(x.!; false;) {}");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ================================================================== *
 *  Nested for loops                                                   *
 * ================================================================== */

TEST_F(it_run_for, nested_for_counts_correctly) {
  _bind("count", create_i32_value(vm, 0));
  _bind("i", create_i32_value(vm, 0));
  _bind("j", create_i32_value(vm, 0));
  value_t v = _run_stmt("for(i = 0; i < 3; i = i + 1) { for(j = 0; j < 2; j = j + 1) { count = count + 1; } }");
  ASSERT_NE(v, nullptr);

  value_t count_val = _run_expr("count");
  ASSERT_NE(count_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(count_val), 6);
}

TEST_F(it_run_for, nested_break_only_exits_inner) {
  _bind("count", create_i32_value(vm, 0));
  _bind("i", create_i32_value(vm, 0));
  _bind("j", create_i32_value(vm, 0));
  value_t v = _run_stmt("for(i = 0; i < 3; i = i + 1) { for(j = 0; j < 10; j = j + 1) { count = count + 1; if(j == 1) { break; } } }");
  ASSERT_NE(v, nullptr);

  /* Inner runs 2 iterations per outer iteration, outer runs 3 times */
  value_t count_val = _run_expr("count");
  ASSERT_NE(count_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(count_val), 6);
}

/* ================================================================== *
 *  Shadow mode                                                        *
 * ================================================================== */

TEST_F(it_run_for, shadow_basic) {
  _bind("i", create_i32_value(vm, 0));
  node_t node = _parse_stmt("for(i = 0; i < 5; i = i + 1) {}");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_for, shadow_break_consumed) {
  _bind("i", create_i32_value(vm, 0));
  node_t node = _parse_stmt("for(;;) { break; }");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_for, shadow_continue_consumed) {
  _bind("i", create_i32_value(vm, 0));
  node_t node = _parse_stmt("for(;;) { continue; }");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_for, shadow_return_propagates) {
  _bind("i", create_i32_value(vm, 0));
  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t func_val = _make_func_with_return_type(i32_type);

  value_t prev_func = vm_set_current_func(vm, func_val);
  node_t node = _parse_stmt("for(;;) { return 42; }");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  vm_set_current_func(vm, prev_func);

  ASSERT_NE(v, nullptr);
  if (value_is_interrupt(v)) {
    EXPECT_EQ(interrupt_get_kind(v), INTERRUPT_KIND_RETURN);
  }
}

TEST_F(it_run_for, shadow_condition_type_error) {
  _bind("s", create_str_value(vm, "hello"));
  node_t node = _parse_stmt("for(;s;) {}");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_for, shadow_infinite_no_condition) {
  _bind("i", create_i32_value(vm, 0));
  node_t node = _parse_stmt("for(;;) { i = i + 1; }");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}
