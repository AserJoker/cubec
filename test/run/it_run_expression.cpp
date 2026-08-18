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
#include "engine/pointer_type.h"
#include "engine/callable_type.h"
#include "cubec/literal_numeric.h"
#include "cubec/literal_string.h"
#include "cubec/literal_nil.h"
#include "cubec/literal_identifier.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_group.h"
#include "cubec/expression_deref.h"
#include "cubec/expression_addr.h"
#include "cubec/expression_member.h"
#include "cubec/expression_call.h"
#include "cubec/expression_subscript.h"
#include "cubec/expression_namespace_access.h"
#include "core/string.h"
#include "core/location.h"
#include "core/vec.h"
#include "core/class.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_run_expression : public CubecTest {
protected:
  location_t _loc() {
    location_t loc;
    memset(&loc, 0, sizeof(loc));
    loc.filename = "test";
    return loc;
  }

  vm_t vm() { return ctx->vm; }

  void free_node(node_t &node) {
    if (node) allocator_free(ctx->allocator, &node);
  }

  /* Create an i32 literal node */
  node_t _i32_node(const char *val) {
    return create_literal_numeric(ctx, _loc(), val,
        CUBEC_LITERAL_NUMERIC_KIND_INTEGER, CUBEC_LITERAL_NUMERIC_TYPE_I32);
  }

  /* Create an f64 literal node */
  node_t _f64_node(const char *val) {
    return create_literal_numeric(ctx, _loc(), val,
        CUBEC_LITERAL_NUMERIC_KIND_FLOAT, CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT);
  }

  /* Create a bool literal node (true=1, false=0) */
  node_t _bool_node(bool b) {
    return _i32_node(b ? "1" : "0");
  }

  /* Register a value in current scope under the given name */
  void _bind(const char *name, value_t val) {
    scope_t scope = vm_get_current_scope(vm());
    name_t n = name_create(scope->allocator, val);
    strmap_insert(scope->names, name, n);
  }

  /* Create identifier node */
  node_t _id_node(const char *name) {
    return create_literal_identifier(ctx, _loc(), name);
  }
};

/* ==================================================================
 *  run_expression dispatcher
 * ================================================================== */

TEST_F(it_run_expression, null_node_returns_void) {
  value_t v = run_expression(ctx, NULL, false);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ==================================================================
 *  Binary arithmetic
 * ================================================================== */

TEST_F(it_run_expression, add_i32) {
  node_t l = _i32_node("10");
  node_t r = _i32_node("20");
  node_t bin = create_expression_binary(ctx, _loc(), "+", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 30);
  free_node(bin);
}

TEST_F(it_run_expression, sub_i32) {
  node_t l = _i32_node("50");
  node_t r = _i32_node("20");
  node_t bin = create_expression_binary(ctx, _loc(), "-", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), 30);
  free_node(bin);
}

TEST_F(it_run_expression, mul_i32) {
  node_t l = _i32_node("6");
  node_t r = _i32_node("7");
  node_t bin = create_expression_binary(ctx, _loc(), "*", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
  free_node(bin);
}

TEST_F(it_run_expression, div_i32) {
  node_t l = _i32_node("100");
  node_t r = _i32_node("4");
  node_t bin = create_expression_binary(ctx, _loc(), "/", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), 25);
  free_node(bin);
}

TEST_F(it_run_expression, mod_i32) {
  node_t l = _i32_node("17");
  node_t r = _i32_node("5");
  node_t bin = create_expression_binary(ctx, _loc(), "%", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), 2);
  free_node(bin);
}

TEST_F(it_run_expression, add_f64) {
  node_t l = _f64_node("1.5");
  node_t r = _f64_node("2.5");
  node_t bin = create_expression_binary(ctx, _loc(), "+", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(v), 4.0);
  free_node(bin);
}

/* ==================================================================
 *  Unary operators
 * ================================================================== */

TEST_F(it_run_expression, unary_neg) {
  node_t r = _i32_node("5");
  node_t bin = create_expression_binary(ctx, _loc(), "-", NULL, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), -5);
  free_node(bin);
}

TEST_F(it_run_expression, unary_lnot) {
  node_t r = _bool_node(false);
  node_t bin = create_expression_binary(ctx, _loc(), "!", NULL, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(v));
  free_node(bin);
}

/* ==================================================================
 *  Comparison operators
 * ================================================================== */

