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
#include "engine/pointer_type.h"
#include "engine/callable_type.h"
#include "engine/func.h"
#include "engine/struct_type.h"
#include "engine/slice_type.h"
#include "engine/array_type.h"
#include "cubec/program.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_nil.h"
#include "cubec/literal_identifier.h"
#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_group.h"
#include "cubec/expression_deref.h"
#include "cubec/expression_addr.h"
#include "cubec/expression_member.h"
#include "cubec/expression_call.h"
#include "cubec/expression_subscript.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/token.h"
#include "cubec/node.h"
#include "core/string.h"
#include "core/location.h"
#include "core/vec.h"
#include "core/class.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ------------------------------------------------------------------ *
 *  it_run_expression — end-to-end expression tests via
 *  lexer→parser→run_expression.
 *
 *  Source strings are tokenized with resolve_token_list, parsed with
 *  read_expression, then executed with run_expression.  Variables
 *  (struct, callable, slice) are pre-bound in the vm scope to
 *  exercise member/call/subscript paths that need runtime objects.
 * ------------------------------------------------------------------ */

class it_run_expression : public CubecTest {
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

  /* Register a value in current scope under the given name */
  void _bind(const char *name, value_t val) {
    scope_t scope = vm_get_current_scope(vm);
    name_t n = name_create(scope->allocator, val);
    strmap_insert(scope->names, name, n);
  }

  /* Parse + run source as a program (declarations & statements) */
  value_t _run_source(const char *source, bool shadow = false) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_t tokens = resolve_token_list(vm, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_program_node(vm, tokens, &position, "test.cubec");
    allocator_free(alloc, &tokens);
    if (!node) return NULL;
    value_t v = run_program(vm, node, shadow);
    allocator_free(alloc, &node);
    return v;
  }

  /* Parse a source string into an expression node via lexer→parser */
  node_t _parse_expr(const char *source) {
    vec_t tokens = resolve_token_list(vm, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_expression(vm, tokens, &position, "test.cubec");
    free_tokens(tokens);
    return node;
  }

  /* Parse + run a source string as an expression */
  value_t _run_expr(const char *source, bool shadow = false) {
    node_t node = _parse_expr(source);
    value_t v = run_expression(vm, node, shadow);
    free_node(node);
    return v;
  }
};

/* ==================================================================
 *  run_expression dispatcher
 * ================================================================== */

TEST_F(it_run_expression, null_node_returns_void) {
  value_t v = run_expression(vm, NULL, false);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ==================================================================
 *  Binary arithmetic
 * ================================================================== */

TEST_F(it_run_expression, add_i32) {
  value_t v = _run_expr("10 + 20");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 30);
}

TEST_F(it_run_expression, sub_i32) {
  value_t v = _run_expr("50 - 20");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 30);
}

TEST_F(it_run_expression, mul_i32) {
  value_t v = _run_expr("6 * 7");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
}

TEST_F(it_run_expression, div_i32) {
  value_t v = _run_expr("100 / 4");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 25);
}

TEST_F(it_run_expression, mod_i32) {
  value_t v = _run_expr("17 % 5");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 2);
}

TEST_F(it_run_expression, add_f64) {
  value_t v = _run_expr("1.5 + 2.5");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(v), 4.0);
}

/* ==================================================================
 *  Unary operators
 * ================================================================== */

TEST_F(it_run_expression, unary_neg) {
  value_t v = _run_expr("-5");
  EXPECT_EQ(*(int32_t *)value_get_data(v), -5);
}

TEST_F(it_run_expression, unary_lnot) {
  value_t v = _run_expr("!false");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(v));
}

/* ==================================================================
 *  Comparison operators
 * ================================================================== */

TEST_F(it_run_expression, equal_true) {
  value_t v = _run_expr("10 == 10");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(v));
}

TEST_F(it_run_expression, equal_false) {
  value_t v = _run_expr("10 == 20");
  EXPECT_FALSE(*(bool *)value_get_data(v));
}

TEST_F(it_run_expression, not_equal) {
  value_t v = _run_expr("10 != 20");
  EXPECT_TRUE(*(bool *)value_get_data(v));
}

