#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <cstring>

using ::testing::Test;

class dt_string : public Test {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_string, create_and_get) {
  string_init_t init = {.str = "Hello"};
  string_t str = (string_t)allocator_create(allocator, &g_string_type, &init);
  ASSERT_NE(str, nullptr);
  EXPECT_STREQ(string_get(str), "Hello");
  EXPECT_EQ(string_get_length(str), 5);
  allocator_free(allocator, str);
}

TEST_F(dt_string, create_empty) {
  string_t str = (string_t)allocator_create(allocator, &g_string_type, NULL);
  ASSERT_NE(str, nullptr);
  EXPECT_STREQ(string_get(str), "");
  EXPECT_EQ(string_get_length(str), 0);
  allocator_free(allocator, str);
}

TEST_F(dt_string, set) {
  string_init_t init = {.str = "Hello"};
  string_t str = (string_t)allocator_create(allocator, &g_string_type, &init);
  ASSERT_NE(str, nullptr);
  string_set(str, "World");
  EXPECT_STREQ(string_get(str), "World");
  EXPECT_EQ(string_get_length(str), 5);
  allocator_free(allocator, str);
}

TEST_F(dt_string, set_empty) {
  string_init_t init = {.str = "Hello"};
  string_t str = (string_t)allocator_create(allocator, &g_string_type, &init);
  ASSERT_NE(str, nullptr);
  string_set(str, "");
  EXPECT_STREQ(string_get(str), "");
  EXPECT_EQ(string_get_length(str), 0);
  allocator_free(allocator, str);
}

TEST_F(dt_string, concat) {
  string_init_t init = {.str = "Hello"};
  string_t str = (string_t)allocator_create(allocator, &g_string_type, &init);
  ASSERT_NE(str, nullptr);
  string_concat(str, " World");
  EXPECT_STREQ(string_get(str), "Hello World");
  EXPECT_EQ(string_get_length(str), 11);
  allocator_free(allocator, str);
}

TEST_F(dt_string, concat_multiple) {
  string_init_t init = {.str = "A"};
  string_t str = (string_t)allocator_create(allocator, &g_string_type, &init);
  ASSERT_NE(str, nullptr);
  string_concat(str, "B");
  string_concat(str, "C");
  string_concat(str, "D");
  EXPECT_STREQ(string_get(str), "ABCD");
  EXPECT_EQ(string_get_length(str), 4);
  allocator_free(allocator, str);
}

TEST_F(dt_string, concat_empty) {
  string_init_t init = {.str = "Hello"};
  string_t str = (string_t)allocator_create(allocator, &g_string_type, &init);
  ASSERT_NE(str, nullptr);
  string_concat(str, "");
  EXPECT_STREQ(string_get(str), "Hello");
  allocator_free(allocator, str);
}

TEST_F(dt_string, long_string) {
  const char *long_str = "This is a very long string that contains many characters and should still work correctly when used with the string module";
  string_init_t init = {.str = long_str};
  string_t str = (string_t)allocator_create(allocator, &g_string_type, &init);
  ASSERT_NE(str, nullptr);
  EXPECT_STREQ(string_get(str), long_str);
  EXPECT_EQ(string_get_length(str), strlen(long_str));
  allocator_free(allocator, str);
}

TEST_F(dt_string, set_long_string) {
  string_t str = (string_t)allocator_create(allocator, &g_string_type, NULL);
  ASSERT_NE(str, nullptr);
  const char *long_str = "This is a very long string that exceeds the initial capacity and should trigger reallocation";
  string_set(str, long_str);
  EXPECT_STREQ(string_get(str), long_str);
  EXPECT_EQ(string_get_length(str), strlen(long_str));
  allocator_free(allocator, str);
}

TEST_F(dt_string, unicode_characters) {
  const char* unicode_str = "你好世界";
  string_init_t init = {.str = unicode_str};
  string_t str = (string_t)allocator_create(allocator, &g_string_type, &init);
  ASSERT_NE(str, nullptr);
  EXPECT_STREQ(string_get(str), unicode_str);
  EXPECT_EQ(string_get_length(str), 12);
  allocator_free(allocator, str);
}