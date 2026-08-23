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
#include "cubec/statement_do_while.h"
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

class it_run_do_while : public CubecTest {
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
 *  Basic do-while loop                                                *
 * ================================================================== */

TEST_F(it_run_do_while, basic_loop) {
  _bind("i", create_i32_value(vm, 0));
  _bind("sum", create_i32_value(vm, 0));
  value_t v = _run_stmt("do { sum = sum + i; i = i + 1; } while(i < 5);");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  value_t sum_val = _run_expr("sum");
  ASSERT_NE(sum_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(sum_val), 10);  /* 0+1+2+3+4 */
}

TEST_F(it_run_do_while, body_executes_at_least_once) {
  /* Condition is false from the start, but body still runs once */
  _bind("i", create_i32_value(vm, 0));
  value_t v = _run_stmt("do { i = 42; } while(false);");
  ASSERT_NE(v, nullptr);

  value_t i_val = _run_expr("i");
  ASSERT_NE(i_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(i_val), 42);
}

TEST_F(it_run_do_while, loop_exits_when_condition_becomes_false) {
  _bind("count", create_i32_value(vm, 0));
  _bind("i", create_i32_value(vm, 0));
  value_t v = _run_stmt("do { count = count + 1; i = i + 1; } while(i < 3);");
  ASSERT_NE(v, nullptr);

  value_t count_val = _run_expr("count");
  ASSERT_NE(count_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(count_val), 3);
}

/* ================================================================== *
 *  Break in do-while                                                  *
 * ================================================================== */

TEST_F(it_run_do_while, break_exits_early) {
  _bind("sum", create_i32_value(vm, 0));
  _bind("i", create_i32_value(vm, 0));
  value_t v = _run_stmt("do { if(i == 3) { break; } sum = sum + i; i = i + 1; } while(i < 100);");
  ASSERT_NE(v, nullptr);

  value_t sum_val = _run_expr("sum");
  ASSERT_NE(sum_val, nullptr);
  /* sum = 0+1+2 = 3 */
  EXPECT_EQ(*(int32_t *)value_get_data(sum_val), 3);
}

TEST_F(it_run_do_while, break_does_not_propagate_past_loop) {
  _bind("i", create_i32_value(vm, 0));
  value_t v = _run_stmt("do { break; } while(true);");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ================================================================== *
 *  Continue in do-while — re-evaluates condition                      *
 * ================================================================== */

TEST_F(it_run_do_while, continue_reevaluates_condition) {
  _bind("sum", create_i32_value(vm, 0));
  _bind("i", create_i32_value(vm, 0));
  /* Skip even numbers: sum odd 1..5 */
  value_t v = _run_stmt("do { i = i + 1; if(i - i / 2 * 2 == 0) { continue; } sum = sum + i; } while(i < 5);");
  ASSERT_NE(v, nullptr);

  value_t sum_val = _run_expr("sum");
  ASSERT_NE(sum_val, nullptr);
  /* Odd 1..5: 1+3+5 = 9 */
  EXPECT_EQ(*(int32_t *)value_get_data(sum_val), 9);
}

TEST_F(it_run_do_while, continue_does_not_propagate_past_loop) {
  _bind("i", create_i32_value(vm, 0));
  value_t v = _run_stmt("do { i = i + 1; continue; } while(i < 3);");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  value_t i_val = _run_expr("i");
  ASSERT_NE(i_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(i_val), 3);
}

/* ================================================================== *
 *  Return in do-while                                                 *
 * ================================================================== */

TEST_F(it_run_do_while, return_propagates_out_of_loop) {
  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t func_val = _make_func_with_return_type(i32_type);

  value_t prev_func = vm_set_current_func(vm, func_val);
  value_t v = _run_stmt("do { return 42; } while(true);");
  vm_set_current_func(vm, prev_func);

  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_INTERRUPT);
  EXPECT_EQ(interrupt_get_kind(v), INTERRUPT_KIND_RETURN);
  value_t inner = interrupt_get_value(v);
  ASSERT_NE(inner, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(inner), 42);
}

TEST_F(it_run_do_while, return_skips_condition) {
  /* Return exits immediately — condition not evaluated */
  _bind("i", create_i32_value(vm, 0));
  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t func_val = _make_func_with_return_type(i32_type);

  value_t prev_func = vm_set_current_func(vm, func_val);
  value_t v = _run_stmt("do { i = i + 1; if(i == 3) { return i; } } while(i < 100);");
  vm_set_current_func(vm, prev_func);

  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_INTERRUPT);
  value_t inner = interrupt_get_value(v);
  ASSERT_NE(inner, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(inner), 3);
}

