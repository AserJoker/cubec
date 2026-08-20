#include "run/run.h"
#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/str_type.h"
#include "engine/nil_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/callable_type.h"
#include "engine/struct_type.h"
#include "engine/tuple_type.h"
#include "engine/array_type.h"
#include "engine/pointer_type.h"
#include "engine/func.h"
#include "engine/ast_func.h"
#include "engine/generic_fn_type.h"
#include "engine/generic_param.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_nil.h"
#include "cubec/literal_identifier.h"
#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_call.h"
#include "cubec/declaration_callable.h"
#include "cubec/declaration_function.h"
#include "cubec/function_argument.h"
#include "cubec/function_capture.h"
#include "cubec/statement_return.h"
#include "cubec/statement_block.h"
#include "cubec/generic_param.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "core/string.h"
#include "core/location.h"
#include "core/vec.h"
#include "core/class.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <string>

using ::testing::Test;

class it_run_closure : public CubecTest {
protected:
  location_t _loc() {
    location_t loc;
    memset(&loc, 0, sizeof(loc));
    loc.filename = "test";
    return loc;
  }

  void free_node(node_t &node) {
    if (node) allocator_free(allocator, &node);
  }

  void free_tokens(vec_t &tokens) {
    if (tokens) allocator_free(allocator, &tokens);
  }

