#include "engine/source.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_source : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_source, create_and_empty) {
  source_cache_t cache = (source_cache_t)allocator_create(
      allocator, &g_source_cache_type, NULL);
  ASSERT_NE(cache, nullptr);
  EXPECT_EQ(source_cache_find(cache, "nonexistent.cubec"), nullptr);
  allocator_free(allocator, &cache);
}

TEST_F(dt_source, load_and_find) {
  source_cache_t cache = (source_cache_t)allocator_create(
      allocator, &g_source_cache_type, NULL);

  struct source_entry *entry =
      source_cache_load(cache, "test.cubec", "hello\nworld\n", false);
  ASSERT_NE(entry, nullptr);

  /* find returns same entry */
  struct source_entry *found = source_cache_find(cache, "test.cubec");
  EXPECT_EQ(found, entry);

  allocator_free(allocator, &cache);
}

TEST_F(dt_source, get_line) {
  source_cache_t cache = (source_cache_t)allocator_create(
      allocator, &g_source_cache_type, NULL);

  source_cache_load(cache, "test.cubec", "line one\nline two\nline three\n",
                    false);
  struct source_entry *entry = source_cache_find(cache, "test.cubec");
  ASSERT_NE(entry, nullptr);

  EXPECT_EQ(source_entry_get_line_count(entry), 4u); /* 3 lines + trailing newline offset */

  EXPECT_STREQ(source_entry_get_line(entry, 1), "line one");
  EXPECT_STREQ(source_entry_get_line(entry, 2), "line two");
  EXPECT_STREQ(source_entry_get_line(entry, 3), "line three");

  /* out of range */
  EXPECT_STREQ(source_entry_get_line(entry, 0), "");
  EXPECT_STREQ(source_entry_get_line(entry, 99), "");

  allocator_free(allocator, &cache);
}

TEST_F(dt_source, no_trailing_newline) {
  source_cache_t cache = (source_cache_t)allocator_create(
      allocator, &g_source_cache_type, NULL);

  source_cache_load(cache, "test.cubec", "single line", false);
  struct source_entry *entry = source_cache_find(cache, "test.cubec");
  ASSERT_NE(entry, nullptr);

  EXPECT_EQ(source_entry_get_line_count(entry), 1u);
  EXPECT_STREQ(source_entry_get_line(entry, 1), "single line");

  allocator_free(allocator, &cache);
}

TEST_F(dt_source, reload_same_file) {
  source_cache_t cache = (source_cache_t)allocator_create(
      allocator, &g_source_cache_type, NULL);

  struct source_entry *e1 =
      source_cache_load(cache, "test.cubec", "first", false);
  struct source_entry *e2 =
      source_cache_load(cache, "test.cubec", "second", false);

  /* loading same file returns existing entry */
  EXPECT_EQ(e1, e2);
  EXPECT_STREQ(source_entry_get_line(e1, 1), "first");

  allocator_free(allocator, &cache);
}