/* ================================================================== *
 *  Exception propagation                                              *
 * ================================================================== */

TEST_F(it_run_do_while, exception_in_condition_propagates) {
  _bind("x", create_i32_value(vm, 42));
  _bind("i", create_i32_value(vm, 0));
  value_t v = _run_stmt("do { i = i + 1; } while(x.!);");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

TEST_F(it_run_do_while, exception_in_body_propagates) {
  _bind("x", create_i32_value(vm, 42));
  value_t v = _run_stmt("do { x.!; } while(true);");
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ================================================================== *
 *  Nested do-while loops                                              *
 * ================================================================== */

TEST_F(it_run_do_while, nested_do_while_counts_correctly) {
  _bind("count", create_i32_value(vm, 0));
  _bind("i", create_i32_value(vm, 0));
  _bind("j", create_i32_value(vm, 0));
  value_t v = _run_stmt("do { j = 0; do { count = count + 1; j = j + 1; } while(j < 2); i = i + 1; } while(i < 3);");
  ASSERT_NE(v, nullptr);

  value_t count_val = _run_expr("count");
  ASSERT_NE(count_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(count_val), 6);
}

TEST_F(it_run_do_while, nested_break_only_exits_inner) {
  _bind("count", create_i32_value(vm, 0));
  _bind("i", create_i32_value(vm, 0));
  _bind("j", create_i32_value(vm, 0));
  value_t v = _run_stmt("do { j = 0; do { count = count + 1; j = j + 1; if(j == 1) { break; } } while(j < 10); i = i + 1; } while(i < 3);");
  ASSERT_NE(v, nullptr);

  /* Inner runs 1 iteration per outer iteration, outer runs 3 times */
  value_t count_val = _run_expr("count");
  ASSERT_NE(count_val, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(count_val), 3);
}

/* ================================================================== *
 *  Shadow mode                                                        *
 * ================================================================== */

TEST_F(it_run_do_while, shadow_basic) {
  _bind("i", create_i32_value(vm, 0));
  node_t node = _parse_stmt("do { i = i + 1; } while(i < 5);");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_do_while, shadow_break_consumed) {
  _bind("i", create_i32_value(vm, 0));
  node_t node = _parse_stmt("do { break; } while(true);");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_do_while, shadow_continue_consumed) {
  _bind("i", create_i32_value(vm, 0));
  node_t node = _parse_stmt("do { continue; } while(i < 5);");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_do_while, shadow_return_propagates) {
  _bind("i", create_i32_value(vm, 0));
  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t func_val = _make_func_with_return_type(i32_type);

  value_t prev_func = vm_set_current_func(vm, func_val);
  node_t node = _parse_stmt("do { return 42; } while(true);");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  vm_set_current_func(vm, prev_func);

  ASSERT_NE(v, nullptr);
  if (value_is_interrupt(v)) {
    EXPECT_EQ(interrupt_get_kind(v), INTERRUPT_KIND_RETURN);
  }
}

TEST_F(it_run_do_while, shadow_condition_type_error) {
  _bind("s", create_str_value(vm, "hello"));
  node_t node = _parse_stmt("do { } while(s);");
  value_t v = run_statement(vm, node, true);
  free_node(node);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}
