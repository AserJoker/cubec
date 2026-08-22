#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/str_type.h"
#include "engine/struct_type.h"
#include "engine/union_type.h"
#include "engine/cunion_type.h"
#include "engine/interface_type.h"
#include "engine/generic_type.h"
#include "engine/generic_param.h"
#include "engine/callable_type.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/program.h"
#include "cubec/expression.h"
#include "cubec/token.h"
#include "cubec/node.h"
#include "run/run.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_run_struct_union : public CubecTest {
protected:
  type_t _get_i32_type() {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_f64_type() {
    return (type_t)value_get_data(vm_get_f64_type(vm));
  }
  type_t _get_bool_type() {
    return (type_t)value_get_data(vm_get_bool_type(vm));
  }
  type_t _get_void_type() {
    return (type_t)value_get_data(vm_get_void_type(vm));
  }
  type_t _get_str_type() {
    return (type_t)value_get_data(vm_get_str_type(vm));
  }
  type_t _get_u64_type() {
    return (type_t)value_get_data(vm_get_u64_type(vm));
  }

  value_t _type_val(type_t t) {
    type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
    return vm_create_value_ref(vm, type_type, t, NULL);
  }

  /* Parse + run source. If out_node is provided, caller must free node
   * after vm_dispose (needed when AST is borrowed by generic types). */
  value_t _run_source(const char *source, node_t *out_node = NULL,
                      bool shadow = false) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_t tokens = resolve_token_list(vm, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_program_node(vm, tokens, &position, "test.cubec");
    allocator_free(alloc, &tokens);
    if (!node) return NULL;
    value_t v = run_program(vm, node, shadow);
    if (out_node) {
      *out_node = node;
    } else {
      allocator_free(alloc, &node);
    }
    return v;
  }

  /* Parse + run a single expression */
  value_t _run_expr(const char *source, bool shadow = false) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_t tokens = resolve_token_list(vm, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_expression(vm, tokens, &position, "test.cubec");
    allocator_free(alloc, &tokens);
    if (!node) return NULL;
    value_t v = run_expression(vm, node, shadow);
    allocator_free(alloc, &node);
    return v;
  }

  void TearDown() override {
    /* For tests that hold ast_node past vm_dispose, free after. */
    if (held_node_) {
      if (vm) vm_dispose(vm, allocator);
      allocator_free(allocator, &held_node_);
      vm = NULL;
      /* skip base TearDown's vm_dispose, do leak check manually */
      if (allocator) {
        size_t total = allocator_get_total(allocator);
        size_t allocs = allocator_get_alloc_count(allocator);
        size_t frees = allocator_get_free_count(allocator);
        bool leaked = (total > 0 || allocs != frees);
        delete_allocator(allocator);
        allocator = NULL;
        if (leaked) {
          ADD_FAILURE() << "[CubecTest] memory leak detected: still_allocated="
                        << total << " bytes, allocs=" << allocs
                        << ", frees=" << frees;
        }
      }
    } else {
      CubecTest::TearDown();
    }
  }

  node_t held_node_ = NULL;
};

/* ---- Non-generic struct declaration ---- */

TEST_F(it_run_struct_union, struct_simple_fields) {
  value_t result = _run_source("struct Point { x: f64; y: f64; }");
  ASSERT_FALSE(value_is_abnormal(result));

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Point");
  ASSERT_NE(n, nullptr);
  ASSERT_NE(n->ref, nullptr);

  value_t type_val = n->ref;
  ASSERT_EQ(type_get_kind(value_get_type(type_val)), TYPE_KIND_TYPE);
  type_t inner = (type_t)value_get_data(type_val);
  ASSERT_EQ(type_get_kind(inner), TYPE_KIND_STRUCT);
  EXPECT_STREQ(type_get_name(inner), "Point");

  vec_t fields = vm_struct_get_fields(vm, type_val);
  ASSERT_EQ(vec_get_size(fields), 2u);
}

TEST_F(it_run_struct_union, struct_pub_field) {
  value_t result = _run_source("struct Data { pub x: i32; y: i32; }");
  ASSERT_FALSE(value_is_abnormal(result));

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Data");
  ASSERT_NE(n, nullptr);
  value_t type_val = n->ref;

  EXPECT_TRUE(vm_struct_is_field_pub(vm, type_val, "x"));
  EXPECT_FALSE(vm_struct_is_field_pub(vm, type_val, "y"));
}

/* ---- Non-generic union declaration ---- */

