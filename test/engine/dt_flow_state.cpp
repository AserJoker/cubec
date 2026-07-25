#include "engine/flow_state.h"
#include "core/allocator.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

class dt_flow_state : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_flow_state, create_dispose) {
  flow_state_t fs = flow_state_create(allocator);
  ASSERT_NE(fs, nullptr);
  EXPECT_EQ(fs->term, FLOW_ALIVE);
  EXPECT_EQ(vec_get_size(fs->tdz_set), 0u);
  flow_state_dispose(fs, allocator);
}

TEST_F(dt_flow_state, mark_returned) {
  flow_state_t fs = flow_state_create(allocator);
  flow_state_mark_returned(fs);
  EXPECT_TRUE(flow_state_is_unreachable(fs));
  EXPECT_TRUE(flow_state_is_all_returned(fs));
  flow_state_dispose(fs, allocator);
}

TEST_F(dt_flow_state, mark_broke) {
  flow_state_t fs = flow_state_create(allocator);
  flow_state_mark_broke(fs);
  EXPECT_TRUE(flow_state_is_unreachable(fs));
  EXPECT_FALSE(flow_state_is_all_returned(fs));
  flow_state_dispose(fs, allocator);
}

TEST_F(dt_flow_state, mark_continued) {
  flow_state_t fs = flow_state_create(allocator);
  flow_state_mark_continued(fs);
  EXPECT_TRUE(flow_state_is_unreachable(fs));
  EXPECT_FALSE(flow_state_is_all_returned(fs));
  flow_state_dispose(fs, allocator);
}

TEST_F(dt_flow_state, tdz_add_remove) {
  flow_state_t fs = flow_state_create(allocator);
  flow_state_add_tdz(fs, "x");
  flow_state_add_tdz(fs, "y");
  EXPECT_TRUE(flow_state_is_tdz(fs, "x"));
  EXPECT_TRUE(flow_state_is_tdz(fs, "y"));
  EXPECT_FALSE(flow_state_is_tdz(fs, "z"));
  flow_state_remove_tdz(fs, "x");
  EXPECT_FALSE(flow_state_is_tdz(fs, "x"));
  EXPECT_TRUE(flow_state_is_tdz(fs, "y"));
  flow_state_dispose(fs, allocator);
}

TEST_F(dt_flow_state, merge_both_alive) {
  flow_state_t a = flow_state_create(allocator);
  flow_state_t b = flow_state_create(allocator);
  flow_state_t merged = flow_state_merge(allocator, a, b);
  EXPECT_EQ(merged->term, FLOW_ALIVE);
  EXPECT_FALSE(flow_state_is_unreachable(merged));
  flow_state_dispose(a, allocator);
  flow_state_dispose(b, allocator);
  flow_state_dispose(merged, allocator);
}

TEST_F(dt_flow_state, merge_both_returned) {
  flow_state_t a = flow_state_create(allocator);
  flow_state_mark_returned(a);
  flow_state_t b = flow_state_create(allocator);
  flow_state_mark_returned(b);
  flow_state_t merged = flow_state_merge(allocator, a, b);
  EXPECT_EQ(merged->term, FLOW_RETURNED);
  EXPECT_TRUE(flow_state_is_all_returned(merged));
  flow_state_dispose(a, allocator);
  flow_state_dispose(b, allocator);
  flow_state_dispose(merged, allocator);
}

TEST_F(dt_flow_state, merge_one_returned) {
  flow_state_t a = flow_state_create(allocator);
  flow_state_t b = flow_state_create(allocator);
  flow_state_mark_returned(b);
  flow_state_t merged = flow_state_merge(allocator, a, b);
  EXPECT_EQ(merged->term, FLOW_ALIVE);
  EXPECT_FALSE(flow_state_is_all_returned(merged));
  flow_state_dispose(a, allocator);
  flow_state_dispose(b, allocator);
  flow_state_dispose(merged, allocator);
}

TEST_F(dt_flow_state, merge_tdz_union) {
  flow_state_t a = flow_state_create(allocator);
  flow_state_add_tdz(a, "x");
  flow_state_add_tdz(a, "y");
  flow_state_t b = flow_state_create(allocator);
  flow_state_add_tdz(b, "y");
  flow_state_add_tdz(b, "z");
  flow_state_t merged = flow_state_merge(allocator, a, b);
  EXPECT_TRUE(flow_state_is_tdz(merged, "x"));
  EXPECT_TRUE(flow_state_is_tdz(merged, "y"));
  EXPECT_TRUE(flow_state_is_tdz(merged, "z"));
  flow_state_dispose(a, allocator);
  flow_state_dispose(b, allocator);
  flow_state_dispose(merged, allocator);
}

TEST_F(dt_flow_state, merge_tdz_both_assigned) {
  /* x is TDZ in neither branch => not TDZ after merge */
  flow_state_t a = flow_state_create(allocator);
  flow_state_add_tdz(a, "y");
  flow_state_t b = flow_state_create(allocator);
  flow_state_add_tdz(b, "z");
  flow_state_t merged = flow_state_merge(allocator, a, b);
  EXPECT_FALSE(flow_state_is_tdz(merged, "x"));
  flow_state_dispose(a, allocator);
  flow_state_dispose(b, allocator);
  flow_state_dispose(merged, allocator);
}

TEST_F(dt_flow_state, merge_tdz_one_assigned) {
  /* x is TDZ in a but not in b => still TDZ after merge */
  flow_state_t a = flow_state_create(allocator);
  flow_state_add_tdz(a, "x");
  flow_state_t b = flow_state_create(allocator);
  /* b: x was assigned (not in tdz_set) */
  flow_state_t merged = flow_state_merge(allocator, a, b);
  EXPECT_TRUE(flow_state_is_tdz(merged, "x"));
  flow_state_dispose(a, allocator);
  flow_state_dispose(b, allocator);
  flow_state_dispose(merged, allocator);
}
