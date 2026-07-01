#include "core/vec.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class DtVec : public Test {
protected:
  TEST_ALLOCATOR;
};

TEST_F(DtVec, CreateAndResize) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 0);
  EXPECT_EQ(vec_get_capacity(vec), 0);
  allocator_free(allocator, vec);
}

TEST_F(DtVec, PushAndGet) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_push(vec, &val3);
  EXPECT_EQ(vec_get_size(vec), 3);
  EXPECT_EQ(*(int *)vec_get(vec, 0), 10);
  EXPECT_EQ(*(int *)vec_get(vec, 1), 20);
  EXPECT_EQ(*(int *)vec_get(vec, 2), 30);
  allocator_free(allocator, vec);
}

TEST_F(DtVec, Pop) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  EXPECT_EQ(vec_get_size(vec), 2);
  vec_pop(vec);
  EXPECT_EQ(vec_get_size(vec), 1);
  vec_pop(vec);
  EXPECT_EQ(vec_get_size(vec), 0);
  allocator_free(allocator, vec);
}

TEST_F(DtVec, Set) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 99;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_set(vec, 1, &val3);
  EXPECT_EQ(*(int *)vec_get(vec, 1), 99);
  allocator_free(allocator, vec);
}

TEST_F(DtVec, Insert) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 15;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_insert(vec, 1, &val3);
  EXPECT_EQ(vec_get_size(vec), 3);
  EXPECT_EQ(*(int *)vec_get(vec, 0), 10);
  EXPECT_EQ(*(int *)vec_get(vec, 1), 15);
  EXPECT_EQ(*(int *)vec_get(vec, 2), 20);
  allocator_free(allocator, vec);
}

TEST_F(DtVec, Remove) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_push(vec, &val3);
  vec_remove(vec, 1);
  EXPECT_EQ(vec_get_size(vec), 2);
  EXPECT_EQ(*(int *)vec_get(vec, 0), 10);
  EXPECT_EQ(*(int *)vec_get(vec, 1), 30);
  allocator_free(allocator, vec);
}

TEST_F(DtVec, Resize) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_resize(vec, 5);
  EXPECT_EQ(vec_get_size(vec), 5);
  EXPECT_EQ(vec_get_capacity(vec), 8);
  vec_resize(vec, 1);
  EXPECT_EQ(vec_get_size(vec), 1);
  allocator_free(allocator, vec);
}

TEST_F(DtVec, GetData) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  void **data = vec_get_data(vec);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(*(int *)data[0], 10);
  EXPECT_EQ(*(int *)data[1], 20);
  allocator_free(allocator, vec);
}

TEST_F(DtVec, ResizeDownToZero) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  vec_resize(vec, 0);
  EXPECT_EQ(vec_get_size(vec), 0);
  allocator_free(allocator, vec);
}

TEST_F(DtVec, InitialCapacity) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  vec_resize(vec, 1);
  EXPECT_EQ(vec_get_capacity(vec), 8);
  allocator_free(allocator, vec);
}