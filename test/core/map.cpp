#include "core/map.h"
#include "common/cubec_test.hpp"
#include "core/allocator.h"
#include <gtest/gtest.h>
class test_map : public cubec_test {};
TEST_F(test_map, create) {
  map_t map = create_map(allocator, NULL);
  allocator_free(allocator, map);
}
TEST_F(test_map, set_and_get) {
  map_t map = create_map(allocator, NULL);
  int32_t key = 123;
  int32_t value = 234;
  map_set(map, &key, &value, NULL);
  EXPECT_EQ(*(int32_t *)map_get(map, &key, NULL), 234);
  allocator_free(allocator, map);
}
TEST_F(test_map, replace) {
  map_t map = create_map(allocator, NULL);
  int32_t key = 123;
  int32_t value = 234;
  int32_t value2 = 456;
  map_set(map, &key, &value, NULL);
  EXPECT_EQ(*(int32_t *)map_get(map, &key, NULL), 234);
  map_set(map, &key, &value2, NULL);
  EXPECT_EQ(*(int32_t *)map_get(map, &key, NULL), 456);
  allocator_free(allocator, map);
}
TEST_F(test_map, remove) {
  map_t map = create_map(allocator, NULL);
  int32_t key = 123;
  int32_t value = 234;
  map_set(map, &key, &value, NULL);
  EXPECT_EQ(*(int32_t *)map_get(map, &key, NULL), 234);
  map_delete(map, &key, NULL);
  EXPECT_EQ(map_get(map, &key, NULL), nullptr);
  EXPECT_EQ(map_get_size(map), 0);
  allocator_free(allocator, map);
}
TEST_F(test_map, auto_free) {
  map_initialize_t initialize = {
      .autofree_key = true,
      .autofree_value = true,
  };
  map_t map = create_map(allocator, &initialize);
  map_set(map, allocator_alloc(allocator, sizeof(int32_t), NULL),
          allocator_alloc(allocator, sizeof(int32_t), NULL), NULL);
  allocator_free(allocator, map);
}