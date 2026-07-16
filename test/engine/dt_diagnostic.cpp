#include "engine/diagnostic.h"
#include "engine/source.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_diagnostic : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_diagnostic, create_and_empty) {
  diagnostic_list_t list = (diagnostic_list_t)allocator_create(
      allocator, &g_diagnostic_list_type, NULL);
  ASSERT_NE(list, nullptr);
  EXPECT_EQ(diagnostic_list_get_size(list), 0u);
  EXPECT_EQ(diagnostic_list_get_error_count(list), 0u);
  allocator_free(allocator, &list);
}

TEST_F(dt_diagnostic, push_error) {
  diagnostic_list_t list = (diagnostic_list_t)allocator_create(
      allocator, &g_diagnostic_list_type, NULL);

  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 5, NULL},
                    .end = {1, 10, NULL}};
  diagnostic_list_push(list, DIAGNOSTIC_ERROR, loc, "type mismatch");

  EXPECT_EQ(diagnostic_list_get_size(list), 1u);
  EXPECT_EQ(diagnostic_list_get_error_count(list), 1u);

  allocator_free(allocator, &list);
}

TEST_F(dt_diagnostic, push_warning_and_note) {
  diagnostic_list_t list = (diagnostic_list_t)allocator_create(
      allocator, &g_diagnostic_list_type, NULL);

  location_t loc = {.filename = "test.cubec",
                    .begin = {2, 1, NULL},
                    .end = {2, 5, NULL}};
  diagnostic_list_push(list, DIAGNOSTIC_WARNING, loc, "unused variable");

  location_t note_loc = {.filename = "test.cubec",
                         .begin = {2, 1, NULL},
                         .end = {2, 5, NULL}};
  diagnostic_list_push_note(list, note_loc, "declared here");

  EXPECT_EQ(diagnostic_list_get_size(list), 1u);
  EXPECT_EQ(diagnostic_list_get_error_count(list), 0u);

  allocator_free(allocator, &list);
}

TEST_F(dt_diagnostic, clear) {
  diagnostic_list_t list = (diagnostic_list_t)allocator_create(
      allocator, &g_diagnostic_list_type, NULL);

  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 1, NULL},
                    .end = {1, 2, NULL}};
  diagnostic_list_push(list, DIAGNOSTIC_ERROR, loc, "err1");
  diagnostic_list_push(list, DIAGNOSTIC_ERROR, loc, "err2");

  EXPECT_EQ(diagnostic_list_get_size(list), 2u);
  EXPECT_EQ(diagnostic_list_get_error_count(list), 2u);

  diagnostic_list_clear(list);
  EXPECT_EQ(diagnostic_list_get_size(list), 0u);
  EXPECT_EQ(diagnostic_list_get_error_count(list), 0u);

  allocator_free(allocator, &list);
}

TEST_F(dt_diagnostic, emit_with_source) {
  diagnostic_list_t list = (diagnostic_list_t)allocator_create(
      allocator, &g_diagnostic_list_type, NULL);
  source_cache_t cache = (source_cache_t)allocator_create(
      allocator, &g_source_cache_type, NULL);

  /* load source */
  source_cache_load(cache, "test.cubec", "var x: i32 = \"hello\"\nvar y = 42\n",
                    false);

  /* push diagnostic */
  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 14, NULL},
                    .end = {1, 21, NULL}};
  diagnostic_list_push(list, DIAGNOSTIC_ERROR, loc,
                       "type mismatch: expected i32, found string");

  /* emit to a temp file */
  const char *tmp_path = "cubec_test_diag_tmp.txt";
  FILE *tmp = fopen(tmp_path, "w");
  ASSERT_NE(tmp, nullptr);

  diagnostic_list_init_t init = {.output = tmp};
  allocator_free(allocator, &list);
  list = (diagnostic_list_t)allocator_create(allocator, &g_diagnostic_list_type,
                                             &init);
  diagnostic_list_push(list, DIAGNOSTIC_ERROR, loc,
                       "type mismatch: expected i32, found string");
  diagnostic_list_emit(list, cache);
  fclose(tmp);

  /* read back and verify */
  tmp = fopen(tmp_path, "r");
  ASSERT_NE(tmp, nullptr);
  char buf[4096];
  size_t n = fread(buf, 1, sizeof(buf) - 1, tmp);
  buf[n] = '\0';
  fclose(tmp);

  EXPECT_NE(strstr(buf, "error:"), nullptr);
  EXPECT_NE(strstr(buf, "type mismatch"), nullptr);
  EXPECT_NE(strstr(buf, "test.cubec:1:14"), nullptr);
  EXPECT_NE(strstr(buf, "^"), nullptr);

  remove(tmp_path);

  allocator_free(allocator, &list);
  allocator_free(allocator, &cache);
}