TEST_F(it_run_expression, less_than) {
  value_t v = _run_expr("5 < 10");
  EXPECT_TRUE(*(bool *)value_get_data(v));
}

TEST_F(it_run_expression, greater_than) {
  value_t v = _run_expr("10 > 5");
  EXPECT_TRUE(*(bool *)value_get_data(v));
}

TEST_F(it_run_expression, less_equal_true) {
  value_t v = _run_expr("5 <= 5");
  EXPECT_TRUE(*(bool *)value_get_data(v));
}

TEST_F(it_run_expression, greater_equal_true) {
  value_t v = _run_expr("10 >= 10");
  EXPECT_TRUE(*(bool *)value_get_data(v));
}

/* ==================================================================
 *  Short-circuit logical operators
 * ================================================================== */

TEST_F(it_run_expression, logical_and_true) {
  value_t v = _run_expr("true && true");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(v));
}

TEST_F(it_run_expression, logical_and_false) {
  value_t v = _run_expr("false && true");
  EXPECT_FALSE(*(bool *)value_get_data(v));
}

TEST_F(it_run_expression, logical_or_true) {
  value_t v = _run_expr("true || false");
  EXPECT_TRUE(*(bool *)value_get_data(v));
}

TEST_F(it_run_expression, logical_or_false) {
  value_t v = _run_expr("false || false");
  EXPECT_FALSE(*(bool *)value_get_data(v));
}

/* ==================================================================
 *  Bitwise operators
 * ================================================================== */

TEST_F(it_run_expression, band_i32) {
  value_t v = _run_expr("12 & 10");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 8);
}

TEST_F(it_run_expression, bor_i32) {
  value_t v = _run_expr("12 | 10");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 14);
}

TEST_F(it_run_expression, bxor_i32) {
  value_t v = _run_expr("12 ^ 10");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 6);
}

TEST_F(it_run_expression, shl_i32) {
  value_t v = _run_expr("1 << 4");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 16);
}

TEST_F(it_run_expression, shr_i32) {
  value_t v = _run_expr("16 >> 4");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 1);
}

TEST_F(it_run_expression, bnot_i32) {
  value_t v = _run_expr("~0");
  EXPECT_EQ(*(int32_t *)value_get_data(v), ~0);
}

/* ==================================================================
 *  Group expression
 * ================================================================== */

TEST_F(it_run_expression, group_passthrough) {
  value_t v = _run_expr("(42)");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
}

TEST_F(it_run_expression, group_wraps_binary) {
  value_t v = _run_expr("(10 + 20)");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 30);
}

TEST_F(it_run_expression, group_precedence) {
  /* (2 + 3) * 4 = 20, not 2 + 12 = 14 */
  value_t v = _run_expr("(2 + 3) * 4");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 20);
}

/* ==================================================================
 *  Nested expressions
 * ================================================================== */

TEST_F(it_run_expression, nested_arithmetic) {
  /* 2 + 3 * 4 = 14 */
  value_t v = _run_expr("2 + 3 * 4");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 14);
}

TEST_F(it_run_expression, chained_arithmetic) {
  /* ((1 + 2) * 3) - 4 = 5 */
  value_t v = _run_expr("(1 + 2) * 3 - 4");
  EXPECT_EQ(*(int32_t *)value_get_data(v), 5);
}

/* ==================================================================
 *  Type mismatch returns exception
 * ================================================================== */

TEST_F(it_run_expression, add_i32_str_returns_exception) {
  value_t v = _run_expr("10 + \"hello\"");
  EXPECT_TRUE(value_is_abnormal(v));
}

/* ==================================================================
 *  Shadow propagation
 * ================================================================== */

TEST_F(it_run_expression, add_shadow) {
  value_t v = _run_expr("10 + 20", true);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_TRUE(value_is_shadow(v));
}

TEST_F(it_run_expression, logical_and_shadow_left) {
  value_t v = _run_expr("true && true", true);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_BOOL);
  EXPECT_TRUE(value_is_shadow(v));
}

/* ==================================================================
 *  Script-mode shadow guard: shadow=false accessing a compile-time
 *  (shadow) value must report an error, not silently propagate shadow.
 * ================================================================== */