TEST_F(it_run_struct_union, union_simple_fields) {
  value_t result = _run_source("union Result { value: i32; error: void; }");
  ASSERT_FALSE(value_is_abnormal(result));

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Result");
  ASSERT_NE(n, nullptr);
  ASSERT_NE(n->ref, nullptr);

  value_t type_val = n->ref;
  ASSERT_EQ(type_get_kind(value_get_type(type_val)), TYPE_KIND_TYPE);
  type_t inner = (type_t)value_get_data(type_val);
  ASSERT_EQ(type_get_kind(inner), TYPE_KIND_UNION);

  vec_t fields = vm_union_get_fields(vm, type_val);
  ASSERT_EQ(vec_get_size(fields), 2u);
}

/* ---- Cunion declaration ---- */

TEST_F(it_run_struct_union, cunion_simple_fields) {
  value_t result = _run_source("cunion Data { int_val: i32; float_val: f64; }");
  ASSERT_FALSE(value_is_abnormal(result));

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Data");
  ASSERT_NE(n, nullptr);
  ASSERT_NE(n->ref, nullptr);

  value_t type_val = n->ref;
  ASSERT_EQ(type_get_kind(value_get_type(type_val)), TYPE_KIND_TYPE);
  type_t inner = (type_t)value_get_data(type_val);
  ASSERT_EQ(type_get_kind(inner), TYPE_KIND_CUNION);

  vec_t fields = vm_cunion_get_fields(vm, type_val);
  ASSERT_EQ(vec_get_size(fields), 2u);
}

/* ---- Interface declaration ---- */

TEST_F(it_run_struct_union, interface_simple_methods) {
  value_t result = _run_source(
      "interface Printable { func to_string(): *u8; }");
  ASSERT_FALSE(value_is_abnormal(result));

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Printable");
  ASSERT_NE(n, nullptr);
  ASSERT_NE(n->ref, nullptr);

  value_t type_val = n->ref;
  ASSERT_EQ(type_get_kind(value_get_type(type_val)), TYPE_KIND_TYPE);
  type_t inner = (type_t)value_get_data(type_val);
  ASSERT_EQ(type_get_kind(inner), TYPE_KIND_INTERFACE);

  strmap_t methods = vm_interface_get_methods(vm, type_val);
  ASSERT_NE(methods, nullptr);
  EXPECT_NE(strmap_find(methods, "to_string"), nullptr);
}

TEST_F(it_run_struct_union, interface_method_with_params) {
  value_t result = _run_source(
      "interface Container { func get(idx: u64): i32; func len(): u64; }");
  ASSERT_FALSE(value_is_abnormal(result));

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Container");
  ASSERT_NE(n, nullptr);
  value_t type_val = n->ref;

  strmap_t methods = vm_interface_get_methods(vm, type_val);
  EXPECT_NE(strmap_find(methods, "get"), nullptr);
  EXPECT_NE(strmap_find(methods, "len"), nullptr);
}

/* ---- Generic struct declaration ---- */

TEST_F(it_run_struct_union, generic_struct_instantiation) {
  value_t result = _run_source(
      "struct Pair[T] { first: T; second: T; }", &held_node_);
  ASSERT_FALSE(value_is_abnormal(result));

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Pair");
  ASSERT_NE(n, nullptr);
  ASSERT_NE(n->ref, nullptr);

  value_t generic_val = n->ref;
  ASSERT_EQ(type_get_kind(value_get_type(generic_val)), TYPE_KIND_GENERIC);

  /* Instantiate Pair[i32] via value_instantiate */
  value_t i32_type_val = _type_val(_get_i32_type());
  value_t argv[] = {i32_type_val};
  value_t instance = value_instantiate(vm, generic_val, 1, argv);
  ASSERT_FALSE(value_is_abnormal(instance))
      << "instantiation failed: "
      << (value_is_abnormal(instance)
              ? (const char *)value_get_data(instance)
              : "");

  ASSERT_EQ(type_get_kind(value_get_type(instance)), TYPE_KIND_TYPE);
  type_t instance_type = (type_t)value_get_data(instance);
  ASSERT_EQ(type_get_kind(instance_type), TYPE_KIND_STRUCT);

  vec_t fields = vm_struct_get_fields(vm, instance);
  ASSERT_EQ(vec_get_size(fields), 2u);
}

/* ---- Generic union declaration ---- */

