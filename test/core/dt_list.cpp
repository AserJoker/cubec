#include "core/list.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_list : public Test {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_list, create_and_destroy) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  ASSERT_NE(list, nullptr);
  EXPECT_EQ(list_get_size(list), 0);
  allocator_free(allocator, list);
}

TEST_F(dt_list, push_and_get) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  list_push(list, &val1);
  list_push(list, &val2);
  list_push(list, &val3);
  EXPECT_EQ(list_get_size(list), 3);
  EXPECT_EQ(*(int *)list_get(list, 0), 10);
  EXPECT_EQ(*(int *)list_get(list, 1), 20);
  EXPECT_EQ(*(int *)list_get(list, 2), 30);
  allocator_free(allocator, list);
}

TEST_F(dt_list, pop) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20;
  list_push(list, &val1);
  list_push(list, &val2);
  EXPECT_EQ(list_get_size(list), 2);
  EXPECT_EQ(*(int *)list_pop(list), 20);
  EXPECT_EQ(list_get_size(list), 1);
  EXPECT_EQ(*(int *)list_pop(list), 10);
  EXPECT_EQ(list_get_size(list), 0);
  allocator_free(allocator, list);
}

TEST_F(dt_list, unshift_and_shift) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  list_unshift(list, &val1);
  list_unshift(list, &val2);
  list_unshift(list, &val3);
  EXPECT_EQ(list_get_size(list), 3);
  EXPECT_EQ(*(int *)list_shift(list), 30);
  EXPECT_EQ(*(int *)list_shift(list), 20);
  EXPECT_EQ(*(int *)list_shift(list), 10);
  EXPECT_EQ(list_get_size(list), 0);
  allocator_free(allocator, list);
}

TEST_F(dt_list, get_first_and_last) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  list_push(list, &val1);
  list_push(list, &val2);
  list_push(list, &val3);
  EXPECT_EQ(*(int *)list_get_first(list), 10);
  EXPECT_EQ(*(int *)list_get_last(list), 30);
  allocator_free(allocator, list);
}

TEST_F(dt_list, set) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20, val3 = 99;
  list_push(list, &val1);
  list_push(list, &val2);
  list_set(list, 1, &val3);
  EXPECT_EQ(*(int *)list_get(list, 1), 99);
  allocator_free(allocator, list);
}

TEST_F(dt_list, insert) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20, val3 = 15;
  list_push(list, &val1);
  list_push(list, &val2);
  list_insert(list, 1, &val3);
  EXPECT_EQ(list_get_size(list), 3);
  EXPECT_EQ(*(int *)list_get(list, 0), 10);
  EXPECT_EQ(*(int *)list_get(list, 1), 15);
  EXPECT_EQ(*(int *)list_get(list, 2), 20);
  allocator_free(allocator, list);
}

TEST_F(dt_list, insert_at_beginning) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20;
  list_push(list, &val1);
  list_insert(list, 0, &val2);
  EXPECT_EQ(list_get_size(list), 2);
  EXPECT_EQ(*(int *)list_get_first(list), 20);
  allocator_free(allocator, list);
}

TEST_F(dt_list, insert_at_end) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20;
  list_push(list, &val1);
  list_insert(list, 1, &val2);
  EXPECT_EQ(list_get_size(list), 2);
  EXPECT_EQ(*(int *)list_get_last(list), 20);
  allocator_free(allocator, list);
}

TEST_F(dt_list, remove) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  list_push(list, &val1);
  list_push(list, &val2);
  list_push(list, &val3);
  list_remove(list, 1);
  EXPECT_EQ(list_get_size(list), 2);
  EXPECT_EQ(*(int *)list_get(list, 0), 10);
  EXPECT_EQ(*(int *)list_get(list, 1), 30);
  allocator_free(allocator, list);
}

TEST_F(dt_list, remove_first) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20;
  list_push(list, &val1);
  list_push(list, &val2);
  list_remove(list, 0);
  EXPECT_EQ(list_get_size(list), 1);
  EXPECT_EQ(*(int *)list_get_first(list), 20);
  allocator_free(allocator, list);
}

TEST_F(dt_list, remove_last) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20;
  list_push(list, &val1);
  list_push(list, &val2);
  list_remove(list, 1);
  EXPECT_EQ(list_get_size(list), 1);
  EXPECT_EQ(*(int *)list_get_last(list), 10);
  allocator_free(allocator, list);
}

TEST_F(dt_list, clear) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  list_push(list, &val1);
  list_push(list, &val2);
  list_push(list, &val3);
  EXPECT_EQ(list_get_size(list), 3);
  list_clear(list);
  EXPECT_EQ(list_get_size(list), 0);
  allocator_free(allocator, list);
}

TEST_F(dt_list, get_data) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  list_push(list, &val1);
  list_push(list, &val2);
  list_push(list, &val3);
  void **data = list_get_data(list);
  ASSERT_NE(data, nullptr);
  EXPECT_EQ(*(int *)data[0], 10);
  EXPECT_EQ(*(int *)data[1], 20);
  EXPECT_EQ(*(int *)data[2], 30);
  allocator_free(allocator, data);
  allocator_free(allocator, list);
}

TEST_F(dt_list, iteration) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 10, val2 = 20, val3 = 30;
  list_push(list, &val1);
  list_push(list, &val2);
  list_push(list, &val3);
  list_iter_t iter = list_iter_first(list);
  int count = 0;
  int expected[] = {10, 20, 30};
  void *data;
  while ((data = list_iter_next(&iter)) != NULL) {
    EXPECT_EQ(*(int *)data, expected[count++]);
  }
  EXPECT_EQ(count, 3);
  allocator_free(allocator, list);
}

TEST_F(dt_list, iteration_empty_list) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  list_iter_t iter = list_iter_first(list);
  EXPECT_EQ(list_iter_next(&iter), nullptr);
  allocator_free(allocator, list);
}

TEST_F(dt_list, single_element) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int val1 = 42;
  list_push(list, &val1);
  EXPECT_EQ(list_get_size(list), 1);
  EXPECT_EQ(*(int *)list_get_first(list), 42);
  EXPECT_EQ(*(int *)list_get_last(list), 42);
  EXPECT_EQ(*(int *)list_get(list, 0), 42);
  allocator_free(allocator, list);
}

TEST_F(dt_list, mixed_operations) {
  list_t list = (list_t)allocator_create(allocator, &g_list_type, NULL);
  int a = 1, b = 2, c = 3, d = 4, e = 5;
  list_unshift(list, &c);
  list_unshift(list, &b);
  list_unshift(list, &a);
  list_push(list, &d);
  list_push(list, &e);
  EXPECT_EQ(list_get_size(list), 5);
  EXPECT_EQ(*(int *)list_get_first(list), 1);
  EXPECT_EQ(*(int *)list_get_last(list), 5);
  list_pop(list);
  list_shift(list);
  EXPECT_EQ(list_get_size(list), 3);
  EXPECT_EQ(*(int *)list_get_first(list), 2);
  EXPECT_EQ(*(int *)list_get_last(list), 4);
  allocator_free(allocator, list);
}