TEST_F(it_run_expression, script_guard_rejects_shadow_identifier) {
  /* bind a shadow value (compile-time-only name) into scope, then
   * reference it in script mode (shadow=false). The dispatcher guard
   * must catch this and return an exception. */
  type_t i32t = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t sh = vm_create_value_shadow(vm, i32t, NULL, true);
  _bind("compileonly", sh);

  value_t v = _run_expr("compileonly");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_expression, script_guard_allows_void_result) {
  /* void is a legitimate runtime "no value" (e.g. assignment result),
   * not a compile-time placeholder — the guard must NOT trip on void. */
  value_t i32_val = create_i32_value(vm, 0);
  _bind("g", i32_val);

  value_t v = _run_expr("g = 1");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_FALSE(value_is_abnormal(v));
}

/* ==================================================================
 *  Assignment expression
 * ================================================================== */

TEST_F(it_run_expression, assign_identifier_returns_void) {
  value_t i32_val = create_i32_value(vm, 0);
  _bind("x", i32_val);

  value_t v = _run_expr("x = 99");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "x");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 99);
}

TEST_F(it_run_expression, assign_compound_add) {
  value_t i32_val = create_i32_value(vm, 10);
  _bind("y", i32_val);

  value_t v = _run_expr("y += 5");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "y");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 15);
}

TEST_F(it_run_expression, assign_discard_wildcard) {
  value_t v = _run_expr("_ = 42");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ==================================================================
 *  Dereference expression
 * ================================================================== */

TEST_F(it_run_expression, deref_pointer) {
  value_t i32_val = create_i32_value(vm, 77);
  value_t ptr_val = value_addrof(vm, i32_val);
  ASSERT_NE(ptr_val, nullptr);
  _bind("p", ptr_val);

  value_t v = _run_expr("p.*");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 77);
}

/* ==================================================================
 *  Address-of expression
 * ================================================================== */

TEST_F(it_run_expression, addr_of_value) {
  value_t i32_val = create_i32_value(vm, 55);
  _bind("a", i32_val);

  value_t v = _run_expr("a.&");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_POINTER);
}

/* ==================================================================
 *  Member expression — struct field access
 * ================================================================== */

TEST_F(it_run_expression, member_get_struct_field) {
  /* Declare struct Point and create a value via source */
  _run_source("struct Point { pub x: i32; pub y: i32; }; var pt = .Point{.x = 10, .y = 20};");

  /* Access pt.x */
  value_t v = _run_expr("pt.x");
  ASSERT_FALSE(value_is_abnormal(v)) << "member access on struct failed";
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 10);
}

TEST_F(it_run_expression, member_on_non_struct_returns_error) {
  value_t i32_val = create_i32_value(vm, 123);
  _bind("simple", i32_val);

  value_t v = _run_expr("simple.field");
  EXPECT_TRUE(value_is_abnormal(v));
}

/* ==================================================================
 *  Subscript expression — array/slice index
 * ================================================================== */

TEST_F(it_run_expression, subscript_array_element) {
  /* Create an array [3]i32 { 10, 20, 30 } */
  type_t i32_t = (type_t)value_get_data(vm_get_i32_type(vm));
  array_type_t at = array_type_create(vm, i32_t, create_i32_value(vm, 3), true);
  vec_push(vm_get_types(vm), at);

  value_t e0 = create_i32_value(vm, 10);
  value_t e1 = create_i32_value(vm, 20);
  value_t e2 = create_i32_value(vm, 30);
  value_t elems[] = {e0, e1, e2};
  value_t av = create_array_value(vm, at, elems);
  _bind("arr", av);

  /* arr[1] should be 20 */
  value_t v = _run_expr("arr[1]");
  ASSERT_FALSE(value_is_abnormal(v)) << "subscript on array failed";
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 20);
}

TEST_F(it_run_expression, subscript_on_non_indexable_returns_error) {
  value_t i32_val = create_i32_value(vm, 0);
  _bind("num", i32_val);

  value_t v = _run_expr("num[0]");
  EXPECT_TRUE(value_is_abnormal(v));
}

/* ==================================================================
 *  Call expression — panic builtin
 * ================================================================== */

