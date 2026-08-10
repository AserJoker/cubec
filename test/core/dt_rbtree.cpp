#include "core/rbtree.h"
#include "core/node.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_rbtree : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_rbtree, create_and_empty) {
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, NULL);
  ASSERT_NE(tree, nullptr);
  EXPECT_EQ(rbtree_get_size(tree), 0);
  allocator_free(allocator, &tree);
}

TEST_F(dt_rbtree, insert_and_find) {
  rbtree_init_t init = {.auto_dispose = true};
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, &init);
  node_t node1 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t node3 = (node_t)allocator_create(allocator, &g_node_class, NULL);

  rbtree_insert(tree, alloc_get_id(node1), node1);
  rbtree_insert(tree, alloc_get_id(node2), node2);
  rbtree_insert(tree, alloc_get_id(node3), node3);
  EXPECT_EQ(rbtree_get_size(tree), 3);

  void *found = rbtree_find(tree, alloc_get_id(node1));
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, node1);

  found = rbtree_find(tree, alloc_get_id(node2));
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, node2);

  found = rbtree_find(tree, alloc_get_id(node3));
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, node3);

  allocator_free(allocator, &tree);
}

TEST_F(dt_rbtree, find_nonexistent) {
  rbtree_init_t init = {.auto_dispose = true};
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, &init);
  node_t node = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t nonexistent = (node_t)allocator_create(allocator, &g_node_class, NULL);
  rbtree_insert(tree, alloc_get_id(node), node);
  void *found = rbtree_find(tree, alloc_get_id(nonexistent));
  EXPECT_EQ(found, nullptr);
  allocator_free(allocator, &nonexistent);
  allocator_free(allocator, &tree);
}

TEST_F(dt_rbtree, insert_duplicate_key) {
  rbtree_init_t init = {.auto_dispose = true};
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, &init);
  node_t node1 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_class, NULL);

  rbtree_insert(tree, alloc_get_id(node1), node1);
  EXPECT_EQ(rbtree_get_size(tree), 1);

  rbtree_insert(tree, alloc_get_id(node1), node1);
  EXPECT_EQ(rbtree_get_size(tree), 1);

  void *found = rbtree_find(tree, alloc_get_id(node1));
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, node1);

  (void)node2;
  allocator_free(allocator, &node2);
  allocator_free(allocator, &tree);
}

TEST_F(dt_rbtree, Remove) {
  rbtree_init_t init = {.auto_dispose = true};
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, &init);
  node_t node1 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t node3 = (node_t)allocator_create(allocator, &g_node_class, NULL);

  rbtree_insert(tree, alloc_get_id(node1), node1);
  rbtree_insert(tree, alloc_get_id(node2), node2);
  rbtree_insert(tree, alloc_get_id(node3), node3);
  EXPECT_EQ(rbtree_get_size(tree), 3);

  /* auto_dispose=true: rbtree_remove frees node2; save its id before remove */
  uint64_t node2_id = alloc_get_id(node2);
  size_t result = rbtree_remove(tree, node2_id);
  EXPECT_EQ(result, 2);
  EXPECT_EQ(rbtree_get_size(tree), 2);
  EXPECT_EQ(rbtree_find(tree, node2_id), nullptr);
  /* Verify remaining entries still accessible */
  EXPECT_NE(rbtree_find(tree, alloc_get_id(node1)), nullptr);
  EXPECT_NE(rbtree_find(tree, alloc_get_id(node3)), nullptr);

  allocator_free(allocator, &tree);
}

TEST_F(dt_rbtree, remove_nonexistent) {
  rbtree_init_t init = {.auto_dispose = true};
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, &init);
  node_t node = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t nonexistent = (node_t)allocator_create(allocator, &g_node_class, NULL);

  rbtree_insert(tree, alloc_get_id(node), node);
  size_t result = rbtree_remove(tree, alloc_get_id(nonexistent));
  EXPECT_EQ(result, 1);
  EXPECT_EQ(rbtree_get_size(tree), 1);
  allocator_free(allocator, &nonexistent);
  allocator_free(allocator, &tree);
}

TEST_F(dt_rbtree, Clear) {
  rbtree_init_t init = {.auto_dispose = true};
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, &init);
  node_t node1 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t node3 = (node_t)allocator_create(allocator, &g_node_class, NULL);

  rbtree_insert(tree, alloc_get_id(node1), node1);
  rbtree_insert(tree, alloc_get_id(node2), node2);
  rbtree_insert(tree, alloc_get_id(node3), node3);
  EXPECT_EQ(rbtree_get_size(tree), 3);

  /* auto_dispose=true: rbtree_clear frees all nodes; cannot use them afterwards */
  rbtree_clear(tree);
  EXPECT_EQ(rbtree_get_size(tree), 0);

  allocator_free(allocator, &tree);
}

TEST_F(dt_rbtree, iterator_empty) {
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, NULL);
  rbtree_iter_t iter = rbtree_iter_first(tree);
  void *node = rbtree_iter_next(&iter);
  EXPECT_EQ(node, nullptr);
  allocator_free(allocator, &tree);
}

