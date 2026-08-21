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
#include "cubec/expression_spread.h"
#include "cubec/declaration_callable.h"
#include "cubec/declaration_function.h"
#include "cubec/function_argument.h"
#include "cubec/statement_return.h"
#include "cubec/statement_block.h"
#include "cubec/generic_param.h"
#include "cubec/token.h"
#include "core/string.h"
#include "core/location.h"
#include "core/vec.h"
#include "core/class.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_run_spread : public CubecTest {
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

  type_t _i32(vm_t vm) { return (type_t)value_get_data(vm_get_i32_type(vm)); }
  type_t _f64(vm_t vm) { return (type_t)value_get_data(vm_get_f64_type(vm)); }
  type_t _bool(vm_t vm) { return (type_t)value_get_data(vm_get_bool_type(vm)); }
  type_t _str(vm_t vm) { return (type_t)value_get_data(vm_get_str_type(vm)); }
  type_t _void(vm_t vm) { return (type_t)value_get_data(vm_get_void_type(vm)); }

  value_t _make_i32_tuple2(vm_t vm, int32_t a, int32_t b) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_init_t vi = {.auto_dispose = false};
    vec_t types = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(types, _i32(vm));
    vec_push(types, _i32(vm));
    value_t tv = vm_create_tuple_type_value(vm, types, true);
    allocator_free(alloc, &types);
    tuple_type_t tt = (tuple_type_t)value_get_data(tv);
    value_t elems[] = {
        vm_create_value(vm, _i32(vm), &a, NULL),
        vm_create_value(vm, _i32(vm), &b, NULL),
    };
    return create_tuple_value(vm, tt, elems);
  }

  value_t _make_i32_array3(vm_t vm, int32_t a, int32_t b, int32_t c) {
    array_type_t at = (array_type_t)value_get_data(
        vm_create_array_type_value(vm, _i32(vm), create_i32_value(vm, 3), true));
    value_t elems[] = {
        vm_create_value(vm, _i32(vm), &a, NULL),
        vm_create_value(vm, _i32(vm), &b, NULL),
        vm_create_value(vm, _i32(vm), &c, NULL),
    };
    return create_array_value(vm, at, elems);
  }
};

/* ==== Callable type expression ==== */

TEST_F(it_run_spread, callable_type_basic) {
  vm_t vm = vm_create(allocator);
  value_t v = _run_expr("func(i32, f64) -> bool");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_TYPE);
  type_t inner = (type_t)value_get_data(v);
  EXPECT_EQ(type_get_kind(inner), TYPE_KIND_CALLABLE);
  vm_dispose(vm, allocator);
}

TEST_F(it_run_spread, callable_type_with_spread) {
  /* func(...T) -> bool where T is bound to tuple(i32, f64)
   * The spread should expand T's tuple elements as individual params. */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  /* Create a pack generic param ...T */
  type_t pack_type = (type_t)value_get_data(vm_get_pack_type(vm));
  vec_init_t evi = {.auto_dispose = true};
  vec_t extends = (vec_t)allocator_create(alloc, &g_vec_class, &evi);
  generic_param_t gp = generic_param_create(alloc, "T", pack_type, extends, true);
  allocator_free(alloc, &extends);

  /* Bind T to a tuple(i32, f64) type value in scope */
  vec_init_t tvi = {.auto_dispose = false};
  vec_t elem_types = (vec_t)allocator_create(alloc, &g_vec_class, &tvi);
  vec_push(elem_types, _i32(vm));
  vec_push(elem_types, _f64(vm));
  tuple_type_t tt = tuple_type_create(alloc, elem_types, true);
  allocator_free(alloc, &elem_types);
  vec_push(vm_get_types(vm), tt);

  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  value_t t_val = vm_create_value_ref(vm, type_type, (type_t)tt, "T");
  _bind("T", t_val);

  /* Evaluate func(...T) -> bool */
  value_t v = _run_expr("func(...T) -> bool");
  ASSERT_FALSE(value_is_abnormal(v)) << "expected callable type, got error";
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_TYPE);
  type_t inner = (type_t)value_get_data(v);
  EXPECT_EQ(type_get_kind(inner), TYPE_KIND_CALLABLE);

  callable_type_t ct = (callable_type_t)inner;
  /* ...T should expand to 2 params: i32, f64 */
  EXPECT_EQ(callable_type_get_param_count(ct), 2u);
  EXPECT_EQ(type_get_kind(callable_type_get_param_type(ct, 0)), TYPE_KIND_I32);
  EXPECT_EQ(type_get_kind(callable_type_get_param_type(ct, 1)), TYPE_KIND_F64);

  allocator_free(alloc, &gp);
  vm_dispose(vm, allocator);
}

