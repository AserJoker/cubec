#include "engine/semantic_type.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "engine/symbol.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_semantic_type : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_semantic_type, create_named_primitive) {
  semantic_type_t t =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  ASSERT_NE(t, nullptr);
  EXPECT_EQ(semantic_type_get_kind(t), TYPE_I32);
  EXPECT_STREQ(semantic_type_get_name(t), "i32");
  EXPECT_FALSE(semantic_type_is_incomplete(t));

  allocator_free(allocator, &t);
}

TEST_F(dt_semantic_type, create_pointer) {
  semantic_type_t base =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t ptr = semantic_type_create_pointer(allocator, base);

  EXPECT_EQ(semantic_type_get_kind(ptr), TYPE_POINTER);
  EXPECT_EQ(semantic_type_get_name(ptr), nullptr);
  EXPECT_FALSE(semantic_type_is_incomplete(ptr));

  allocator_free(allocator, &ptr);
  allocator_free(allocator, &base);
}

TEST_F(dt_semantic_type, create_slice) {
  semantic_type_t elem =
      semantic_type_create_named(allocator, "u8", TYPE_U8);
  semantic_type_t sl = semantic_type_create_slice(allocator, elem);

  EXPECT_EQ(semantic_type_get_kind(sl), TYPE_SLICE);
  EXPECT_FALSE(semantic_type_is_incomplete(sl));

  allocator_free(allocator, &sl);
  allocator_free(allocator, &elem);
}

TEST_F(dt_semantic_type, create_array) {
  semantic_type_t elem =
      semantic_type_create_named(allocator, "f64", TYPE_F64);
  semantic_type_t arr = semantic_type_create_array(allocator, elem, 10, NULL);

  EXPECT_EQ(semantic_type_get_kind(arr), TYPE_ARRAY);
  EXPECT_FALSE(semantic_type_is_incomplete(arr));

  allocator_free(allocator, &arr);
  allocator_free(allocator, &elem);
}

TEST_F(dt_semantic_type, create_qualifier) {
  semantic_type_t base =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t q = semantic_type_create_qualifier(allocator, base, false, true);

  EXPECT_EQ(semantic_type_get_kind(q), TYPE_QUALIFIER);
  EXPECT_FALSE(semantic_type_is_incomplete(q));

  allocator_free(allocator, &q);
  allocator_free(allocator, &base);
}

TEST_F(dt_semantic_type, create_function) {
  semantic_type_t ret =
      semantic_type_create_named(allocator, "void", TYPE_VOID);
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(allocator, &g_vec_type, &vi);
  semantic_type_t ft =
      semantic_type_create_function(allocator, ret, params, false);

  EXPECT_EQ(semantic_type_get_kind(ft), TYPE_FUNCTION);
  EXPECT_FALSE(semantic_type_is_incomplete(ft));

  allocator_free(allocator, &ft);
  allocator_free(allocator, &ret);
}

TEST_F(dt_semantic_type, equals_same_name) {
  semantic_type_t a =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t b =
      semantic_type_create_named(allocator, "i32", TYPE_I32);

  EXPECT_TRUE(semantic_type_equals(a, b));

  allocator_free(allocator, &b);
  allocator_free(allocator, &a);
}

TEST_F(dt_semantic_type, equals_different_name) {
  semantic_type_t a =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t b =
      semantic_type_create_named(allocator, "i64", TYPE_I64);

  EXPECT_FALSE(semantic_type_equals(a, b));

  allocator_free(allocator, &b);
  allocator_free(allocator, &a);
}

TEST_F(dt_semantic_type, equals_pointer) {
  semantic_type_t i32_a =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t i32_b =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t pa = semantic_type_create_pointer(allocator, i32_a);
  semantic_type_t pb = semantic_type_create_pointer(allocator, i32_b);

  /* Both point to named "i32" — nominal equality */
  EXPECT_TRUE(semantic_type_equals(pa, pb));

  allocator_free(allocator, &pb);
  allocator_free(allocator, &pa);
  allocator_free(allocator, &i32_b);
  allocator_free(allocator, &i32_a);
}

TEST_F(dt_semantic_type, cannot_decay_array_to_slice) {
  /* Design: array→slice is NOT an implicit conversion */
  semantic_type_t elem =
      semantic_type_create_named(allocator, "u8", TYPE_U8);
  semantic_type_t arr = semantic_type_create_array(allocator, elem, 10, NULL);
  semantic_type_t sl = semantic_type_create_slice(allocator, elem);

  EXPECT_FALSE(semantic_type_can_decay(arr, sl));

  allocator_free(allocator, &sl);
  allocator_free(allocator, &arr);
  allocator_free(allocator, &elem);
}