  void _bind(const char *name, value_t val) {
    scope_t scope = vm_get_current_scope(vm);
    name_t n = name_create(scope->allocator, val);
    strmap_insert(scope->names, name, n);
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

  node_t _parse_program(const char *source) {
    vec_t tokens = resolve_token_list(vm, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_program_node(vm, tokens, &position, "test.cubec");
    free_tokens(tokens);
    return node;
  }

  value_t _run_source(const char *source, bool shadow = false) {
    node_t node = _parse_program(source);
    value_t v = run_program(vm, node, shadow);
    free_node(node);
    return v;
  }

  type_t _i32() { return (type_t)value_get_data(vm_get_i32_type(vm)); }
  type_t _bool() { return (type_t)value_get_data(vm_get_bool_type(vm)); }

  std::string _error_msg(value_t v) {
    if (type_get_kind(value_get_type(v)) == TYPE_KIND_EXCEPTION) {
      const char *msg = (const char *)value_get_data(v);
      return std::string(" msg=") + (msg ? msg : "<null>");
    }
    return "";
  }
};

/* ==== Basic closure capture (AST-level) ==== */

TEST_F(it_run_closure, capture_single_variable) {
  location_t loc = _loc();

  int32_t x_val = 10;
  value_t x = vm_create_value(vm, _i32(), &x_val, NULL);
  _bind("x", x);

  vec_init_t cvi = {.auto_dispose = true};
  vec_t captures = (vec_t)allocator_create(allocator, &g_vec_class, &cvi);
  vec_push(captures, create_function_capture(vm, loc, "x"));

  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(allocator, &g_vec_class, &avi);
  vec_push(args, create_function_argument(vm, loc, "a",
      create_literal_identifier(vm, loc, "i32")));

  node_t ret_type = create_literal_identifier(vm, loc, "i32");
  node_t body_expr = create_expression_binary(
      vm, loc, "+",
      create_literal_identifier(vm, loc, "a"),
      create_literal_identifier(vm, loc, "x"));
  node_t ret_stmt = create_statement_return(vm, loc, body_expr);
  vec_init_t bvi = {.auto_dispose = true};
  vec_t stmts = (vec_t)allocator_create(allocator, &g_vec_class, &bvi);
  vec_push(stmts, ret_stmt);
  node_t body = create_statement_block(vm, loc, stmts);

  node_t func_node = create_declaration_function(vm, loc, NULL,
      captures, NULL, args, ret_type, body,
      false, false, false, false, false);

  value_t func_val = run_expression(vm, func_node, false);
  ASSERT_FALSE(value_is_abnormal(func_val)) << "function creation failed";
  _bind("adder", func_val);

  value_t result = _run_expr("adder(5)");
  ASSERT_FALSE(value_is_abnormal(result)) << "function call failed";
  EXPECT_EQ(*(int32_t *)value_get_data(result), 15);

  allocator_free(allocator, &func_node);
}

TEST_F(it_run_closure, capture_multiple_variables) {
  location_t loc = _loc();

  int32_t a_val = 100, b_val = 200;
  value_t av = vm_create_value(vm, _i32(), &a_val, NULL);
  value_t bv = vm_create_value(vm, _i32(), &b_val, NULL);
  _bind("a", av);
  _bind("b", bv);

  vec_init_t cvi = {.auto_dispose = true};
  vec_t captures = (vec_t)allocator_create(allocator, &g_vec_class, &cvi);
  vec_push(captures, create_function_capture(vm, loc, "a"));
  vec_push(captures, create_function_capture(vm, loc, "b"));

  vec_init_t pvi = {.auto_dispose = true};
  vec_t params = (vec_t)allocator_create(allocator, &g_vec_class, &pvi);
  vec_push(params, create_function_argument(vm, loc, "x",
      create_literal_identifier(vm, loc, "i32")));

  node_t ret_type = create_literal_identifier(vm, loc, "i32");
  node_t body_expr = create_expression_binary(
      vm, loc, "+",
      create_expression_binary(vm, loc, "+",
          create_literal_identifier(vm, loc, "x"),
          create_literal_identifier(vm, loc, "a")),
      create_literal_identifier(vm, loc, "b"));
  node_t ret_stmt = create_statement_return(vm, loc, body_expr);
  vec_init_t bvi = {.auto_dispose = true};
  vec_t stmts = (vec_t)allocator_create(allocator, &g_vec_class, &bvi);
  vec_push(stmts, ret_stmt);
  node_t body = create_statement_block(vm, loc, stmts);

  node_t func_node = create_declaration_function(vm, loc, NULL,
      captures, NULL, params, ret_type, body,
      false, false, false, false, false);

  value_t func_val = run_expression(vm, func_node, false);
  ASSERT_FALSE(value_is_abnormal(func_val));
  _bind("sum3", func_val);

  value_t result = _run_expr("sum3(1)");
  ASSERT_FALSE(value_is_abnormal(result));
  EXPECT_EQ(*(int32_t *)value_get_data(result), 301);

  allocator_free(allocator, &func_node);
}

TEST_F(it_run_closure, capture_not_found_returns_error) {
  location_t loc = _loc();

  vec_init_t cvi = {.auto_dispose = true};
  vec_t captures = (vec_t)allocator_create(allocator, &g_vec_class, &cvi);
  vec_push(captures, create_function_capture(vm, loc, "z"));

  vec_init_t pvi = {.auto_dispose = true};
  vec_t params = (vec_t)allocator_create(allocator, &g_vec_class, &pvi);

  node_t ret_type = create_literal_identifier(vm, loc, "i32");
  node_t body_expr = create_literal_identifier(vm, loc, "z");
  node_t ret_stmt = create_statement_return(vm, loc, body_expr);
  vec_init_t bvi = {.auto_dispose = true};
  vec_t stmts = (vec_t)allocator_create(allocator, &g_vec_class, &bvi);
  vec_push(stmts, ret_stmt);
  node_t body = create_statement_block(vm, loc, stmts);

  node_t func_node = create_declaration_function(vm, loc, NULL,
      captures, NULL, params, ret_type, body,
      false, false, false, false, false);

  value_t func_val = run_expression(vm, func_node, false);
  EXPECT_TRUE(value_is_abnormal(func_val))
      << "expected error for uncapturable variable";

  allocator_free(allocator, &func_node);
}

TEST_F(it_run_closure, no_captures_still_works) {
  location_t loc = _loc();

  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(allocator, &g_vec_class, &avi);
  vec_push(args, create_function_argument(vm, loc, "a",
      create_literal_identifier(vm, loc, "i32")));
  vec_push(args, create_function_argument(vm, loc, "b",
      create_literal_identifier(vm, loc, "i32")));

  node_t ret_type = create_literal_identifier(vm, loc, "i32");
  node_t body_expr = create_expression_binary(
      vm, loc, "+",
      create_literal_identifier(vm, loc, "a"),
      create_literal_identifier(vm, loc, "b"));
  node_t ret_stmt = create_statement_return(vm, loc, body_expr);
  vec_init_t bvi = {.auto_dispose = true};
  vec_t stmts = (vec_t)allocator_create(allocator, &g_vec_class, &bvi);
  vec_push(stmts, ret_stmt);
  node_t body = create_statement_block(vm, loc, stmts);

  node_t func_node = create_declaration_function(vm, loc, NULL,
      NULL, NULL, args, ret_type, body,
      false, false, false, false, false);

  value_t func_val = run_expression(vm, func_node, false);
  ASSERT_FALSE(value_is_abnormal(func_val));
  _bind("add", func_val);

  value_t result = _run_expr("add(3, 4)");
  ASSERT_FALSE(value_is_abnormal(result));
  EXPECT_EQ(*(int32_t *)value_get_data(result), 7);

  allocator_free(allocator, &func_node);
}

TEST_F(it_run_closure, closure_scope_isolation) {
  location_t loc = _loc();

  int32_t x_val = 42;
  value_t x = vm_create_value(vm, _i32(), &x_val, NULL);
  _bind("x", x);

  vec_init_t cvi = {.auto_dispose = true};
  vec_t captures = (vec_t)allocator_create(allocator, &g_vec_class, &cvi);
  vec_push(captures, create_function_capture(vm, loc, "x"));

  vec_init_t pvi = {.auto_dispose = true};
  vec_t params = (vec_t)allocator_create(allocator, &g_vec_class, &pvi);

  node_t ret_type = create_literal_identifier(vm, loc, "i32");
  node_t body_expr = create_literal_identifier(vm, loc, "x");
  node_t ret_stmt = create_statement_return(vm, loc, body_expr);
  vec_init_t bvi = {.auto_dispose = true};
  vec_t stmts = (vec_t)allocator_create(allocator, &g_vec_class, &bvi);
  vec_push(stmts, ret_stmt);
  node_t body = create_statement_block(vm, loc, stmts);

  node_t func_node = create_declaration_function(vm, loc, NULL,
      captures, NULL, params, ret_type, body,
      false, false, false, false, false);

  value_t func_val = run_expression(vm, func_node, false);
  ASSERT_FALSE(value_is_abnormal(func_val));
  _bind("get_x", func_val);

  value_t result = _run_expr("get_x()");
  ASSERT_FALSE(value_is_abnormal(result));
  EXPECT_EQ(*(int32_t *)value_get_data(result), 42);

  allocator_free(allocator, &func_node);
}

/* ==== Decorator / function-factory pattern (end-to-end via parser) ==== */

TEST_F(it_run_closure, decorator_make_adder) {
  /* func make_adder(x: i32): func(i32) -> i32 {
   *   return func|x|(a: i32) -> i32 { return a + x; };
   * }
   * var add5 = make_adder(5);
   * var result = add5(10); → 15
   */
  value_t v = _run_source(
      "func make_adder(x: i32): func(i32) -> i32 {\n"
      "  return func|x|(a: i32) -> i32 { return a + x; };\n"
      "}\n"
      "var add5 = make_adder(5);\n"
      "var result = add5(10);\n");
  ASSERT_FALSE(value_is_abnormal(v)) << _error_msg(v);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "result");
  ASSERT_TRUE(n && n->ref);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 15);
}