TEST_F(it_run_struct_union, generic_union_instantiation) {
  value_t result = _run_source(
      "union Option[T] { value: T; empty: void; }", &held_node_);
  ASSERT_FALSE(value_is_abnormal(result));

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Option");
  ASSERT_NE(n, nullptr);
  value_t generic_val = n->ref;

  ASSERT_EQ(type_get_kind(value_get_type(generic_val)), TYPE_KIND_GENERIC);

  /* Instantiate Option[i32] */
  value_t i32_type_val = _type_val(_get_i32_type());
  value_t argv[] = {i32_type_val};
  value_t instance = value_instantiate(vm, generic_val, 1, argv);
  ASSERT_FALSE(value_is_abnormal(instance))
      << "instantiation failed: "
      << (value_is_abnormal(instance)
              ? (const char *)value_get_data(instance)
              : "");

  ASSERT_EQ(type_get_kind(value_get_type(instance)), TYPE_KIND_TYPE);
  type_t instance_type = (type_t)value_get_data(instance);
  ASSERT_EQ(type_get_kind(instance_type), TYPE_KIND_UNION);

  vec_t fields = vm_union_get_fields(vm, instance);
  ASSERT_EQ(vec_get_size(fields), 2u);
}

/* ---- Generic interface declaration ---- */

TEST_F(it_run_struct_union, generic_interface_instantiation) {
  value_t result = _run_source(
      "interface Iterator[T] { func next(): T; }", &held_node_);
  ASSERT_FALSE(value_is_abnormal(result));

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Iterator");
  ASSERT_NE(n, nullptr);
  value_t generic_val = n->ref;

  ASSERT_EQ(type_get_kind(value_get_type(generic_val)), TYPE_KIND_GENERIC);

  /* Instantiate Iterator[i32] */
  value_t i32_type_val = _type_val(_get_i32_type());
  value_t argv[] = {i32_type_val};
  value_t instance = value_instantiate(vm, generic_val, 1, argv);
  ASSERT_FALSE(value_is_abnormal(instance))
      << "instantiation failed: "
      << (value_is_abnormal(instance)
              ? (const char *)value_get_data(instance)
              : "");

  ASSERT_EQ(type_get_kind(value_get_type(instance)), TYPE_KIND_TYPE);
  type_t instance_type = (type_t)value_get_data(instance);
  ASSERT_EQ(type_get_kind(instance_type), TYPE_KIND_INTERFACE);

  strmap_t methods = vm_interface_get_methods(vm, instance);
  EXPECT_NE(strmap_find(methods, "next"), nullptr);
}

/* ---- Struct with method ---- */

TEST_F(it_run_struct_union, struct_with_method) {
  value_t result = _run_source(
      "struct Counter { value: i32; func get(): i32 { return 0; } }");
  ASSERT_FALSE(value_is_abnormal(result));

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Counter");
  ASSERT_NE(n, nullptr);
  value_t type_val = n->ref;

  vec_t fields = vm_struct_get_fields(vm, type_val);
  ASSERT_EQ(vec_get_size(fields), 1u);

  strmap_t methods = vm_struct_get_methods(vm, type_val);
  EXPECT_NE(strmap_find(methods, "get"), nullptr);
}

/* ---- Multiple structs in same scope ---- */

TEST_F(it_run_struct_union, multiple_structs) {
  value_t result = _run_source(
      "struct Point { x: f64; y: f64; } "
      "struct Color { r: i32; g: i32; b: i32; }");
  ASSERT_FALSE(value_is_abnormal(result));

  scope_t scope = vm_get_current_scope(vm);
  name_t pt = scope_lookup(scope, "Point");
  name_t col = scope_lookup(scope, "Color");
  ASSERT_NE(pt, nullptr);
  ASSERT_NE(col, nullptr);

  vec_t pt_fields = vm_struct_get_fields(vm, pt->ref);
  vec_t col_fields = vm_struct_get_fields(vm, col->ref);
  EXPECT_EQ(vec_get_size(pt_fields), 2u);
  EXPECT_EQ(vec_get_size(col_fields), 3u);
}

/* ---- Generic inference from struct field ---- */

TEST_F(it_run_struct_union, generic_infer_anon_struct_field) {
  /* func get_id[T](arg: struct { id: T; }): T { return arg.id; };
   * Calling get_id(.{.id = 42}) should infer T = i32 and return 42. */
  value_t result = _run_source(
      "func get_id[T](arg: struct { id: T; }): T { return arg.id; };",
      &held_node_);
  ASSERT_FALSE(value_is_abnormal(result));

  /* Call get_id with anonymous struct value */
  value_t v = _run_expr("get_id(.{.id = 42})");
  ASSERT_FALSE(value_is_abnormal(v))
      << "generic inference failed: "
      << (value_is_abnormal(v) ? (const char *)value_get_data(v) : "");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);
}