TEST_F(it_run_expression, call_panic_returns_exception) {
  value_t panic_fn = vm_get_builtin(vm, "panic");
  ASSERT_NE(panic_fn, nullptr);
  /* panic is already registered in global scope by vm bootstrap */

  value_t v = _run_expr("panic(\"oops\")");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ==================================================================
 *  Namespace access expression (::) on non-type
 * ================================================================== */

TEST_F(it_run_expression, namespace_access_on_non_type_returns_error) {
  value_t i32_val = create_i32_value(vm, 1);
  _bind("nottype", i32_val);

  value_t v = _run_expr("nottype::prop");
  EXPECT_TRUE(value_is_abnormal(v));
}

/* ==================================================================
 *  Assignment with safe_cast — type coercion on write
 * ================================================================== */

TEST_F(it_run_expression, assign_i32_to_i8_safe_cast_rejected) {
  /* x: i8 = 42 — i32→i8 is narrowing, safe_cast rejects */
  value_t i8_val = create_i8_value(vm, 0);
  _bind("x", i8_val);

  value_t v = _run_expr("x = 42");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_expression, assign_i64_to_i32_narrowing_rejected) {
  /* y: i32 = big_i64 — i64→i32 is narrowing, safe_cast rejects */
  value_t i32_val = create_i32_value(vm, 0);
  _bind("y", i32_val);

  value_t big_val = create_i64_value(vm, 0x1FFFFFFFF);
  _bind("big", big_val);

  value_t v = _run_expr("y = big");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_expression, assign_i8_to_i32_safe_cast_ok) {
  /* z: i32 = small_i8 — i8→i32 is widening, safe_cast allows */
  value_t i32_val = create_i32_value(vm, 0);
  _bind("z", i32_val);

  value_t small_val = create_i8_value(vm, 42);
  _bind("small", small_val);

  value_t v = _run_expr("z = small");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "z");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 42);
}

TEST_F(it_run_expression, assign_i32_to_u32_same_width_safe_cast_ok) {
  /* z: u32 = neg_i32 — same width, safe_cast allows */
  value_t u32_val = create_u32_value(vm, 0);
  _bind("z", u32_val);

  value_t neg_val = create_i32_value(vm, -1);
  _bind("neg", neg_val);

  value_t v = _run_expr("z = neg");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "z");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(*(uint32_t *)value_get_data(n->ref), 0xFFFFFFFFu);
}

TEST_F(it_run_expression, assign_incompatible_type_returns_error) {
  /* i32 = bool — safe_cast fails, returns exception */
  value_t i32_val = create_i32_value(vm, 0);
  _bind("x", i32_val);

  value_t v = _run_expr("x = true");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_expression, assign_incompatible_type_shadow_skips_write) {
  /* shadow mode: x = true → shadow path evaluates rvalue only,
   * does not attempt safe_cast/write (type mismatch not detected at
   * expression level in shadow mode; statement layer handles it) */
  value_t i32_val = create_i32_value(vm, 0);
  _bind("x", i32_val);

  value_t v = _run_expr("x = true", true);
  /* Shadow path for = returns the rvalue's shadow result (bool shadow) */
  EXPECT_TRUE(value_is_shadow(v));
}

/* ==================================================================
 *  Type negotiation — different-width integer arithmetic
 * ================================================================== */

TEST_F(it_run_expression, add_i8_i32_promotes_to_i32) {
  /* i8 + i32 → i32 (wider type wins) */
  value_t a8 = create_i8_value(vm, 10);
  _bind("a", a8);

  value_t v = _run_expr("a + 20");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 30);
}

TEST_F(it_run_expression, add_i32_i64_promotes_to_i64) {
  /* i32 + i64 → i64 */
  value_t a32 = create_i32_value(vm, 100);
  _bind("a", a32);
  value_t b64 = create_i64_value(vm, 200);
  _bind("b", b64);

  value_t v = _run_expr("a + b");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I64);
  EXPECT_EQ(*(int64_t *)value_get_data(v), 300);
}

TEST_F(it_run_expression, add_i32_u32_promotes_to_u32) {
  /* i32 + u32 → u32 (same size, unsigned wins) */
  value_t a32 = create_i32_value(vm, 10);
  _bind("a", a32);
  value_t b32 = create_u32_value(vm, 20);
  _bind("b", b32);

  value_t v = _run_expr("a + b");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U32);
  EXPECT_EQ(*(uint32_t *)value_get_data(v), 30u);
}

