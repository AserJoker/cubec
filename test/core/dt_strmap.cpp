#include "core/strmap.h"
#include "core/node.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_strmap : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_strmap, create_and_empty) {
  strmap_t map = (strmap_t)allocator_create(allocator, &g_strmap_class, NULL);
  ASSERT_NE(map, nullptr);
  EXPECT_EQ(strmap_get_size(map), 0);
  EXPECT_EQ(strmap_find(map, "any"), nullptr);
  allocator_free(allocator, &map);
}

TEST_F(dt_strmap, insert_and_find) {
  strmap_t map = (strmap_t)allocator_create(allocator, &g_strmap_class, NULL);
  node_t node1 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_class, NULL);

  strmap_insert(map, "alpha", node1);
  strmap_insert(map, "beta", node2);
  EXPECT_EQ(strmap_get_size(map), 2);

  EXPECT_EQ(strmap_find(map, "alpha"), node1);
  EXPECT_EQ(strmap_find(map, "beta"), node2);
  EXPECT_EQ(strmap_find(map, "gamma"), nullptr);

  allocator_free(allocator, &map);
  allocator_free(allocator, &node1);
  allocator_free(allocator, &node2);
}

TEST_F(dt_strmap, insert_replace) {
  strmap_t map = (strmap_t)allocator_create(allocator, &g_strmap_class, NULL);
  node_t node1 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_class, NULL);

  strmap_insert(map, "key", node1);
  void *old = strmap_insert(map, "key", node2);
  EXPECT_EQ(old, node1);
  EXPECT_EQ(strmap_get_size(map), 1);
  EXPECT_EQ(strmap_find(map, "key"), node2);

  allocator_free(allocator, &map);
  allocator_free(allocator, &node1);
  allocator_free(allocator, &node2);
}

TEST_F(dt_strmap, remove) {
  strmap_t map = (strmap_t)allocator_create(allocator, &g_strmap_class, NULL);
  node_t node1 = (node_t)allocator_create(allocator, &g_node_class, NULL);

  strmap_insert(map, "alpha", node1);
  EXPECT_EQ(strmap_get_size(map), 1);

  void *removed = strmap_remove(map, "alpha");
  EXPECT_EQ(removed, node1);
  EXPECT_EQ(strmap_get_size(map), 0);
  EXPECT_EQ(strmap_find(map, "alpha"), nullptr);

  /* remove non-existent key */
  removed = strmap_remove(map, "nope");
  EXPECT_EQ(removed, nullptr);

  allocator_free(allocator, &map);
  allocator_free(allocator, &node1);
}

TEST_F(dt_strmap, many_insertions) {
  strmap_t map = (strmap_t)allocator_create(allocator, &g_strmap_class, NULL);
  const int N = 100;
  node_t *nodes = (node_t *)allocator_alloc(allocator, sizeof(node_t) * N);

  for (int i = 0; i < N; i++) {
    char key[16];
    snprintf(key, sizeof(key), "key_%03d", i);
    nodes[i] = (node_t)allocator_create(allocator, &g_node_class, NULL);
    strmap_insert(map, key, nodes[i]);
  }
  EXPECT_EQ(strmap_get_size(map), (size_t)N);

  for (int i = 0; i < N; i++) {
    char key[16];
    snprintf(key, sizeof(key), "key_%03d", i);
    EXPECT_EQ(strmap_find(map, key), nodes[i]);
  }

  allocator_free(allocator, &map);
  for (int i = 0; i < N; i++) {
    allocator_free(allocator, &nodes[i]);
  }
  allocator_free(allocator, &nodes);
}

TEST_F(dt_strmap, iterator) {
  strmap_t map = (strmap_t)allocator_create(allocator, &g_strmap_class, NULL);
  node_t n1 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t n2 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t n3 = (node_t)allocator_create(allocator, &g_node_class, NULL);

  strmap_insert(map, "banana", n1);
  strmap_insert(map, "apple", n2);
  strmap_insert(map, "cherry", n3);

  strmap_iter_t iter = strmap_iter_first(map);
  const char *key;
  int count = 0;
  bool found_banana = false, found_apple = false, found_cherry = false;
  while ((key = strmap_iter_next(&iter)) != nullptr) {
    count++;
    if (strcmp(key, "banana") == 0) found_banana = true;
    if (strcmp(key, "apple") == 0) found_apple = true;
    if (strcmp(key, "cherry") == 0) found_cherry = true;
  }
  EXPECT_EQ(count, 3);
  EXPECT_TRUE(found_banana);
  EXPECT_TRUE(found_apple);
  EXPECT_TRUE(found_cherry);

  allocator_free(allocator, &map);
  allocator_free(allocator, &n1);
  allocator_free(allocator, &n2);
  allocator_free(allocator, &n3);
}

TEST_F(dt_strmap, clear) {
  strmap_init_t init = {.value_auto_dispose = true};
  strmap_t map = (strmap_t)allocator_create(allocator, &g_strmap_class, &init);
  node_t n1 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t n2 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  strmap_insert(map, "a", n1);
  strmap_insert(map, "b", n2);
  EXPECT_EQ(strmap_get_size(map), 2);

  strmap_clear(map);
  EXPECT_EQ(strmap_get_size(map), 0);
  EXPECT_EQ(strmap_find(map, "a"), nullptr);

  allocator_free(allocator, &map);
}

TEST_F(dt_strmap, clone) {
  strmap_init_t init = {.value_auto_dispose = true};
  strmap_t map = (strmap_t)allocator_create(allocator, &g_strmap_class, &init);
  node_t n1 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  strmap_insert(map, "key", n1);

  strmap_t copy = (strmap_t)alloc_clone(allocator, map);
  ASSERT_NE(copy, nullptr);
  EXPECT_EQ(strmap_get_size(copy), 1);
  EXPECT_NE(strmap_find(copy, "key"), nullptr);

  allocator_free(allocator, &copy);
  allocator_free(allocator, &map);
}

TEST_F(dt_strmap, move) {
  strmap_init_t init = {.value_auto_dispose = true};
  strmap_t map = (strmap_t)allocator_create(allocator, &g_strmap_class, &init);
  node_t n1 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  strmap_insert(map, "key", n1);

  strmap_t moved = (strmap_t)alloc_move(allocator, map);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(strmap_get_size(moved), 1);
  EXPECT_NE(strmap_find(moved, "key"), nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &map);
}
