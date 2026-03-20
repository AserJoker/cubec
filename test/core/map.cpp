#include "core/map.h"
#include "common/cubec_test.hpp"
#include "core/allocator.h"
#include <gtest/gtest.h>
class test_map : public cubec_test {};
TEST_F(test_map, create) {
  cubec_map_t map = cubec_create_map(allocator, NULL);
  cubec_allocator_free(allocator, map);
}
TEST_F(test_map, set_and_get) {
  cubec_map_t map = cubec_create_map(allocator, NULL);
  int32_t key = 123;
  int32_t value = 234;
  cubec_map_set(map, &key, &value, NULL);
  EXPECT_EQ(*(int32_t *)cubec_map_get(map, &key, NULL), 234);
  cubec_allocator_free(allocator, map);
}
TEST_F(test_map, replace) {
  cubec_map_t map = cubec_create_map(allocator, NULL);
  int32_t key = 123;
  int32_t value = 234;
  int32_t value2 = 456;
  cubec_map_set(map, &key, &value, NULL);
  EXPECT_EQ(*(int32_t *)cubec_map_get(map, &key, NULL), 234);
  cubec_map_set(map, &key, &value2, NULL);
  EXPECT_EQ(*(int32_t *)cubec_map_get(map, &key, NULL), 456);
  cubec_allocator_free(allocator, map);
}
TEST_F(test_map, remove) {
  cubec_map_t map = cubec_create_map(allocator, NULL);
  int32_t key = 123;
  int32_t value = 234;
  cubec_map_set(map, &key, &value, NULL);
  EXPECT_EQ(*(int32_t *)cubec_map_get(map, &key, NULL), 234);
  cubec_map_delete(map, &key, NULL);
  EXPECT_EQ(cubec_map_get(map, &key, NULL), nullptr);
  EXPECT_EQ(cubec_map_get_size(map), 0);
  cubec_allocator_free(allocator, map);
}
TEST_F(test_map, auto_free) {
  cubec_map_initialize_t initialize = {
      .autofree_key = true,
      .autofree_value = true,
  };
  cubec_map_t map = cubec_create_map(allocator, &initialize);
  cubec_map_set(map, cubec_allocator_alloc(allocator, sizeof(int32_t), NULL),
                cubec_allocator_alloc(allocator, sizeof(int32_t), NULL), NULL);
  cubec_allocator_free(allocator, map);
}