TEST_F(it_run_expression, add_i8_u64_promotes_to_u64) {
  /* i8 + u64 → u64 (wider wins) */
  value_t a8 = create_i8_value(vm, 5);
  _bind("a", a8);
  value_t b64 = create_u64_value(vm, 100);
  _bind("b", b64);

  value_t v = _run_expr("a + b");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U64);
  EXPECT_EQ(*(uint64_t *)value_get_data(v), 105ull);
}

TEST_F(it_run_expression, sub_u8_i32_promotes_to_i32) {
  /* u8 - i32 → i32 (wider signed wins) */
  value_t a8 = create_u8_value(vm, 50);
  _bind("a", a8);

  value_t v = _run_expr("a - 20");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 30);
}

TEST_F(it_run_expression, mul_i16_u16_promotes_to_u16) {
  /* i16 * u16 → u16 (same size, unsigned wins) */
  value_t a16 = create_i16_value(vm, 3);
  _bind("a", a16);
  value_t b16 = create_u16_value(vm, 7);
  _bind("b", b16);

  value_t v = _run_expr("a * b");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U16);
  EXPECT_EQ(*(uint16_t *)value_get_data(v), 21);
}

TEST_F(it_run_expression, type_negotiation_shadow_propagates) {
  /* shadow: i8 + i32 → shadow i32 */
  value_t a8 = create_i8_value(vm, 10);
  _bind("a", a8);

  value_t v = _run_expr("a + 20", true);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_TRUE(value_is_shadow(v));
}

/* ==================================================================
 *  Typed literal suffixes — explicit-width integer values
 * ================================================================== */

TEST_F(it_run_expression, literal_suffix_i8) {
  value_t v = _run_expr("42i8");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I8);
  EXPECT_EQ(*(int8_t *)value_get_data(v), 42);
}

TEST_F(it_run_expression, literal_suffix_i64) {
  value_t v = _run_expr("1000i64");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I64);
  EXPECT_EQ(*(int64_t *)value_get_data(v), 1000);
}

TEST_F(it_run_expression, literal_suffix_u32) {
  value_t v = _run_expr("100u32");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U32);
  EXPECT_EQ(*(uint32_t *)value_get_data(v), 100u);
}

TEST_F(it_run_expression, literal_suffix_f64) {
  value_t v = _run_expr("1.5f64");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(v), 1.5);
}

TEST_F(it_run_expression, same_type_arithmetic_with_suffix) {
  /* 10i8 + 20i8 → i8 result (no widening) */
  value_t v = _run_expr("10i8 + 20i8");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I8);
  EXPECT_EQ(*(int8_t *)value_get_data(v), 30);
}

/* ---- typeof / sizeof / alignof ---- */

TEST_F(it_run_expression, typeof_i32_literal) {
  value_t v = _run_expr("typeof(42)");
  ASSERT_FALSE(value_is_abnormal(v));
  /* typeof returns a type value; its data is the type_t */
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_TYPE);
  type_t inner = (type_t)value_get_data(v);
  EXPECT_EQ(type_get_kind(inner), TYPE_KIND_I32);
}

TEST_F(it_run_expression, typeof_arithmetic) {
  value_t v = _run_expr("typeof(1 + 2)");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_TYPE);
  type_t inner = (type_t)value_get_data(v);
  EXPECT_EQ(type_get_kind(inner), TYPE_KIND_I32);
}

TEST_F(it_run_expression, sizeof_i32) {
  value_t v = _run_expr("sizeof(42)");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U64);
  EXPECT_EQ(*(uint64_t *)value_get_data(v), 4u);
}

TEST_F(it_run_expression, sizeof_bool) {
  value_t v = _run_expr("sizeof(true)");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U64);
  EXPECT_EQ(*(uint64_t *)value_get_data(v), 1u);
}

TEST_F(it_run_expression, alignof_i32) {
  value_t v = _run_expr("alignof(42)");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U64);
  EXPECT_EQ(*(uint64_t *)value_get_data(v), 4u);
}

