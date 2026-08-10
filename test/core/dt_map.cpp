#include "core/map.h"
#include "core/node.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_map : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_map, create_and_empty) {
  map_t map = (map_t)allocator_create(allocator, &g_map_type, NULL);
  ASSERT_NE(map, nullptr);
  EXPECT_EQ(map_get_size(map), 0);
  allocator_free(allocator, &map);
}

TEST_F(dt_map, insert_and_find) {
  map_init_t map_init = {.key_auto_dispose = true,.value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &map_init);
  node_init_t init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t node1 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t node3 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val1 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val2 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val3 = (node_t)allocator_create(allocator, &g_node_type, &init);

  map_insert(map, node1, val1);
  map_insert(map, node2, val2);
  map_insert(map, node3, val3);
  EXPECT_EQ(map_get_size(map), 3);

  void *found = map_find(map, node1);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, val1);

  found = map_find(map, node2);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, val2);

  found = map_find(map, node3);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, val3);

  allocator_free(allocator, &map);
}

TEST_F(dt_map, find_nonexistent) {
  map_init_t map_init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &map_init);
  node_init_t init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t node = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t nonexistent = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val = (node_t)allocator_create(allocator, &g_node_type, &init);
  map_insert(map, node, val);
  void *found = map_find(map, nonexistent);
  EXPECT_EQ(found, nullptr);
  allocator_free(allocator, &nonexistent);
  allocator_free(allocator, &map);
}

TEST_F(dt_map, insert_duplicate_key) {
  map_init_t map_init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &map_init);
  node_init_t init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t node1 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val1 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val99 = (node_t)allocator_create(allocator, &g_node_type, &init);

  map_insert(map, node1, val1);
  EXPECT_EQ(map_get_size(map), 1);

  map_insert(map, node1, val99);
  EXPECT_EQ(map_get_size(map), 1);

  void *found = map_find(map, node1);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, val99);

  allocator_free(allocator, &map);
}

TEST_F(dt_map, remove) {
  map_init_t map_init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &map_init);
  node_init_t init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t node1 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t node3 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val1 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val2 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val3 = (node_t)allocator_create(allocator, &g_node_type, &init);

  map_insert(map, node1, val1);
  map_insert(map, node2, val2);
  map_insert(map, node3, val3);
  EXPECT_EQ(map_get_size(map), 3);

  /* key_auto_dispose=true: map_remove frees node2; cannot use it afterwards */
  size_t result = map_remove(map, node2);
  EXPECT_EQ(result, 2);
  EXPECT_EQ(map_get_size(map), 2);
  /* Verify remaining entries are still accessible */
  EXPECT_NE(map_find(map, node1), nullptr);
  EXPECT_NE(map_find(map, node3), nullptr);

  allocator_free(allocator, &map);
}

TEST_F(dt_map, remove_nonexistent) {
  map_init_t map_init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &map_init);
  node_init_t init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t node = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t nonexistent = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val = (node_t)allocator_create(allocator, &g_node_type, &init);

  map_insert(map, node, val);
  size_t result = map_remove(map, nonexistent);
  EXPECT_EQ(result, 1);
  EXPECT_EQ(map_get_size(map), 1);
  allocator_free(allocator, &nonexistent);
  allocator_free(allocator, &map);
}

TEST_F(dt_map, clear) {
  map_init_t map_init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &map_init);
  node_init_t init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t node1 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t node3 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val1 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val2 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val3 = (node_t)allocator_create(allocator, &g_node_type, &init);

  map_insert(map, node1, val1);
  map_insert(map, node2, val2);
  map_insert(map, node3, val3);
  EXPECT_EQ(map_get_size(map), 3);

  /* key_auto_dispose=true: map_clear frees all keys; cannot use them afterwards */
  map_clear(map);
  EXPECT_EQ(map_get_size(map), 0);

  allocator_free(allocator, &map);
}

TEST_F(dt_map, iterator_empty) {
  map_t map = (map_t)allocator_create(allocator, &g_map_type, NULL);
  map_iter_t iter = map_iter_first(map);
  void *value = map_iter_next(&iter);
  EXPECT_EQ(value, nullptr);
  allocator_free(allocator, &map);
}

TEST_F(dt_map, iterator_single) {
  map_init_t map_init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &map_init);
  node_init_t init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t node = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val = (node_t)allocator_create(allocator, &g_node_type, &init);
  map_insert(map, node, val);

  map_iter_t iter = map_iter_first(map);
  void *found = map_iter_next(&iter);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, val);

  found = map_iter_next(&iter);
  EXPECT_EQ(found, nullptr);

  allocator_free(allocator, &map);
}

TEST_F(dt_map, iterator_multiple) {
  map_init_t map_init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &map_init);
  node_init_t init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t nodes[5];
  node_t vals[5];
  for (int i = 0; i < 5; i++) {
    nodes[i] = (node_t)allocator_create(allocator, &g_node_type, &init);
    vals[i] = (node_t)allocator_create(allocator, &g_node_type, &init);
  }

  for (int i = 0; i < 5; i++) {
    map_insert(map, nodes[i], vals[i]);
  }

  map_iter_t iter = map_iter_first(map);
  size_t count = 0;
  void *value;
  while ((value = map_iter_next(&iter)) != nullptr) {
    count++;
  }
  EXPECT_EQ(count, 5);

  allocator_free(allocator, &map);
}

