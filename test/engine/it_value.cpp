#include "engine/type.h"
#include "engine/value.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_value : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  type_t _make_i32_type() {
    return type_create(allocator, TYPE_KIND_I32, "i32", 4, 4, false,
                       (vtable_t){0});
  }

  type_t _make_void_type() {
    return type_create(allocator, TYPE_KIND_VOID, "void", 0, 1, false,
                       (vtable_t){0});
  }
};

TEST_F(it_value, create_and_accessors) {
  type_t i32_type = _make_i32_type();
  int32_t *data = (int32_t *)allocator_alloc(allocator, sizeof(int32_t));
  *data = 42;

  value_t v = value_create(allocator, i32_type, data, true);
  EXPECT_EQ(value_get_type(v), i32_type);
  EXPECT_EQ(value_get_data(v), data);
  EXPECT_TRUE(value_is_own(v));
  EXPECT_FALSE(value_is_shadow(v));
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);

  allocator_free(allocator, &v);
  allocator_free(allocator, &i32_type);
  delete_allocator(allocator);
}

TEST_F(it_value, shadow_value) {
  type_t i32_type = _make_i32_type();
  value_t v = value_create(allocator, i32_type, NULL, false);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_FALSE(value_is_own(v));

  allocator_free(allocator, &v);
  allocator_free(allocator, &i32_type);
  delete_allocator(allocator);
}

TEST_F(it_value, dispose_owns_data) {
  type_t i32_type = _make_i32_type();
  int32_t *data = (int32_t *)allocator_alloc(allocator, sizeof(int32_t));
  *data = 99;

  value_t v = value_create(allocator, i32_type, data, true);
  allocator_free(allocator, &v);
  allocator_free(allocator, &i32_type);
  delete_allocator(allocator);
}

TEST_F(it_value, dispose_no_data_no_crash) {
  type_t void_type = _make_void_type();
  value_t v = value_create(allocator, void_type, NULL, false);
  allocator_free(allocator, &v);
  allocator_free(allocator, &void_type);
  delete_allocator(allocator);
}

TEST_F(it_value, move_transfers_data) {
  type_t i32_type = _make_i32_type();
  int32_t *data = (int32_t *)allocator_alloc(allocator, sizeof(int32_t));
  *data = 13;

  value_t v = value_create(allocator, i32_type, data, true);
  value_t moved = (value_t)alloc_move(allocator, v);

  EXPECT_EQ(value_get_type(moved), i32_type);
  EXPECT_TRUE(value_is_own(moved));
  EXPECT_EQ(*(int32_t *)value_get_data(moved), 13);

  EXPECT_EQ(value_get_data(v), nullptr);
  EXPECT_FALSE(value_is_own(v));

  allocator_free(allocator, &moved);
  allocator_free(allocator, &v);
  allocator_free(allocator, &i32_type);
  delete_allocator(allocator);
}

TEST_F(it_value, type_accessors) {
  type_t i32_type = _make_i32_type();
  EXPECT_EQ(type_get_kind(i32_type), TYPE_KIND_I32);
  EXPECT_STREQ(type_get_name(i32_type), "i32");
  EXPECT_EQ(type_get_size(i32_type), 4u);
  EXPECT_EQ(type_get_align(i32_type), 4u);

  allocator_free(allocator, &i32_type);
  delete_allocator(allocator);
}
