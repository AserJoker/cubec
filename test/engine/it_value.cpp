#include "engine/stype.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_value : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);
};

/* ---- Minimal stype for testing ---- */

static value_t *_dummy_clone(allocator_t alloc, value_t *obj) {
  /* allocate new data and copy */
  size_t sz = obj->type->size;
  void *new_data = allocator_alloc(alloc, sz);
  memcpy(new_data, obj->data, sz);
  return value_create(alloc, obj->type, new_data, true);
}

static void _dummy_dispose(allocator_t alloc, value_t *obj) {
  allocator_free(alloc, &obj->data);
}

static stype_t _make_int32_type(void) {
  return (stype_t){
      .kind = TYPE_KIND_I32,
      .name = "i32",
      .size = 4,
      .align = 4,
      .vtable = {.clone = _dummy_clone, .dispose = _dummy_dispose},
  };
}

static stype_t _make_void_type(void) {
  return (stype_t){
      .kind = TYPE_KIND_VOID,
      .name = "void",
      .size = 0,
      .align = 1,
      .vtable = {.clone = NULL, .dispose = NULL},
  };
}

/* ---- Tests ---- */

TEST_F(it_value, create_and_accessors) {
  stype_t i32_type = _make_int32_type();
  int32_t *data = (int32_t *)allocator_alloc(allocator, sizeof(int32_t));
  *data = 42;

  value_t *v = value_create(allocator, &i32_type, data, true);
  EXPECT_EQ(value_get_type(v), &i32_type);
  EXPECT_EQ(value_get_data(v), data);
  EXPECT_TRUE(value_is_own(v));
  EXPECT_FALSE(value_is_shadow(v));
  EXPECT_EQ(*(int32_t *)value_get_data(v), 42);

  value_dispose(v, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value, shadow_value) {
  stype_t i32_type = _make_int32_type();
  value_t *v = value_create(allocator, &i32_type, NULL, false);
  EXPECT_TRUE(value_is_shadow(v));
  EXPECT_FALSE(value_is_own(v));

  value_dispose(v, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value, dispose_calls_vtable_dispose) {
  stype_t i32_type = _make_int32_type();
  int32_t *data = (int32_t *)allocator_alloc(allocator, sizeof(int32_t));
  *data = 99;

  value_t *v = value_create(allocator, &i32_type, data, true);
  value_dispose(v, allocator);
  /* data should have been freed by vtable.dispose — no crash = success */
  delete_allocator(allocator);
}

TEST_F(it_value, dispose_no_vtable_no_crash) {
  stype_t void_type = _make_void_type();
  value_t *v = value_create(allocator, &void_type, NULL, false);
  value_dispose(v, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value, clone_delegates_to_vtable) {
  stype_t i32_type = _make_int32_type();
  int32_t *data = (int32_t *)allocator_alloc(allocator, sizeof(int32_t));
  *data = 7;

  value_t *v = value_create(allocator, &i32_type, data, true);
  value_t *cloned = (value_t *)alloc_clone(allocator, v);

  EXPECT_EQ(value_get_type(cloned), &i32_type);
  EXPECT_TRUE(value_is_own(cloned));
  EXPECT_NE(value_get_data(cloned), value_get_data(v));
  EXPECT_EQ(*(int32_t *)value_get_data(cloned), 7);

  value_dispose(cloned, allocator);
  value_dispose(v, allocator);
  delete_allocator(allocator);
}

TEST_F(it_value, move_transfers_data) {
  stype_t i32_type = _make_int32_type();
  int32_t *data = (int32_t *)allocator_alloc(allocator, sizeof(int32_t));
  *data = 13;

  value_t *v = value_create(allocator, &i32_type, data, true);
  value_t *moved = (value_t *)alloc_move(allocator, v);

  EXPECT_EQ(value_get_type(moved), &i32_type);
  EXPECT_TRUE(value_is_own(moved));
  EXPECT_EQ(*(int32_t *)value_get_data(moved), 13);

  /* source should be cleared */
  EXPECT_EQ(value_get_data(v), nullptr);
  EXPECT_FALSE(value_is_own(v));

  value_dispose(moved, allocator);
  /* v's data is NULL + own=false, so dispose is safe */
  value_dispose(v, allocator);
  delete_allocator(allocator);
}

/* ---- stype_t tests ---- */

TEST_F(it_value, stype_accessors) {
  stype_t i32_type = _make_int32_type();
  EXPECT_EQ(stype_get_kind(&i32_type), TYPE_KIND_I32);
  EXPECT_STREQ(stype_get_name(&i32_type), "i32");
  EXPECT_EQ(stype_get_size(&i32_type), 4u);
  EXPECT_EQ(stype_get_align(&i32_type), 4u);
}

TEST_F(it_value, type_kind_type_bootstrap) {
  /* TYPE_KIND_TYPE: value.type and value.data point to same stype_t */
  stype_t meta_stype = {
      .kind = TYPE_KIND_TYPE,
      .name = "Type",
      .size = sizeof(stype_t),
      .align = 8,
      .vtable = {.clone = NULL, .dispose = NULL},
  };

  value_t *type_val =
      value_create(allocator, &meta_stype, &meta_stype, true);
  EXPECT_EQ(value_get_type(type_val), &meta_stype);
  EXPECT_EQ(value_get_data(type_val), &meta_stype);
  EXPECT_EQ(value_get_type(type_val), value_get_data(type_val));
  EXPECT_TRUE(value_is_own(type_val));

  value_dispose(type_val, allocator);
  delete_allocator(allocator);
}
