#include "core/location.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_location : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* Helper: build a location_t from a C string. */
static location_t make_location(const char *s) {
  location_t loc;
  loc.filename = "test.cubec";
  loc.begin.offset = s;
  loc.begin.line = 1;
  loc.begin.column = 1;
  loc.end.offset = s + strlen(s);
  loc.end.line = 1;
  loc.end.column = strlen(s) + 1;
  return loc;
}

/* ============================================================================
 *  location_is
 * ============================================================================ */

TEST_F(dt_location, exact_match) {
  location_t loc = make_location("hello");
  EXPECT_TRUE(location_is(&loc, "hello"));
}

TEST_F(dt_location, mismatch_same_length) {
  location_t loc = make_location("abc");
  EXPECT_FALSE(location_is(&loc, "xyz"));
}

TEST_F(dt_location, short_vs_long) {
  /* Regression: before the fix, "b" would match "break" because
   * strncmp("b", "break", 1) == 0 without checking str[1] == '\0'. */
  location_t loc = make_location("b");
  EXPECT_FALSE(location_is(&loc, "break"));
}

TEST_F(dt_location, long_vs_short) {
  location_t loc = make_location("break");
  EXPECT_FALSE(location_is(&loc, "b"));
}

TEST_F(dt_location, prefix_partial_match) {
  /* "bre" is a prefix of "break", but lengths differ */
  location_t loc = make_location("bre");
  EXPECT_FALSE(location_is(&loc, "break"));
}

TEST_F(dt_location, many_short_keywords) {
  /* Verify single-char or short identifiers don't match common keywords */
  const char *keywords[] = {"break", "return", "while", "if", "else",
                             "continue", "switch", "struct", "enum", NULL};
  location_t loc = make_location("b");
  for (int i = 0; keywords[i]; i++) {
    EXPECT_FALSE(location_is(&loc, keywords[i]))
        << "single-char 'b' should not match keyword '" << keywords[i] << "'";
  }
}

TEST_F(dt_location, empty_location_vs_empty_string) {
  location_t loc = make_location("");
  EXPECT_TRUE(location_is(&loc, ""));
}

TEST_F(dt_location, empty_location_vs_non_empty) {
  location_t loc = make_location("");
  EXPECT_FALSE(location_is(&loc, "hello"));
}

TEST_F(dt_location, non_empty_vs_empty_string) {
  location_t loc = make_location("hello");
  EXPECT_FALSE(location_is(&loc, ""));
}

TEST_F(dt_location, case_sensitivity) {
  location_t loc = make_location("Hello");
  EXPECT_FALSE(location_is(&loc, "hello"));
  EXPECT_FALSE(location_is(&loc, "HELLO"));
}

/* ============================================================================
 *  location_get
 * ============================================================================ */

TEST_F(dt_location, get_extracts_text) {
  const char *source = "hello world";
  location_t loc;
  loc.filename = "test.cubec";
  loc.begin.offset = source;
  loc.begin.line = 1;
  loc.begin.column = 1;
  loc.end.offset = source + 5;
  loc.end.line = 1;
  loc.end.column = 6;

  char *text = location_get(&loc, allocator);
  EXPECT_STREQ(text, "hello");
  allocator_free(allocator, &text);
}

TEST_F(dt_location, get_empty_location) {
  location_t loc = make_location("");
  char *text = location_get(&loc, allocator);
  EXPECT_STREQ(text, "");
  allocator_free(allocator, &text);
}