TEST_F(it_run_expression, sizeof_with_var) {
  _run_source("var x: i64 = 100;");
  value_t v = _run_expr("sizeof(x)");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U64);
  EXPECT_EQ(*(uint64_t *)value_get_data(v), 8u);
}

TEST_F(it_run_expression, typeof_with_var) {
  _run_source("var x: f64 = 3.14;");
  value_t v = _run_expr("typeof(x)");
  ASSERT_FALSE(value_is_abnormal(v));
  type_t inner = (type_t)value_get_data(v);
  EXPECT_EQ(type_get_kind(inner), TYPE_KIND_F64);
}

TEST_F(it_run_expression, sizeof_never_shadow) {
  /* sizeof always returns concrete u64, even in shadow context */
  value_t v = _run_expr("sizeof(42)", /*shadow=*/true);
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_FALSE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U64);
  EXPECT_EQ(*(uint64_t *)value_get_data(v), 4u);
}

TEST_F(it_run_expression, typeof_never_shadow) {
  /* typeof always returns concrete type value, even in shadow context */
  value_t v = _run_expr("typeof(42)", /*shadow=*/true);
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_FALSE(value_is_shadow(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_TYPE);
  type_t inner = (type_t)value_get_data(v);
  EXPECT_EQ(type_get_kind(inner), TYPE_KIND_I32);
}

/* ---- typeof/sizeof/alignof with type expressions ---- */

TEST_F(it_run_expression, typeof_type_expr) {
  /* typeof(i32) — i32 is a type expression, should unwrap to i32 */
  value_t v = _run_expr("typeof(i32)");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_TYPE);
  type_t inner = (type_t)value_get_data(v);
  EXPECT_EQ(type_get_kind(inner), TYPE_KIND_I32);
}

TEST_F(it_run_expression, sizeof_type_expr) {
  /* sizeof(i64) — type expression directly */
  value_t v = _run_expr("sizeof(i64)");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U64);
  EXPECT_EQ(*(uint64_t *)value_get_data(v), 8u);
}

TEST_F(it_run_expression, alignof_type_expr) {
  /* alignof(bool) — type expression directly */
  value_t v = _run_expr("alignof(bool)");
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_U64);
  EXPECT_EQ(*(uint64_t *)value_get_data(v), 1u);
}

/* ==================================================================
 *  Ternary expression
 * ================================================================== */

TEST_F(it_run_expression, ternary_true_branch) {
  value_t v = _run_expr("true ? 1 : 2");
  ASSERT_FALSE(value_is_abnormal(v));
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 1);
}

TEST_F(it_run_expression, ternary_false_branch) {
  value_t v = _run_expr("false ? 1 : 2");
  ASSERT_FALSE(value_is_abnormal(v));
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 2);
}

TEST_F(it_run_expression, ternary_with_comparison) {
  value_t v = _run_expr("3 > 2 ? 10 : 20");
  ASSERT_FALSE(value_is_abnormal(v));
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 10);
}

TEST_F(it_run_expression, ternary_non_bool_condition_error) {
  /* condition is i32, not convertible to bool */
  value_t v = _run_expr("1 ? 2 : 3");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_expression, ternary_type_mismatch_error) {
  /* consequent is i32, alternate is str — different types */
  value_t v = _run_expr("true ? 1 : \"hello\"");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_expression, ternary_same_type_different_values) {
  value_t v = _run_expr("false ? 100 : 200");
  ASSERT_FALSE(value_is_abnormal(v));
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 200);
}

TEST_F(it_run_expression, ternary_shadow_mode) {
  /* shadow mode: both branches evaluated for type checking */
  value_t v = _run_expr("true ? 1 : 2", true);
  ASSERT_FALSE(value_is_abnormal(v));
  EXPECT_TRUE(value_is_shadow(v));
  ASSERT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
}

/* ==================================================================
 *  Slice expression — host[start:length]
 * ================================================================== */