/* ==== Spread in function call ==== */

TEST_F(it_run_spread, call_with_tuple_spread) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _loc();

  /* Create a function: func add(a: i32, b: i32) -> i32 { return a + b; } */
  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
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
  vec_t stmts = (vec_t)allocator_create(alloc, &g_vec_class, &bvi);
  vec_push(stmts, ret_stmt);
  node_t body = create_statement_block(vm, loc, stmts);
  node_t name_node = create_literal_identifier(vm, loc, "add");
  node_t func_node = create_declaration_function(vm, loc, name_node, NULL, NULL,
      args, ret_type, body, false, false, false, false, false);

  value_t func_val = run_expression(vm, func_node, false);
  ASSERT_FALSE(value_is_abnormal(func_val));
  _bind("add", func_val);

  /* Create a tuple(i32, i32) = (3, 4) and bind as "tup" */
  value_t tup = _make_i32_tuple2(vm, 3, 4);
  _bind("tup", tup);

  /* Call: add(...tup) → should expand to add(3, 4) → 7 */
  value_t result = _run_expr("add(...tup)");
  ASSERT_FALSE(value_is_abnormal(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(result), 7);

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

TEST_F(it_run_spread, call_with_array_spread) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _loc();

  /* Create a function: func sum(a: i32, b: i32, c: i32) -> i32 */
  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
  vec_push(args, create_function_argument(vm, loc, "a",
      create_literal_identifier(vm, loc, "i32")));
  vec_push(args, create_function_argument(vm, loc, "b",
      create_literal_identifier(vm, loc, "i32")));
  vec_push(args, create_function_argument(vm, loc, "c",
      create_literal_identifier(vm, loc, "i32")));

  node_t ret_type = create_literal_identifier(vm, loc, "i32");
  node_t body_expr = create_expression_binary(
      vm, loc, "+",
      create_expression_binary(vm, loc, "+",
          create_literal_identifier(vm, loc, "a"),
          create_literal_identifier(vm, loc, "b")),
      create_literal_identifier(vm, loc, "c"));
  node_t ret_stmt = create_statement_return(vm, loc, body_expr);
  vec_init_t bvi = {.auto_dispose = true};
  vec_t stmts = (vec_t)allocator_create(alloc, &g_vec_class, &bvi);
  vec_push(stmts, ret_stmt);
  node_t body = create_statement_block(vm, loc, stmts);
  node_t name_node = create_literal_identifier(vm, loc, "sum");
  node_t func_node = create_declaration_function(vm, loc, name_node, NULL, NULL,
      args, ret_type, body, false, false, false, false, false);

  value_t func_val = run_expression(vm, func_node, false);
  ASSERT_FALSE(value_is_abnormal(func_val));
  _bind("sum", func_val);

  /* Create a [3]i32 = [10, 20, 30] and bind as "arr" */
  value_t arr = _make_i32_array3(vm, 10, 20, 30);
  _bind("arr", arr);

  /* Call: sum(...arr) → should expand to sum(10, 20, 30) → 60 */
  value_t result = _run_expr("sum(...arr)");
  ASSERT_FALSE(value_is_abnormal(result));
  EXPECT_EQ(*(int32_t *)value_get_data(result), 60);

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

/* ==== Spread in initialize list ==== */

TEST_F(it_run_spread, init_list_tuple_spread) {
  vm_t vm = vm_create(allocator);

  /* Create a tuple(1, 2) and bind as "t" */
  value_t tup = _make_i32_tuple2(vm, 1, 2);
  _bind("t", tup);

  /* .{0, ...t, 3} should produce tuple(i32, i32, i32, i32) = (0, 1, 2, 3) */
  value_t result = _run_expr(".{0, ...t, 3}");
  ASSERT_FALSE(value_is_abnormal(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_TUPLE);
  tuple_type_t tt = (tuple_type_t)value_get_type(result);
  EXPECT_EQ(tuple_type_get_field_count(tt), 4u);

  vm_dispose(vm, allocator);
}

TEST_F(it_run_spread, init_list_array_spread) {
  vm_t vm = vm_create(allocator);

  /* Create a [3]i32 and bind as "a" */
  value_t arr = _make_i32_array3(vm, 10, 20, 30);
  _bind("a", arr);

  /* .{...a} should produce tuple(i32, i32, i32) */
  value_t result = _run_expr(".{...a}");
  ASSERT_FALSE(value_is_abnormal(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_TUPLE);
  tuple_type_t tt = (tuple_type_t)value_get_type(result);
  EXPECT_EQ(tuple_type_get_field_count(tt), 3u);

  vm_dispose(vm, allocator);
}
