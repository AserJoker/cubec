#include "core/list.h"
#include "common/cubec_test.hpp"
#include "core/allocator.h"
#include <cstddef>
#include <gtest/gtest.h>
class test_list : public cubec_test {};
TEST_F(test_list, create) {
  list_t list = create_list(allocator, NULL);
  allocator_free(allocator, list);
}
TEST_F(test_list, auto_free) {
  list_initialize_t initialize = {
      .autofree = true,
      .compare = NULL,
  };
  list_t list = create_list(allocator, &initialize);
  list_append(list, allocator_alloc(allocator, sizeof(int32_t), NULL));
  allocator_free(allocator, list);
}
TEST_F(test_list, replace) {
  list_initialize_t initialize = {
      .autofree = true,
      .compare = NULL,
  };
  list_t list = create_list(allocator, &initialize);
  list_append(list, allocator_alloc(allocator, sizeof(int32_t), NULL));
  list_node_t it = list_get_first(list);
  list_set_data(list, it, allocator_alloc(allocator, sizeof(int32_t), NULL));
  EXPECT_EQ(list_get_size(list), 1);
  allocator_free(allocator, list);
}
TEST_F(test_list, remove) {
  list_initialize_t initialize = {
      .autofree = true,
      .compare = NULL,
  };
  list_t list = create_list(allocator, &initialize);
  list_append(list, allocator_alloc(allocator, sizeof(int32_t), NULL));
  list_node_t it = list_get_first(list);
  list_erase(list, it);
  EXPECT_EQ(list_get_size(list), 0);
  allocator_free(allocator, list);
}
TEST_F(test_list, foreach) {
  list_t list = create_list(allocator, NULL);
  int32_t data[] = {
      1, 2, 3, 4, 5,
  };
  for (auto &item : data) {
    list_append(list, &item);
  }
  EXPECT_EQ(list_get_size(list), 5);
  list_node_t it = list_get_first(list);
  size_t idx = 0;
  while (it != list_get_end(list)) {
    int32_t &item = *(int32_t *)list_node_get(it);
    EXPECT_EQ(item, data[idx++]);
    it = list_node_next(it);
  }
  allocator_free(allocator, list);
}

TEST_F(test_list, move) {
  list_t list = create_list(allocator, NULL);
  int32_t data = 123;
  list_append(list, &data);
  list_node_t it = list_get_first(list);
  EXPECT_EQ(list_node_get(it), &data);
  list_node_move(it);
  EXPECT_EQ(list_node_get(it), nullptr);
  allocator_free(allocator, list);
}