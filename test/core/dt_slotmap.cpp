#include "core/slotmap.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_slotmap : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);
};

TEST_F(dt_slotmap, insert_and_get) {
  slotmap_t *sm = slotmap_create(allocator);
  int x = 42;
  slot_id_t id = slotmap_insert(sm, &x);
  EXPECT_EQ(slot_id_index(id), 0u);
  EXPECT_EQ(slot_id_version(id), 0u);
  void *ptr = slotmap_get(sm, id);
  EXPECT_EQ(ptr, &x);
  EXPECT_EQ(*(int *)ptr, 42);
  slotmap_dispose(sm);
  delete_allocator(allocator);
}

TEST_F(dt_slotmap, insert_multiple) {
  slotmap_t *sm = slotmap_create(allocator);
  int a = 1, b = 2, c = 3;
  slot_id_t id_a = slotmap_insert(sm, &a);
  slot_id_t id_b = slotmap_insert(sm, &b);
  slot_id_t id_c = slotmap_insert(sm, &c);
  EXPECT_EQ(slot_id_index(id_a), 0u);
  EXPECT_EQ(slot_id_index(id_b), 1u);
  EXPECT_EQ(slot_id_index(id_c), 2u);
  EXPECT_EQ(*(int *)slotmap_get(sm, id_a), 1);
  EXPECT_EQ(*(int *)slotmap_get(sm, id_b), 2);
  EXPECT_EQ(*(int *)slotmap_get(sm, id_c), 3);
  slotmap_dispose(sm);
  delete_allocator(allocator);
}

TEST_F(dt_slotmap, remove_and_version_bump) {
  slotmap_t *sm = slotmap_create(allocator);
  int x = 10;
  slot_id_t id1 = slotmap_insert(sm, &x);
  EXPECT_EQ(slot_id_version(id1), 0u);
  EXPECT_TRUE(slotmap_remove(sm, id1));
  /* old handle is stale */
  EXPECT_EQ(slotmap_get(sm, id1), nullptr);
  /* insert reuses the free slot */
  int y = 20;
  slot_id_t id2 = slotmap_insert(sm, &y);
  EXPECT_EQ(slot_id_index(id2), slot_id_index(id1));
  EXPECT_EQ(slot_id_version(id2), 1u);
  EXPECT_EQ(*(int *)slotmap_get(sm, id2), 20);
  slotmap_dispose(sm);
  delete_allocator(allocator);
}

TEST_F(dt_slotmap, remove_twice_fails) {
  slotmap_t *sm = slotmap_create(allocator);
  int x = 5;
  slot_id_t id = slotmap_insert(sm, &x);
  EXPECT_TRUE(slotmap_remove(sm, id));
  EXPECT_FALSE(slotmap_remove(sm, id));
  slotmap_dispose(sm);
  delete_allocator(allocator);
}

TEST_F(dt_slotmap, get_invalid_index) {
  slotmap_t *sm = slotmap_create(allocator);
  slot_id_t bad = slot_id_make(9999, 0);
  EXPECT_EQ(slotmap_get(sm, bad), nullptr);
  slotmap_dispose(sm);
  delete_allocator(allocator);
}

TEST_F(dt_slotmap, slot_id_make_roundtrip) {
  slot_id_t id = slot_id_make(0x123456789ABC, 0x4321);
  EXPECT_EQ(slot_id_index(id), 0x123456789ABCull);
  EXPECT_EQ(slot_id_version(id), 0x4321u);
}

TEST_F(dt_slotmap, insert_null_ptr) {
  slotmap_t *sm = slotmap_create(allocator);
  slot_id_t id = slotmap_insert(sm, nullptr);
  /* NULL is a valid stored pointer, but get returns NULL */
  EXPECT_EQ(slotmap_get(sm, id), nullptr);
  /* can still remove it */
  EXPECT_TRUE(slotmap_remove(sm, id));
  slotmap_dispose(sm);
  delete_allocator(allocator);
}

TEST_F(dt_slotmap, version_wraps) {
  slotmap_t *sm = slotmap_create(allocator);
  int x = 1;
  slot_id_t id = slotmap_insert(sm, &x);
  /* simulate version at max by directly setting */
  sm->entries[0].version = 65535;
  slot_id_t old_id = slot_id_make(0, 65535);
  EXPECT_TRUE(slotmap_remove(sm, old_id));
  /* version wraps to 0 */
  EXPECT_EQ(sm->entries[0].version, 0u);
  slotmap_dispose(sm);
  delete_allocator(allocator);
}

TEST_F(dt_slotmap, compact_trims_trailing_free) {
  slotmap_t *sm = slotmap_create(allocator);
  int a = 1, b = 2, c = 3;
  slot_id_t id_a = slotmap_insert(sm, &a);
  slot_id_t id_b = slotmap_insert(sm, &b);
  slot_id_t id_c = slotmap_insert(sm, &c);
  EXPECT_EQ(sm->count, 3u);

  /* remove trailing slots */
  slotmap_remove(sm, id_c);
  slotmap_remove(sm, id_b);
  EXPECT_EQ(sm->count, 3u); /* count unchanged after remove */

  uint64_t trimmed = slotmap_compact(sm);
  EXPECT_EQ(trimmed, 2u);
  EXPECT_EQ(sm->count, 1u);
  EXPECT_EQ(sm->free_count, 0u); /* indices 1,2 removed from free_list */

  /* remaining slot still accessible */
  EXPECT_EQ(*(int *)slotmap_get(sm, id_a), 1);
  slotmap_dispose(sm);
  delete_allocator(allocator);
}

TEST_F(dt_slotmap, compact_stops_at_live_entry) {
  slotmap_t *sm = slotmap_create(allocator);
  int a = 1, b = 2, c = 3;
  slot_id_t id_a = slotmap_insert(sm, &a);
  slot_id_t id_b = slotmap_insert(sm, &b);
  slot_id_t id_c = slotmap_insert(sm, &c);

  /* remove only the last one */
  slotmap_remove(sm, id_c);

  uint64_t trimmed = slotmap_compact(sm);
  EXPECT_EQ(trimmed, 1u);
  EXPECT_EQ(sm->count, 2u);

  /* a and b still accessible */
  EXPECT_EQ(*(int *)slotmap_get(sm, id_a), 1);
  EXPECT_EQ(*(int *)slotmap_get(sm, id_b), 2);
  slotmap_dispose(sm);
  delete_allocator(allocator);
}

TEST_F(dt_slotmap, compact_middle_free_not_trimmed) {
  slotmap_t *sm = slotmap_create(allocator);
  int a = 1, b = 2, c = 3;
  slot_id_t id_a = slotmap_insert(sm, &a);
  slot_id_t id_b = slotmap_insert(sm, &b);
  slot_id_t id_c = slotmap_insert(sm, &c);

  /* remove middle slot — not trailing, won't be trimmed */
  slotmap_remove(sm, id_b);

  uint64_t trimmed = slotmap_compact(sm);
  EXPECT_EQ(trimmed, 0u);
  EXPECT_EQ(sm->count, 3u);
  EXPECT_EQ(sm->free_count, 1u); /* index 1 stays in free_list */

  EXPECT_EQ(*(int *)slotmap_get(sm, id_a), 1);
  EXPECT_EQ(*(int *)slotmap_get(sm, id_c), 3);
  slotmap_dispose(sm);
  delete_allocator(allocator);
}

TEST_F(dt_slotmap, compact_empty) {
  slotmap_t *sm = slotmap_create(allocator);
  EXPECT_EQ(slotmap_compact(sm), 0u);
  slotmap_dispose(sm);
  delete_allocator(allocator);
}