TEST_F(dt_semantic_type, cannot_decay_array_to_pointer) {
  /* Design: array→pointer is NOT an implicit conversion */
  semantic_type_t elem =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t arr = semantic_type_create_array(allocator, elem, 5, NULL);
  semantic_type_t ptr = semantic_type_create_pointer(allocator, elem);

  EXPECT_FALSE(semantic_type_can_decay(arr, ptr));

  allocator_free(allocator, &ptr);
  allocator_free(allocator, &arr);
  allocator_free(allocator, &elem);
}

TEST_F(dt_semantic_type, cannot_decay_unrelated) {
  semantic_type_t a =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t b =
      semantic_type_create_named(allocator, "f64", TYPE_F64);

  EXPECT_FALSE(semantic_type_can_decay(a, b));

  allocator_free(allocator, &b);
  allocator_free(allocator, &a);
}

TEST_F(dt_semantic_type, implicit_convert_nil_to_pointer) {
  semantic_type_t nil_t =
      semantic_type_create_named(allocator, "nil", TYPE_NIL);
  semantic_type_t elem =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t ptr = semantic_type_create_pointer(allocator, elem);

  EXPECT_TRUE(semantic_type_can_implicit_convert(nil_t, ptr));

  allocator_free(allocator, &ptr);
  allocator_free(allocator, &elem);
  allocator_free(allocator, &nil_t);
}

TEST_F(dt_semantic_type, implicit_convert_same_type) {
  semantic_type_t t =
      semantic_type_create_named(allocator, "bool", TYPE_BOOL);
  EXPECT_TRUE(semantic_type_can_implicit_convert(t, t));
  allocator_free(allocator, &t);
}

TEST_F(dt_semantic_type, qualifier_const_flag) {
  semantic_type_t base =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t q = semantic_type_create_qualifier(allocator, base, true, false);

  EXPECT_TRUE(semantic_type_is_const(q));
  EXPECT_FALSE(semantic_type_is_volatile(q));

  allocator_free(allocator, &q);
  allocator_free(allocator, &base);
}

TEST_F(dt_semantic_type, qualifier_volatile_flag) {
  semantic_type_t base =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t q = semantic_type_create_qualifier(allocator, base, false, true);

  EXPECT_FALSE(semantic_type_is_const(q));
  EXPECT_TRUE(semantic_type_is_volatile(q));

  allocator_free(allocator, &q);
  allocator_free(allocator, &base);
}

TEST_F(dt_semantic_type, qualifier_both_flags) {
  semantic_type_t base =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  /* const volatile i32 → const(volatile(i32)): outer=const, inner=volatile */
  semantic_type_t inner =
      semantic_type_create_qualifier(allocator, base, false, true);
  semantic_type_t outer =
      semantic_type_create_qualifier(allocator, inner, true, false);

  EXPECT_TRUE(semantic_type_is_const(outer));
  EXPECT_FALSE(semantic_type_is_volatile(outer));

  EXPECT_FALSE(semantic_type_is_const(inner));
  EXPECT_TRUE(semantic_type_is_volatile(inner));

  allocator_free(allocator, &outer);
  allocator_free(allocator, &inner);
  allocator_free(allocator, &base);
}

TEST_F(dt_semantic_type, strip_qualifier) {
  semantic_type_t base =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t q = semantic_type_create_qualifier(allocator, base, true, false);

  semantic_type_t stripped = semantic_type_strip_qualifier(q);
  EXPECT_EQ(semantic_type_get_kind(stripped), TYPE_I32);
  EXPECT_EQ(stripped, base);

  /* Stripping a non-qualifier returns itself */
  semantic_type_t stripped2 = semantic_type_strip_qualifier(base);
  EXPECT_EQ(stripped2, base);

  allocator_free(allocator, &q);
  allocator_free(allocator, &base);
}

TEST_F(dt_semantic_type, is_const_volatile_non_qualifier) {
  semantic_type_t base =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  EXPECT_FALSE(semantic_type_is_const(base));
  EXPECT_FALSE(semantic_type_is_volatile(base));

  semantic_type_t ptr = semantic_type_create_pointer(allocator, base);
  EXPECT_FALSE(semantic_type_is_const(ptr));
  EXPECT_FALSE(semantic_type_is_volatile(ptr));

  allocator_free(allocator, &ptr);
  allocator_free(allocator, &base);
}

TEST_F(dt_semantic_type, implicit_convert_add_const) {
  /* i32 → const i32 should be allowed */
  semantic_type_t base =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t const_base =
      semantic_type_create_qualifier(allocator, base, true, false);

  EXPECT_TRUE(semantic_type_can_implicit_convert(base, const_base));
  /* const i32 → i32 should NOT be allowed */
  EXPECT_FALSE(semantic_type_can_implicit_convert(const_base, base));

  allocator_free(allocator, &const_base);
  allocator_free(allocator, &base);
}
