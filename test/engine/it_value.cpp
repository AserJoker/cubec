#include "engine/type.h"
#include "engine/value.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_value : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);

  static value_t _dummy_clone(allocator_t alloc, value_t obj) {
    size_t sz = type_get_size(value_get_type(obj));
    void *new_data = allocator_alloc(alloc, sz);
    memcpy(new_data, value_get_data(obj), sz);
    return value_create(alloc, value_get_type(obj), new_data, true);
  }

  static void _dummy_dispose(allocator_t alloc, value_t obj) {
    void *d = value_get_data(obj);
    allocator_free(alloc, &d);
  }

  type_t _make_i32_type() {
    type_t t = (type_t)allocator_alloc(allocator, sizeof(struct _type_t));
    t->kind = TYPE_KIND_I32;
    t->name = cstring_clone(allocator, "i32");
    t->size = 4;
    t->align = 4;
    t->mut = false;
    t->vtable = (vtable_t){.clone = _dummy_clone, .dispose = _dummy_dispose};
    return t;
  }

  type_t _make_void_type() {
    type_t t = (type_t)allocator_alloc(allocator, sizeof(struct _type_t));
    t->kind = TYPE_KIND_VOID;
    t->name = cstring_clone(allocator, "void");
    t->size = 0;
    t->align = 1;
    t->mut = false;
    t->vtable = (vtable_t){.clone = NULL, .dispose = NULL};
    return t;
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

  value_dispose(v, allocator);
  allocator_free(allocator, &i32_type->name);
  allocator_free(allocator, &i32_type);
  delete_allocator(allocator);
}

TEST_F(it_value, shadow_value) {
  type_t i32_type = _make_i32_type();
  value_t v = value_create(allocator, i32_type, NULL, false);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_FALSE(value_is_own(v));

  value_dispose(v, allocator);
  allocator_free(allocator, &i32_type->name);
  allocator_free(allocator, &i32_type);
  delete_allocator(allocator);
}

TEST_F(it_value, dispose_calls_vtable_dispose) {
  type_t i32_type = _make_i32_type();
  int32_t *data = (int32_t *)allocator_alloc(allocator, sizeof(int32_t));
  *data = 99;

  value_t v = value_create(allocator, i32_type, data, true);
  value_dispose(v, allocator);
  allocator_free(allocator, &i32_type->name);
  allocator_free(allocator, &i32_type);
  delete_allocator(allocator);
}

TEST_F(it_value, dispose_no_vtable_no_crash) {
  type_t void_type = _make_void_type();
  value_t v = value_create(allocator, void_type, NULL, false);
  value_dispose(v, allocator);
  allocator_free(allocator, &void_type->name);
  allocator_free(allocator, &void_type);
  delete_allocator(allocator);
}

TEST_F(it_value, clone_delegates_to_vtable) {
  type_t i32_type = _make_i32_type();
  int32_t *data = (int32_t *)allocator_alloc(allocator, sizeof(int32_t));
  *data = 7;

  value_t v = value_create(allocator, i32_type, data, true);
  value_t cloned = (value_t)alloc_clone(allocator, v);

  EXPECT_EQ(value_get_type(cloned), i32_type);
  EXPECT_TRUE(value_is_own(cloned));
  EXPECT_NE(value_get_data(cloned), value_get_data(v));
  EXPECT_EQ(*(int32_t *)value_get_data(cloned), 7);

  value_dispose(cloned, allocator);
  value_dispose(v, allocator);
  allocator_free(allocator, &i32_type->name);
  allocator_free(allocator, &i32_type);
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

  value_dispose(moved, allocator);
  value_dispose(v, allocator);
  allocator_free(allocator, &i32_type->name);
  allocator_free(allocator, &i32_type);
  delete_allocator(allocator);
}

TEST_F(it_value, type_accessors) {
  type_t i32_type = _make_i32_type();
  EXPECT_EQ(type_get_kind(i32_type), TYPE_KIND_I32);
  EXPECT_STREQ(type_get_name(i32_type), "i32");
  EXPECT_EQ(type_get_size(i32_type), 4u);
  EXPECT_EQ(type_get_align(i32_type), 4u);

  allocator_free(allocator, &i32_type->name);
  allocator_free(allocator, &i32_type);
  delete_allocator(allocator);
}
