#include "core/vec.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_vec : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_vec, create_and_resize) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 0);
  EXPECT_EQ(vec_get_capacity(vec), 0);
  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, push_and_get) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_push(vec, &val3);
  EXPECT_EQ(vec_get_size(vec), 3);
  EXPECT_EQ(*(int *)vec_get(vec, 0), 10);
  EXPECT_EQ(*(int *)vec_get(vec, 1), 20);
  EXPECT_EQ(*(int *)vec_get(vec, 2), 30);
  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, pop) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  EXPECT_EQ(vec_get_size(vec), 2);
  vec_pop(vec);
  EXPECT_EQ(vec_get_size(vec), 1);
  vec_pop(vec);
  EXPECT_EQ(vec_get_size(vec), 0);
  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, set) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 99;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_set(vec, 1, &val3);
  EXPECT_EQ(*(int *)vec_get(vec, 1), 99);
  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, insert) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 15;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_insert(vec, 1, &val3);
  EXPECT_EQ(vec_get_size(vec), 3);
  EXPECT_EQ(*(int *)vec_get(vec, 0), 10);
  EXPECT_EQ(*(int *)vec_get(vec, 1), 15);
  EXPECT_EQ(*(int *)vec_get(vec, 2), 20);
  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, remove) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_push(vec, &val3);
  vec_remove(vec, 1);
  EXPECT_EQ(vec_get_size(vec), 2);
  EXPECT_EQ(*(int *)vec_get(vec, 0), 10);
  EXPECT_EQ(*(int *)vec_get(vec, 1), 30);
  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, resize) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_resize(vec, 5);
  EXPECT_EQ(vec_get_size(vec), 5);
  EXPECT_EQ(vec_get_capacity(vec), 8);
  vec_resize(vec, 1);
  EXPECT_EQ(vec_get_size(vec), 1);
  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, get_data) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  void **data = vec_get_data(vec);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(*(int *)data[0], 10);
  EXPECT_EQ(*(int *)data[1], 20);
  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, resize_down_to_zero) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  vec_resize(vec, 0);
  EXPECT_EQ(vec_get_size(vec), 0);
  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, initial_capacity) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  vec_resize(vec, 1);
  EXPECT_EQ(vec_get_capacity(vec), 8);
  allocator_free(allocator, &vec);
}

/* ============================================================================
 *  Iterator tests
 * ============================================================================ */

TEST_F(dt_vec, iteration) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_push(vec, &val3);
  vec_iter_t iter = vec_iter_first(vec);
  int count = 0;
  int expected[] = {10, 20, 30};
  void *data;
  while ((data = vec_iter_next(&iter)) != NULL) {
    EXPECT_EQ(*(int *)data, expected[count++]);
  }
  EXPECT_EQ(count, 3);
  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, iteration_empty_vec) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  vec_iter_t iter = vec_iter_first(vec);
  EXPECT_EQ(vec_iter_get(&iter), nullptr);
  EXPECT_EQ(vec_iter_next(&iter), nullptr);
  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, iter_get) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_push(vec, &val3);

  vec_iter_t iter = vec_iter_first(vec);
  EXPECT_EQ(*(int *)vec_iter_get(&iter), 10);

  vec_iter_next(&iter);
  EXPECT_EQ(*(int *)vec_iter_get(&iter), 20);

  vec_iter_next(&iter);
  EXPECT_EQ(*(int *)vec_iter_get(&iter), 30);

  /* Exhausted */
  vec_iter_next(&iter);
  EXPECT_EQ(vec_iter_get(&iter), nullptr);

  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, iter_set) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 99;
  vec_push(vec, &val1);
  vec_push(vec, &val2);

  vec_iter_t iter = vec_iter_first(vec);
  vec_iter_next(&iter);  /* advance to second element */
  void *old = vec_iter_set(&iter, &val3);
  EXPECT_EQ(*(int *)old, 20);
  EXPECT_EQ(*(int *)vec_iter_get(&iter), 99);
  EXPECT_EQ(*(int *)vec_get(vec, 1), 99);  /* verify underlying vec updated */

  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, iter_remove) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_push(vec, &val3);

  /* Remove second element (20) */
  vec_iter_t iter = vec_iter_first(vec);
  vec_iter_next(&iter);  /* at second element */
  vec_iter_remove(&iter);
  EXPECT_EQ(vec_get_size(vec), 2);

  /* Iterator stays at same index, now pointing to (was 30) */
  EXPECT_EQ(*(int *)vec_iter_get(&iter), 30);
  EXPECT_EQ(*(int *)vec_get(vec, 0), 10);
  EXPECT_EQ(*(int *)vec_get(vec, 1), 30);

  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, iter_remove_first) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_push(vec, &val3);

  /* Remove first element */
  vec_iter_t iter = vec_iter_first(vec);
  vec_iter_remove(&iter);
  EXPECT_EQ(vec_get_size(vec), 2);
  EXPECT_EQ(*(int *)vec_iter_get(&iter), 20);
  EXPECT_EQ(*(int *)vec_get(vec, 0), 20);

  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, iter_remove_last) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20;
  vec_push(vec, &val1);
  vec_push(vec, &val2);

  /* Navigate to last element */
  vec_iter_t iter = vec_iter_first(vec);
  vec_iter_next(&iter);  /* at last */
  vec_iter_remove(&iter);
  EXPECT_EQ(vec_get_size(vec), 1);
  EXPECT_EQ(*(int *)vec_get(vec, 0), 10);
  EXPECT_EQ(vec_iter_get(&iter), nullptr);  /* exhausted */

  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, iter_remove_exhausted) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10;
  vec_push(vec, &val1);

  vec_iter_t iter = vec_iter_first(vec);
  vec_iter_next(&iter);  /* exhaust */
  EXPECT_EQ(vec_iter_remove(&iter), nullptr);
  EXPECT_EQ(vec_get_size(vec), 1);  /* unchanged */

  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, iter_traverse_and_remove_all) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  vec_push(vec, &val1);
  vec_push(vec, &val2);
  vec_push(vec, &val3);

  /* Remove all elements via iterator */
  vec_iter_t iter = vec_iter_first(vec);
  EXPECT_EQ(*(int *)vec_iter_remove(&iter), 10);
  EXPECT_EQ(*(int *)vec_iter_remove(&iter), 20);
  EXPECT_EQ(*(int *)vec_iter_remove(&iter), 30);
  EXPECT_EQ(vec_get_size(vec), 0);
  EXPECT_EQ(vec_iter_get(&iter), nullptr);

  allocator_free(allocator, &vec);
}

TEST_F(dt_vec, iter_single_element) {
  vec_t vec = (vec_t)allocator_create(allocator, &g_vec_type, NULL);
  int val1 = 42;
  vec_push(vec, &val1);

  vec_iter_t iter = vec_iter_first(vec);
  EXPECT_EQ(*(int *)vec_iter_get(&iter), 42);
  vec_iter_next(&iter);
  EXPECT_EQ(vec_iter_get(&iter), nullptr);

  allocator_free(allocator, &vec);
}