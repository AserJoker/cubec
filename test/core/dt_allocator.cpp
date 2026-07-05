#include "core/allocator.h"
#include "core/vec.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_allocator : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_allocator, create_and_delete) {
  allocator_t alloc = create_allocator(NULL, NULL);
  ASSERT_NE(alloc, nullptr);
  delete_allocator(alloc);
}

TEST_F(dt_allocator, alloc_and_free) {
  void *ptr = allocator_alloc(allocator, 64);
  ASSERT_NE(ptr, nullptr);
  memset(ptr, 0xAB, 64);
  allocator_free(allocator, &ptr);
}

TEST_F(dt_allocator, alloc_zero_size) {
  void *ptr = allocator_alloc(allocator, 0);
  EXPECT_EQ(ptr, nullptr);
}

TEST_F(dt_allocator, free_null) {
  allocator_free(allocator, NULL);
}

TEST_F(dt_allocator, multiple_allocations) {
  void *ptrs[10];
  for (int i = 0; i < 10; i++) {
    ptrs[i] = allocator_alloc(allocator, (i + 1) * 16);
    ASSERT_NE(ptrs[i], nullptr);
  }
  for (int i = 0; i < 10; i++) {
    allocator_free(allocator, &ptrs[i]);
  }
}

TEST_F(dt_allocator, allocator_create) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  EXPECT_NE(vec, nullptr);
  allocator_free(allocator, &vec);
}

TEST_F(dt_allocator, value_get_type) {
  int *val = (int *)allocator_alloc(allocator, sizeof(int));
  *val = 42;
  type_t *type = value_get_type(val);
  EXPECT_EQ(type, nullptr);
  allocator_free(allocator, &val);
}

TEST_F(dt_allocator, value_get_id) {
  int *val1 = (int *)allocator_alloc(allocator, sizeof(int));
  int *val2 = (int *)allocator_alloc(allocator, sizeof(int));
  uint64_t id1 = value_get_id(val1);
  uint64_t id2 = value_get_id(val2);
  EXPECT_NE(id1, id2);
  allocator_free(allocator, &val1);
  allocator_free(allocator, &val2);
}

TEST_F(dt_allocator, value_clone) {
  int *val = (int *)allocator_alloc(allocator, sizeof(int));
  *val = 42;
  int *cloned = (int *)value_clone(allocator, val);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(*cloned, 42);
  allocator_free(allocator, &val);
  allocator_free(allocator, &cloned);
}

TEST_F(dt_allocator, value_clone_null) {
  void *cloned = value_clone(allocator, NULL);
  EXPECT_EQ(cloned, nullptr);
}

TEST_F(dt_allocator, value_move) {
  int *val = (int *)allocator_alloc(allocator, sizeof(int));
  *val = 42;
  int *moved = (int *)value_move(allocator, val);
  ASSERT_NE(moved, nullptr);
  // value_move copies data for types without type info
  EXPECT_EQ(*moved, 42);
  allocator_free(allocator, &val);
  allocator_free(allocator, &moved);
}

TEST_F(dt_allocator, value_move_null) {
  void *moved = value_move(allocator, NULL);
  EXPECT_EQ(moved, nullptr);
}