TEST_F(dt_rbtree, iterator_single) {
  rbtree_init_t init = {.auto_dispose = true};
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, &init);
  node_t node = (node_t)allocator_create(allocator, &g_node_class, NULL);
  rbtree_insert(tree, alloc_get_id(node), node);

  rbtree_iter_t iter = rbtree_iter_first(tree);
  void *found = rbtree_iter_next(&iter);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found, node);

  found = rbtree_iter_next(&iter);
  EXPECT_EQ(found, nullptr);

  allocator_free(allocator, &tree);
}

TEST_F(dt_rbtree, iterator_inorder) {
  rbtree_init_t init = {.auto_dispose = true};
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, &init);
  node_t nodes[5];
  for (int i = 0; i < 5; i++) {
    nodes[i] = (node_t)allocator_create(allocator, &g_node_class, NULL);
  }

  // Insert in random order
  rbtree_insert(tree, alloc_get_id(nodes[2]), nodes[2]);
  rbtree_insert(tree, alloc_get_id(nodes[0]), nodes[0]);
  rbtree_insert(tree, alloc_get_id(nodes[4]), nodes[4]);
  rbtree_insert(tree, alloc_get_id(nodes[1]), nodes[1]);
  rbtree_insert(tree, alloc_get_id(nodes[3]), nodes[3]);

  rbtree_iter_t iter = rbtree_iter_first(tree);
  size_t count = 0;
  void *found;
  while ((found = rbtree_iter_next(&iter)) != nullptr) {
    ASSERT_LT(count, 5);
    EXPECT_EQ(found, nodes[count]);
    count++;
  }
  EXPECT_EQ(count, 5);

  allocator_free(allocator, &tree);
}

TEST_F(dt_rbtree, iterator_after_remove) {
  rbtree_init_t init = {.auto_dispose = true};
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, &init);
  node_t nodes[3];
  for (int i = 0; i < 3; i++) {
    nodes[i] = (node_t)allocator_create(allocator, &g_node_class, NULL);
  }

  rbtree_insert(tree, alloc_get_id(nodes[0]), nodes[0]);
  rbtree_insert(tree, alloc_get_id(nodes[1]), nodes[1]);
  rbtree_insert(tree, alloc_get_id(nodes[2]), nodes[2]);

  rbtree_remove(tree, alloc_get_id(nodes[1]));

  rbtree_iter_t iter = rbtree_iter_first(tree);
  size_t count = 0;
  void *found;
  while ((found = rbtree_iter_next(&iter)) != nullptr) {
    ASSERT_LT(count, 2);
    EXPECT_EQ(found, nodes[count == 0 ? 0 : 2]);
    count++;
  }
  EXPECT_EQ(count, 2);

  allocator_free(allocator, &tree);
}

TEST_F(dt_rbtree, auto_dispose) {
  allocator_t allocator2 = create_allocator(NULL, NULL);
  rbtree_init_t init = {.auto_dispose = true};
  rbtree_t tree = (rbtree_t)allocator_create(allocator2, &g_rbtree_class, &init);

  node_t node1 = (node_t)allocator_create(allocator2, &g_node_class, NULL);
  node_t node2 = (node_t)allocator_create(allocator2, &g_node_class, NULL);

  rbtree_insert(tree, alloc_get_id(node1), node1);
  rbtree_insert(tree, alloc_get_id(node2), node2);
  EXPECT_EQ(rbtree_get_size(tree), 2);

  allocator_free(allocator2, &tree);
  delete_allocator(allocator2);
}

TEST_F(dt_rbtree, clone_with_value_clone) {
  rbtree_init_t init = {.auto_dispose = true};
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, &init);
  node_t node1 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_class, NULL);

  rbtree_insert(tree, alloc_get_id(node1), node1);
  rbtree_insert(tree, alloc_get_id(node2), node2);
  EXPECT_EQ(rbtree_get_size(tree), 2);

  rbtree_t cloned = (rbtree_t)alloc_clone(allocator, tree);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(rbtree_get_size(cloned), 2);

  rbtree_iter_t iter = rbtree_iter_first(cloned);
  size_t count = 0;
  while (rbtree_iter_next(&iter) != nullptr) {
    count++;
  }
  EXPECT_EQ(count, 2);

  allocator_free(allocator, &tree);
  allocator_free(allocator, &cloned);
}

TEST_F(dt_rbtree, move_with_value_move) {
  rbtree_init_t init = {.auto_dispose = true};
  rbtree_t tree = (rbtree_t)allocator_create(allocator, &g_rbtree_class, &init);
  node_t node1 = (node_t)allocator_create(allocator, &g_node_class, NULL);
  node_t node2 = (node_t)allocator_create(allocator, &g_node_class, NULL);

  rbtree_insert(tree, alloc_get_id(node1), node1);
  rbtree_insert(tree, alloc_get_id(node2), node2);
  EXPECT_EQ(rbtree_get_size(tree), 2);

  rbtree_t moved = (rbtree_t)alloc_move(allocator, tree);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(rbtree_get_size(moved), 2);
  EXPECT_EQ(rbtree_get_size(tree), 0);

  rbtree_iter_t iter = rbtree_iter_first(moved);
  size_t count = 0;
  while (rbtree_iter_next(&iter) != nullptr) {
    count++;
  }
  EXPECT_EQ(count, 2);

  // moved 获得了 tree 的所有权（包括 auto_dispose），需要释放
  allocator_free(allocator, &moved);
  // tree 变成空壳后必须显式释放，否则内存泄漏
  allocator_free(allocator, &tree);
}
