#include "core/allocator.h"
#include "core/vec.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class DtAllocator : public Test {
protected:
  TEST_ALLOCATOR;
};

TEST_F(DtAllocator, CreateAndDelete) {
  allocator_t alloc = create_allocator(NULL, NULL);
  ASSERT_NE(alloc, nullptr);
  delete_allocator(alloc);
}

TEST_F(DtAllocator, AllocAndFree) {
  void *ptr = allocator_alloc(allocator, 64);
  ASSERT_NE(ptr, nullptr);
  memset(ptr, 0xAB, 64);
  allocator_free(allocator, ptr);
}

TEST_F(DtAllocator, AllocZeroSize) {
  void *ptr = allocator_alloc(allocator, 0);
  EXPECT_EQ(ptr, nullptr);
}

TEST_F(DtAllocator, FreeNull) {
  allocator_free(allocator, NULL);
}

TEST_F(DtAllocator, MultipleAllocations) {
  void *ptrs[10];
  for (int i = 0; i < 10; i++) {
    ptrs[i] = allocator_alloc(allocator, (i + 1) * 16);
    ASSERT_NE(ptrs[i], nullptr);
  }
  for (int i = 0; i < 10; i++) {
    allocator_free(allocator, ptrs[i]);
  }
}

TEST_F(DtAllocator, AllocatorCreate) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  EXPECT_NE(vec, nullptr);
  allocator_free(allocator, vec);
}

TEST_F(DtAllocator, ValueGetType) {
  int *val = (int *)allocator_alloc(allocator, sizeof(int));
  *val = 42;
  type_t *type = value_get_type(val);
  EXPECT_EQ(type, nullptr);
  allocator_free(allocator, val);
}

TEST_F(DtAllocator, ValueGetId) {
  int *val1 = (int *)allocator_alloc(allocator, sizeof(int));
  int *val2 = (int *)allocator_alloc(allocator, sizeof(int));
  uint64_t id1 = value_get_id(val1);
  uint64_t id2 = value_get_id(val2);
  EXPECT_NE(id1, id2);
  allocator_free(allocator, val1);
  allocator_free(allocator, val2);
}

TEST_F(DtAllocator, ValueClone) {
  int *val = (int *)allocator_alloc(allocator, sizeof(int));
  *val = 42;
  int *cloned = (int *)value_clone(allocator, val);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(*cloned, 42);
  allocator_free(allocator, val);
  allocator_free(allocator, cloned);
}

TEST_F(DtAllocator, ValueCloneNull) {
  void *cloned = value_clone(allocator, NULL);
  EXPECT_EQ(cloned, nullptr);
}

TEST_F(DtAllocator, ValueMove) {
  int *val = (int *)allocator_alloc(allocator, sizeof(int));
  *val = 42;
  int *moved = (int *)value_move(allocator, val);
  ASSERT_NE(moved, nullptr);
  // value_move clears data for types without type info
  EXPECT_EQ(*moved, 0);
  allocator_free(allocator, val);
  allocator_free(allocator, moved);
}

TEST_F(DtAllocator, ValueMoveNull) {
  void *moved = value_move(allocator, NULL);
  EXPECT_EQ(moved, nullptr);
}