TEST_F(dt_map, iterator_after_remove) {
  map_init_t map_init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &map_init);
  node_init_t init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t nodes[3];
  node_t vals[3];
  for (int i = 0; i < 3; i++) {
    nodes[i] = (node_t)allocator_create(allocator, &g_node_type, &init);
    vals[i] = (node_t)allocator_create(allocator, &g_node_type, &init);
  }

  map_insert(map, nodes[0], vals[0]);
  map_insert(map, nodes[1], vals[1]);
  map_insert(map, nodes[2], vals[2]);

  map_remove(map, nodes[1]);

  map_iter_t iter = map_iter_first(map);
  size_t count = 0;
  void *value;
  while ((value = map_iter_next(&iter)) != nullptr) {
    count++;
  }
  EXPECT_EQ(count, 2);

  allocator_free(allocator, &map);
}

TEST_F(dt_map, auto_dispose) {
  allocator_t allocator2 = create_allocator(NULL, NULL);
  map_init_t init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator2, &g_map_type, &init);

  node_init_t node_init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t node1 = (node_t)allocator_create(allocator2, &g_node_type, &node_init);
  node_t node2 = (node_t)allocator_create(allocator2, &g_node_type, &node_init);
  node_t val1 = (node_t)allocator_create(allocator2, &g_node_type, &node_init);
  node_t val2 = (node_t)allocator_create(allocator2, &g_node_type, &node_init);

  map_insert(map, node1, val1);
  map_insert(map, node2, val2);
  EXPECT_EQ(map_get_size(map), 2);

  allocator_free(allocator2, &map);
  delete_allocator(allocator2);
}

TEST_F(dt_map, clone_with_value_clone) {
  map_init_t map_init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &map_init);
  node_init_t init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t node1 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val1 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val2 = (node_t)allocator_create(allocator, &g_node_type, &init);

  map_insert(map, node1, val1);
  map_insert(map, node2, val2);
  EXPECT_EQ(map_get_size(map), 2);

  void *orig_val1 = map_find(map, node1);
  void *orig_val2 = map_find(map, node2);
  ASSERT_NE(orig_val1, nullptr);
  ASSERT_NE(orig_val2, nullptr);

  map_t cloned = (map_t)alloc_clone(allocator, map);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(map_get_size(cloned), 2);

  map_iter_t iter = map_iter_first(cloned);
  size_t count = 0;
  while (map_iter_next(&iter) != nullptr) {
    count++;
  }
  EXPECT_EQ(count, 2);

  allocator_free(allocator, &map);
  allocator_free(allocator, &cloned);
}

TEST_F(dt_map, move_with_value_move) {
  map_init_t map_init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &map_init);
  node_init_t init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t node1 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val1 = (node_t)allocator_create(allocator, &g_node_type, &init);
  node_t val2 = (node_t)allocator_create(allocator, &g_node_type, &init);

  map_insert(map, node1, val1);
  map_insert(map, node2, val2);
  EXPECT_EQ(map_get_size(map), 2);

  void *orig_val1 = map_find(map, node1);
  void *orig_val2 = map_find(map, node2);

  map_t moved = (map_t)alloc_move(allocator, map);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(map_get_size(moved), 2);
  EXPECT_EQ(map_get_size(map), 0);

  void *moved_val1 = map_find(moved, node1);
  void *moved_val2 = map_find(moved, node2);
  EXPECT_EQ(moved_val1, orig_val1);
  EXPECT_EQ(moved_val2, orig_val2);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &map);
}

TEST_F(dt_map, threshold_list_mode) {
  map_init_t init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &init);

  node_init_t node_init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t nodes[15];
  node_t vals[15];
  for (int i = 0; i < 15; i++) {
    nodes[i] = (node_t)allocator_create(allocator, &g_node_type, &node_init);
    vals[i] = (node_t)allocator_create(allocator, &g_node_type, &node_init);
    map_insert(map, nodes[i], vals[i]);
  }

  EXPECT_EQ(map_get_size(map), 15);
  for (int i = 0; i < 15; i++) {
    void *found = map_find(map, nodes[i]);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found, vals[i]);
  }

  allocator_free(allocator, &map);
}

TEST_F(dt_map, threshold_convert_to_rbtree) {
  map_init_t init = {.key_auto_dispose = true, .value_auto_dispose = true};
  map_t map = (map_t)allocator_create(allocator, &g_map_type, &init);

  node_init_t node_init = {.kind = 1, .location = {0}, .parent = NULL};
  node_t nodes[20];
  node_t vals[20];
  for (int i = 0; i < 20; i++) {
    nodes[i] = (node_t)allocator_create(allocator, &g_node_type, &node_init);
    vals[i] = (node_t)allocator_create(allocator, &g_node_type, &node_init);
    map_insert(map, nodes[i], vals[i]);
  }

  EXPECT_EQ(map_get_size(map), 20);
  for (int i = 0; i < 20; i++) {
    void *found = map_find(map, nodes[i]);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found, vals[i]);
  }

  allocator_free(allocator, &map);
}