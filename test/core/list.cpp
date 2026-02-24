#include "core/list.h"
#include "common/cubec_test.hpp"
#include "core/allocator.h"
#include <cstddef>
#include <gtest/gtest.h>
class test_list : public cubec_test {};
TEST_F(test_list, create) {
  cubec_list_t list = cubec_create_list(allocator, NULL);
  cubec_allocator_free(allocator, list);
}
TEST_F(test_list, auto_free) {
  cubec_list_initialize_t initialize = {
      .autofree = true,
  };
  cubec_list_t list = cubec_create_list(allocator, &initialize);
  cubec_list_append(list, allocator,
                    cubec_allocator_alloc(allocator, sizeof(int32_t), NULL));
  cubec_allocator_free(allocator, list);
}
TEST_F(test_list, replace) {
  cubec_list_initialize_t initialize = {
      .autofree = true,
  };
  cubec_list_t list = cubec_create_list(allocator, &initialize);
  cubec_list_append(list, allocator,
                    cubec_allocator_alloc(allocator, sizeof(int32_t), NULL));
  cubec_list_node_t it = cubec_list_get_first(list);
  cubec_list_set_data(list, allocator, it,
                      cubec_allocator_alloc(allocator, sizeof(int32_t), NULL));
  EXPECT_EQ(cubec_list_get_size(list), 1);
  cubec_allocator_free(allocator, list);
}
TEST_F(test_list, remove) {
  cubec_list_initialize_t initialize = {
      .autofree = true,
  };
  cubec_list_t list = cubec_create_list(allocator, &initialize);
  cubec_list_append(list, allocator,
                    cubec_allocator_alloc(allocator, sizeof(int32_t), NULL));
  cubec_list_node_t it = cubec_list_get_first(list);
  cubec_list_erase(list, allocator, it);
  EXPECT_EQ(cubec_list_get_size(list), 0);
  cubec_allocator_free(allocator, list);
}
TEST_F(test_list, foreach) {
  cubec_list_t list = cubec_create_list(allocator, NULL);
  int32_t data[] = {
      1, 2, 3, 4, 5,
  };
  for (auto &item : data) {
    cubec_list_append(list, allocator, &item);
  }
  EXPECT_EQ(cubec_list_get_size(list), 5);
  cubec_list_node_t it = cubec_list_get_first(list);
  size_t idx = 0;
  while (it != cubec_list_get_end(list)) {
    int32_t &item = *(int32_t *)cubec_list_node_get(it);
    EXPECT_EQ(item, data[idx++]);
    it = cubec_list_node_next(it);
  }
  cubec_allocator_free(allocator, list);
}

TEST_F(test_list, move) {
  cubec_list_t list = cubec_create_list(allocator, NULL);
  int32_t data = 123;
  cubec_list_append(list, allocator, &data);
  cubec_list_node_t it = cubec_list_get_first(list);
  EXPECT_EQ(cubec_list_node_get(it), &data);
  cubec_list_node_move(it);
  EXPECT_EQ(cubec_list_node_get(it), nullptr);
  cubec_allocator_free(allocator, list);
}