TEST_F(it_run_expression, slice_array_with_start_and_length) {
  /* Create array [3]i32 { 10, 20, 30 } */
  type_t i32_t = (type_t)value_get_data(vm_get_i32_type(vm));
  array_type_t at = array_type_create(vm, i32_t, create_i32_value(vm, 3), true);
  vec_push(vm_get_types(vm), at);

  value_t e0 = create_i32_value(vm, 10);
  value_t e1 = create_i32_value(vm, 20);
  value_t e2 = create_i32_value(vm, 30);
  value_t elems[] = {e0, e1, e2};
  value_t av = create_array_value(vm, at, elems);
  _bind("arr", av);

  /* arr[1:2] should be a slice of [20, 30] */
  value_t v = _run_expr("arr[1:2]");
  ASSERT_FALSE(value_is_abnormal(v)) << "slice on array failed";
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_SLICE);

  /* Verify slice contents via subscript */
  struct slice_data_t *sd = (struct slice_data_t *)value_get_data(v);
  EXPECT_EQ(sd->len, 2u);

  value_t idx0 = create_i32_value(vm, 0);
  value_t elem0 = value_get_item(vm, v, idx0);
  ASSERT_FALSE(value_is_abnormal(elem0));
  EXPECT_EQ(*(int32_t *)value_get_data(elem0), 20);

  value_t idx1 = create_i32_value(vm, 1);
  value_t elem1 = value_get_item(vm, v, idx1);
  ASSERT_FALSE(value_is_abnormal(elem1));
  EXPECT_EQ(*(int32_t *)value_get_data(elem1), 30);
}

TEST_F(it_run_expression, slice_array_omit_length) {
  /* arr[1:] — length derived from array count minus start */
  type_t i32_t = (type_t)value_get_data(vm_get_i32_type(vm));
  array_type_t at = array_type_create(vm, i32_t, create_i32_value(vm, 4), true);
  vec_push(vm_get_types(vm), at);

  value_t elems[] = {
      create_i32_value(vm, 10), create_i32_value(vm, 20),
      create_i32_value(vm, 30), create_i32_value(vm, 40)};
  value_t av = create_array_value(vm, at, elems);
  _bind("arr", av);

  value_t v = _run_expr("arr[2:]");
  ASSERT_FALSE(value_is_abnormal(v));
  struct slice_data_t *sd = (struct slice_data_t *)value_get_data(v);
  EXPECT_EQ(sd->len, 2u);
}

TEST_F(it_run_expression, slice_non_slicable_type_error) {
  value_t i32_val = create_i32_value(vm, 42);
  _bind("num", i32_val);

  value_t v = _run_expr("num[0:1]");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_expression, slice_start_not_integer_error) {
  type_t i32_t = (type_t)value_get_data(vm_get_i32_type(vm));
  array_type_t at = array_type_create(vm, i32_t, create_i32_value(vm, 2), true);
  vec_push(vm_get_types(vm), at);

  value_t elems[] = {create_i32_value(vm, 1), create_i32_value(vm, 2)};
  value_t av = create_array_value(vm, at, elems);
  _bind("arr", av);

  /* "hello" as start — not an integer */
  value_t v = _run_expr("arr[\"hello\":1]");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_expression, slice_negative_start_error) {
  type_t i32_t = (type_t)value_get_data(vm_get_i32_type(vm));
  array_type_t at = array_type_create(vm, i32_t, create_i32_value(vm, 3), true);
  vec_push(vm_get_types(vm), at);

  value_t elems[] = {
      create_i32_value(vm, 10), create_i32_value(vm, 20),
      create_i32_value(vm, 30)};
  value_t av = create_array_value(vm, at, elems);
  _bind("arr", av);
  _bind("neg", create_i32_value(vm, -1));

  /* Negative start index should be rejected */
  value_t v = _run_expr("arr[neg:1]");
  EXPECT_TRUE(value_is_abnormal(v));
}

TEST_F(it_run_expression, slice_negative_length_error) {
  type_t i32_t = (type_t)value_get_data(vm_get_i32_type(vm));
  array_type_t at = array_type_create(vm, i32_t, create_i32_value(vm, 3), true);
  vec_push(vm_get_types(vm), at);

  value_t elems[] = {
      create_i32_value(vm, 10), create_i32_value(vm, 20),
      create_i32_value(vm, 30)};
  value_t av = create_array_value(vm, at, elems);
  _bind("arr", av);
  _bind("neg", create_i32_value(vm, -2));

  /* Negative length should be rejected */
  value_t v = _run_expr("arr[0:neg]");
  EXPECT_TRUE(value_is_abnormal(v));
}