TEST_F(it_run_struct_union, generic_infer_named_struct_field) {
  /* struct Wrapper[T] { pub value: T; };
   * func unwrap[T](arg: Wrapper[T]): T { return arg.value; };
   * Calling unwrap(.Wrapper[f64]{.value = 3.14}) should infer T = f64. */
  value_t result = _run_source(
      "struct Wrapper[T] { pub value: T; }; "
      "func unwrap[T](arg: Wrapper[T]): T { return arg.value; };",
      &held_node_);
  ASSERT_FALSE(value_is_abnormal(result));

  /* Explicit type args on struct, inferred on function */
  value_t v = _run_expr("unwrap(.Wrapper[f64]{.value = 3.14})");
  ASSERT_FALSE(value_is_abnormal(v))
      << "generic inference failed: "
      << (value_is_abnormal(v) ? (const char *)value_get_data(v) : "");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_F64);
  EXPECT_DOUBLE_EQ(*(double *)value_get_data(v), 3.14);
}

TEST_F(it_run_struct_union, generic_infer_union_field) {
  /* union Option[T] { value: T; empty: void; };
   * func identity[T](arg: Option[T]): Option[T] { return arg; };
   * Calling identity(.Option[i32]{.value = 99}) should infer T = i32
   * and return the same union value. */
  value_t result = _run_source(
      "union Option[T] { value: T; empty: void; }; "
      "func identity[T](arg: Option[T]): Option[T] { return arg; };",
      &held_node_);
  ASSERT_FALSE(value_is_abnormal(result));

  value_t v = _run_expr("identity(.Option[i32]{.value = 99})");
  ASSERT_FALSE(value_is_abnormal(v))
      << "generic inference failed: "
      << (value_is_abnormal(v) ? (const char *)value_get_data(v) : "");
  /* v should be a union Option[i32] value */
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_UNION);
}

TEST_F(it_run_struct_union, generic_infer_multi_param_struct) {
  /* struct Pair[T, U] { pub first: T; pub second: U; };
   * func get_first[T, U](arg: Pair[T, U]): T { return arg.first; };
   * Calling get_first(.Pair[bool, i32]{.first = true, .second = 42}) should infer T=bool, U=i32. */
  value_t result = _run_source(
      "struct Pair[T, U] { pub first: T; pub second: U; }; "
      "func get_first[T, U](arg: Pair[T, U]): T { return arg.first; };",
      &held_node_);
  ASSERT_FALSE(value_is_abnormal(result));

  value_t v = _run_expr("get_first(.Pair[bool, i32]{.first = true, .second = 42})");
  ASSERT_FALSE(value_is_abnormal(v))
      << "generic inference failed: "
      << (value_is_abnormal(v) ? (const char *)value_get_data(v) : "");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(v));
}

/* ---- Struct implement interface ---- */

TEST_F(it_run_struct_union, struct_implement_interface_valid) {
  /* struct Counter implement Printable { value: i32; func to_string(): *u8 { return 0; } };
   * Should succeed — Counter has the to_string method. */
  value_t result = _run_source(
      "interface Printable { func to_string(): *u8; } "
      "struct Counter implement Printable { value: i32; func to_string(): *u8 { return 0; } };",
      &held_node_);
  ASSERT_FALSE(value_is_abnormal(result))
      << (value_is_abnormal(result) ? (const char *)value_get_data(result) : "");
}

TEST_F(it_run_struct_union, struct_implement_interface_missing_method) {
  /* struct Bad implement Printable { value: i32; };
   * Should fail — Bad lacks to_string method. */
  _run_source("interface Printable { func to_string(): *u8; }");

  value_t result = _run_source(
      "interface Printable { func to_string(): *u8; } "
      "struct Bad implement Printable { value: i32; };");
  EXPECT_TRUE(value_is_abnormal(result));
}

TEST_F(it_run_struct_union, struct_implement_non_interface_error) {
  /* struct Foo implement i32 { x: i32; };
   * Should fail — i32 is not an interface. */
  value_t result = _run_source(
      "struct Foo implement i32 { x: i32; };");
  EXPECT_TRUE(value_is_abnormal(result));
}

TEST_F(it_run_struct_union, union_implement_interface_valid) {
  /* union Option implement Printable { value: i32; empty: void; func to_string(): *u8 { return 0; } };
   * Should succeed — Option has to_string. */
  value_t result = _run_source(
      "interface Printable { func to_string(): *u8; } "
      "union Option implement Printable { value: i32; empty: void; func to_string(): *u8 { return 0; } };",
      &held_node_);
  ASSERT_FALSE(value_is_abnormal(result))
      << (value_is_abnormal(result) ? (const char *)value_get_data(result) : "");
}

TEST_F(it_run_struct_union, union_implement_interface_missing_method) {
  /* union Bad implement Printable { value: i32; empty: void; };
   * Should fail — Bad lacks to_string. */
  value_t result = _run_source(
      "interface Printable { func to_string(): *u8; } "
      "union Bad implement Printable { value: i32; empty: void; };");
  EXPECT_TRUE(value_is_abnormal(result));
}