TEST_F(it_run_closure, decorator_make_adder_different_args) {
  value_t v = _run_source(
      "func make_adder(x: i32): func(i32) -> i32 {\n"
      "  return func|x|(a: i32) -> i32 { return a + x; };\n"
      "}\n"
      "var add10 = make_adder(10);\n"
      "var add20 = make_adder(20);\n"
      "var r1 = add10(7);\n"
      "var r2 = add20(3);\n");
  ASSERT_FALSE(value_is_abnormal(v)) << _error_msg(v);

  scope_t scope = vm_get_current_scope(vm);
  name_t n1 = scope_lookup(scope, "r1");
  name_t n2 = scope_lookup(scope, "r2");
  ASSERT_TRUE(n1 && n1->ref);
  ASSERT_TRUE(n2 && n2->ref);
  EXPECT_EQ(*(int32_t *)value_get_data(n1->ref), 17);
  EXPECT_EQ(*(int32_t *)value_get_data(n2->ref), 23);
}

TEST_F(it_run_closure, decorator_capture_multiple) {
  value_t v = _run_source(
      "func make_multiplier(factor: i32, offset: i32): func(i32) -> i32 {\n"
      "  return func|factor, offset|(x: i32) -> i32 { return x * factor + offset; };\n"
      "}\n"
      "var f = make_multiplier(3, 1);\n"
      "var result = f(5);\n");
  ASSERT_FALSE(value_is_abnormal(v)) << _error_msg(v);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "result");
  ASSERT_TRUE(n && n->ref);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 16);
}

TEST_F(it_run_closure, anonymous_func_with_capture_as_expression) {
  value_t v = _run_source(
      "var x = 100;\n"
      "var closure = func|x|(a: i32): i32 { return a + x; };\n"
      "var result = closure(7);\n");
  ASSERT_FALSE(value_is_abnormal(v)) << _error_msg(v);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "result");
  ASSERT_TRUE(n && n->ref);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 107);
}

TEST_F(it_run_closure, nested_closure_capture) {
  /* func make_adder_and_multiplier(x: i32): func(i32) -> func(i32) -> i32 {
   *   return func|x|(m: i32) -> func(i32) -> i32 {
   *     return func|x, m|(a: i32) -> i32 { return a * m + x; };
   *   };
   * }
   * var f = make_adder_and_multiplier(5);
   * var g = f(3);       // g(a) = a * 3 + 5
   * var result = g(10); → 35
   */
  value_t v = _run_source(
      "func make_adder_and_multiplier(x: i32): func(i32) -> func(i32) -> i32 {\n"
      "  return func|x|(m: i32) -> func(i32) -> i32 {\n"
      "    return func|x, m|(a: i32) -> i32 { return a * m + x; };\n"
      "  };\n"
      "}\n"
      "var f = make_adder_and_multiplier(5);\n"
      "var g = f(3);\n"
      "var result = g(10);\n");
  ASSERT_FALSE(value_is_abnormal(v)) << _error_msg(v);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "result");
  ASSERT_TRUE(n && n->ref);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 35);
}