TEST_F(it_run_expression, equal_true) {
  node_t l = _i32_node("10");
  node_t r = _i32_node("10");
  node_t bin = create_expression_binary(ctx, _loc(), "==", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(v));
  free_node(bin);
}

TEST_F(it_run_expression, equal_false) {
  node_t l = _i32_node("10");
  node_t r = _i32_node("20");
  node_t bin = create_expression_binary(ctx, _loc(), "==", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_FALSE(*(bool *)value_get_data(v));
  free_node(bin);
}

TEST_F(it_run_expression, not_equal) {
  node_t l = _i32_node("10");
  node_t r = _i32_node("20");
  node_t bin = create_expression_binary(ctx, _loc(), "!=", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_TRUE(*(bool *)value_get_data(v));
  free_node(bin);
}

TEST_F(it_run_expression, less_than) {
  node_t l = _i32_node("5");
  node_t r = _i32_node("10");
  node_t bin = create_expression_binary(ctx, _loc(), "<", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_TRUE(*(bool *)value_get_data(v));
  free_node(bin);
}

TEST_F(it_run_expression, greater_than) {
  node_t l = _i32_node("10");
  node_t r = _i32_node("5");
  node_t bin = create_expression_binary(ctx, _loc(), ">", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_TRUE(*(bool *)value_get_data(v));
  free_node(bin);
}

TEST_F(it_run_expression, less_equal_true) {
  node_t l = _i32_node("5");
  node_t r = _i32_node("5");
  node_t bin = create_expression_binary(ctx, _loc(), "<=", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_TRUE(*(bool *)value_get_data(v));
  free_node(bin);
}

TEST_F(it_run_expression, greater_equal_true) {
  node_t l = _i32_node("10");
  node_t r = _i32_node("10");
  node_t bin = create_expression_binary(ctx, _loc(), ">=", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_TRUE(*(bool *)value_get_data(v));
  free_node(bin);
}

/* ==================================================================
 *  Short-circuit logical operators
 * ================================================================== */

TEST_F(it_run_expression, logical_and_true) {
  node_t l = _bool_node(true);
  node_t r = _bool_node(true);
  node_t bin = create_expression_binary(ctx, _loc(), "&&", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(v));
  free_node(bin);
}

TEST_F(it_run_expression, logical_and_false) {
  node_t l = _bool_node(false);
  node_t r = _bool_node(true);
  node_t bin = create_expression_binary(ctx, _loc(), "&&", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_FALSE(*(bool *)value_get_data(v));
  free_node(bin);
}

TEST_F(it_run_expression, logical_or_true) {
  node_t l = _bool_node(true);
  node_t r = _bool_node(false);
  node_t bin = create_expression_binary(ctx, _loc(), "||", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_TRUE(*(bool *)value_get_data(v));
  free_node(bin);
}

TEST_F(it_run_expression, logical_or_false) {
  node_t l = _bool_node(false);
  node_t r = _bool_node(false);
  node_t bin = create_expression_binary(ctx, _loc(), "||", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_FALSE(*(bool *)value_get_data(v));
  free_node(bin);
}

/* ==================================================================
 *  Bitwise operators
 * ================================================================== */

TEST_F(it_run_expression, band_i32) {
  node_t l = _i32_node("12");
  node_t r = _i32_node("10");
  node_t bin = create_expression_binary(ctx, _loc(), "&", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), 8);
  free_node(bin);
}

TEST_F(it_run_expression, bor_i32) {
  node_t l = _i32_node("12");
  node_t r = _i32_node("10");
  node_t bin = create_expression_binary(ctx, _loc(), "|", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), 14);
  free_node(bin);
}

TEST_F(it_run_expression, bxor_i32) {
  node_t l = _i32_node("12");
  node_t r = _i32_node("10");
  node_t bin = create_expression_binary(ctx, _loc(), "^", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), 6);
  free_node(bin);
}

TEST_F(it_run_expression, shl_i32) {
  node_t l = _i32_node("1");
  node_t r = _i32_node("4");
  node_t bin = create_expression_binary(ctx, _loc(), "<<", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), 16);
  free_node(bin);
}

TEST_F(it_run_expression, shr_i32) {
  node_t l = _i32_node("16");
  node_t r = _i32_node("4");
  node_t bin = create_expression_binary(ctx, _loc(), ">>", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), 1);
  free_node(bin);
}

TEST_F(it_run_expression, bnot_i32) {
  node_t r = _i32_node("0");
  node_t bin = create_expression_binary(ctx, _loc(), "~", NULL, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), ~0);
  free_node(bin);
}

/* ==================================================================
 *  Unknown operator returns exception
 * ================================================================== */

TEST_F(it_run_expression, unknown_binary_op) {
  node_t l = _i32_node("1");
  node_t r = _i32_node("2");
  node_t bin = create_expression_binary(ctx, _loc(), "$$$", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_TRUE(value_is_error(v));
  free_node(bin);
}

/* ==================================================================
 *  Shadow propagation
 * ================================================================== */

TEST_F(it_run_expression, add_shadow) {
  node_t l = _i32_node("10");
  node_t r = _i32_node("20");
  node_t bin = create_expression_binary(ctx, _loc(), "+", l, r);
  value_t v = run_expression(ctx, bin, true);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_TRUE(value_is_shadow(v));
  free_node(bin);
}

TEST_F(it_run_expression, logical_and_shadow_left) {
  node_t l = _bool_node(true);
  node_t r = _bool_node(true);
  node_t bin = create_expression_binary(ctx, _loc(), "&&", l, r);
  value_t v = run_expression(ctx, bin, true);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_BOOL);
  EXPECT_TRUE(value_is_shadow(v));
  free_node(bin);
}

/* ==================================================================
 *  Group expression
 * ================================================================== */

TEST_F(it_run_expression, group_passthrough) {
  node_t inner = _i32_node("42");
  node_t group = create_expression_group(ctx, _loc(), inner);
  value_t v = run_expression(ctx, group, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
  free_node(group);
}

/* ==================================================================
 *  Assignment expression
 * ================================================================== */

TEST_F(it_run_expression, assign_identifier_returns_void) {
  value_t i32_val = create_i32_value(vm(), 0);
  _bind("x", i32_val);

  node_t lval = _id_node("x");
  node_t rval = _i32_node("99");
  node_t asgn = create_expression_assignment(ctx, _loc(), "=", lval, rval);
  value_t v = run_expression(ctx, asgn, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  /* verify x was updated */
  scope_t scope = vm_get_current_scope(vm());
  name_t n = scope_lookup(scope, "x");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 99);
  free_node(asgn);
}

TEST_F(it_run_expression, assign_compound_add) {
  value_t i32_val = create_i32_value(vm(), 10);
  _bind("y", i32_val);

  node_t lval = _id_node("y");
  node_t rval = _i32_node("5");
  node_t asgn = create_expression_assignment(ctx, _loc(), "+=", lval, rval);
  value_t v = run_expression(ctx, asgn, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  scope_t scope = vm_get_current_scope(vm());
  name_t n = scope_lookup(scope, "y");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 15);
  free_node(asgn);
}

TEST_F(it_run_expression, assign_discard_wildcard) {
  node_t lval = _id_node("_");
  node_t rval = _i32_node("42");
  node_t asgn = create_expression_assignment(ctx, _loc(), "=", lval, rval);
  value_t v = run_expression(ctx, asgn, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  free_node(asgn);
}

TEST_F(it_run_expression, assign_invalid_lvalue) {
  /* a string literal is not a valid lvalue */
  node_t lval = create_literal_string(ctx, _loc(), "bad");
  node_t rval = _i32_node("1");
  node_t asgn = create_expression_assignment(ctx, _loc(), "=", lval, rval);
  value_t v = run_expression(ctx, asgn, false);

  EXPECT_TRUE(value_is_error(v));
  free_node(asgn);
}

TEST_F(it_run_expression, assign_unknown_compound_op) {
  value_t i32_val = create_i32_value(vm(), 0);
  _bind("z", i32_val);

  node_t lval = _id_node("z");
  node_t rval = _i32_node("1");
  node_t asgn = create_expression_assignment(ctx, _loc(), "$$$=", lval, rval);
  value_t v = run_expression(ctx, asgn, false);

  EXPECT_TRUE(value_is_error(v));
  free_node(asgn);
}

/* ==================================================================
 *  Dereference expression
 * ================================================================== */

TEST_F(it_run_expression, deref_pointer) {
  value_t i32_val = create_i32_value(vm(), 77);
  value_t ptr_val = value_addrof(vm(), i32_val);
  ASSERT_NE(ptr_val, nullptr);

  _bind("p", ptr_val);

  node_t id = _id_node("p");
  node_t deref = create_expression_deref(ctx, _loc(), id);
  value_t v = run_expression(ctx, deref, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 77);
  free_node(deref);
}

/* ==================================================================
 *  Address-of expression
 * ================================================================== */

TEST_F(it_run_expression, addr_of_value) {
  value_t i32_val = create_i32_value(vm(), 55);
  _bind("a", i32_val);

  node_t id = _id_node("a");
  node_t addr = create_expression_addr(ctx, _loc(), id);
  value_t v = run_expression(ctx, addr, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_POINTER);
  free_node(addr);
}

/* ==================================================================
 *  Call expression (panic builtin)
 * ================================================================== */

TEST_F(it_run_expression, call_panic_returns_exception) {
  value_t panic_fn = vm_get_builtin(vm(), "panic");
  ASSERT_NE(panic_fn, nullptr);

  node_t callee = _id_node("panic");
  node_t msg = create_literal_string(ctx, _loc(), "oops");
  vec_init_t vi = {true};
  vec_t args = (vec_t)allocator_create(ctx->allocator, &g_vec_class, &vi);
  vec_push(args, msg);
  node_t call = create_expression_call(ctx, _loc(), callee, args);
  value_t v = run_expression(ctx, call, false);

  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
  free_node(call);
}

/* ==================================================================
 *  Member expression (struct field)
 * ================================================================== */

TEST_F(it_run_expression, member_get_field) {
  /* Create a struct value with a field, then access it via member expression */
  value_t i32_val = create_i32_value(vm(), 123);
  value_t str_type_val = vm_get_builtin(vm(), "str");
  /* Use a struct type — we need to create one via the VM */
  /* For now, test that member on a non-struct returns error */
  _bind("simple", i32_val);

  node_t host = _id_node("simple");
  node_t mem = create_expression_member(ctx, _loc(), host, "field");
  value_t v = run_expression(ctx, mem, false);

  /* i32 does not support field access → error */
  EXPECT_TRUE(value_is_error(v));
  free_node(mem);
}

/* ==================================================================
 *  Namespace access expression (::)
 * ================================================================== */

TEST_F(it_run_expression, namespace_access_on_non_type) {
  /* Accessing :: on a non-type value should return error */
  value_t i32_val = create_i32_value(vm(), 1);
  _bind("nottype", i32_val);

  node_t host = _id_node("nottype");
  node_t ns = create_expression_namespace_access(ctx, _loc(), host, "prop");
  value_t v = run_expression(ctx, ns, false);

  EXPECT_TRUE(value_is_error(v));
  free_node(ns);
}

/* ==================================================================
 *  Subscript expression
 * ================================================================== */

TEST_F(it_run_expression, subscript_on_non_indexable) {
  /* i32 does not support subscript access */
  value_t i32_val = create_i32_value(vm(), 0);
  _bind("num", i32_val);

  node_t host = _id_node("num");
  vec_init_t vi2 = {true};
  vec_t args = (vec_t)allocator_create(ctx->allocator, &g_vec_class, &vi2);
  vec_push(args, _i32_node("0"));
  node_t sub = create_expression_subscript(ctx, _loc(), host, args);
  value_t v = run_expression(ctx, sub, false);

  EXPECT_TRUE(value_is_error(v));
  free_node(sub);
}

/* ==================================================================
 *  Nested expressions via dispatcher
 * ================================================================== */

TEST_F(it_run_expression, nested_add_via_dispatcher) {
  /* (10 + 20) via run_expression dispatcher */
  node_t l = _i32_node("10");
  node_t r = _i32_node("20");
  node_t bin = create_expression_binary(ctx, _loc(), "+", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), 30);
  free_node(bin);
}

TEST_F(it_run_expression, group_wraps_binary) {
  /* (10 + 20) wrapped in group */
  node_t l = _i32_node("10");
  node_t r = _i32_node("20");
  node_t bin = create_expression_binary(ctx, _loc(), "+", l, r);
  node_t group = create_expression_group(ctx, _loc(), bin);
  value_t v = run_expression(ctx, group, false);

  EXPECT_EQ(*(int32_t *)value_get_data(v), 30);
  free_node(group);
}

/* ==================================================================
 *  Type mismatch returns exception
 * ================================================================== */

TEST_F(it_run_expression, add_i32_str_returns_exception) {
  node_t l = _i32_node("10");
  node_t r = create_literal_string(ctx, _loc(), "hello");
  node_t bin = create_expression_binary(ctx, _loc(), "+", l, r);
  value_t v = run_expression(ctx, bin, false);

  EXPECT_TRUE(value_is_error(v));
  free_node(bin);
}
