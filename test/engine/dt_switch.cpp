#include "engine/checker.h"
#include "engine/diagnostic.h"
#include "engine/symbol.h"
#include "engine/semantic_type.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "core/error.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <string>

using ::testing::Test;

/* ===== helpers ===== */

#define BUILTIN_ASSERT "builtin func assert(condition: bool): void;\n"

struct compile_result {
  checker_t ctx;
  node_t prog;
  vec_t tokens;
};

static struct compile_result compile_source(allocator_t allocator,
                                            const char *source) {
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  size_t pos = 0;
  node_t prog = read_program_node(allocator, tokens, &pos, "test.cubec");

  /* If parsing failed, fail the test immediately */
  if (g_error) {
    std::string err_msg(g_error->message);
    error_clear();
    GTEST_MESSAGE_AT_(__FILE__, __LINE__,
        ("Parsing failed: " + err_msg).c_str(),
        ::testing::TestPartResult::kFatalFailure);
    return (struct compile_result){NULL, prog, tokens};
  }

  checker_t ctx = checker_create(allocator);
  source_cache_load(ctx->sources, "test.cubec", source, false);

  checker_check_program(ctx, prog);
  return (struct compile_result){ctx, prog, tokens};
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  if (r->ctx) checker_dispose(r->ctx);
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

/* ===== test fixture ===== */

class dt_switch : public CubecTest {
protected:
  TEST_ALLOCATOR;
  void TearDown() override {
    error_clear();
    CubecTest::TearDown();
  }
};

/* ===== switch in test block ===== */

TEST_F(dt_switch, switch_in_test_block_basic) {
  const char *src = BUILTIN_ASSERT
      "test \"switch_test\" {\n"
      "    var x: i32 = 1;\n"
      "    switch(x) {\n"
      "        case(1) -> {\n"
      "            assert(true);\n"
      "        }\n"
      "        else -> {\n"
      "            assert(false);\n"
      "        }\n"
      "    }\n"
      "}\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);
  EXPECT_EQ(r.ctx->test_fail_count, 0u);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_switch, switch_in_test_block_multi_case) {
  const char *src = BUILTIN_ASSERT
      "test \"multi\" {\n"
      "    var x: i32 = 2;\n"
      "    switch(x) {\n"
      "        case(1, 2) -> {\n"
      "            assert(true);\n"
      "        }\n"
      "        else -> {\n"
      "            assert(false);\n"
      "        }\n"
      "    }\n"
      "}\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);
  EXPECT_EQ(r.ctx->test_fail_count, 0u);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_switch, switch_in_test_block_no_else) {
  const char *src = BUILTIN_ASSERT
      "test \"no_else\" {\n"
      "    var x: i32 = 5;\n"
      "    switch(x) {\n"
      "        case(1) -> {\n"
      "            assert(false);\n"
      "        }\n"
      "    }\n"
      "}\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);
  /* No match, no else — nothing happens, test should pass */
  EXPECT_EQ(r.ctx->test_fail_count, 0u);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_switch, switch_in_test_block_string) {
  const char *src = BUILTIN_ASSERT
      "test \"str_switch\" {\n"
      "    var s: str = \"hello\";\n"
      "    switch(s) {\n"
      "        case(\"hello\") -> {\n"
      "            assert(true);\n"
      "        }\n"
      "        else -> {\n"
      "            assert(false);\n"
      "        }\n"
      "    }\n"
      "}\n";
  struct compile_result r = compile_source(allocator, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);
  EXPECT_EQ(r.ctx->test_fail_count, 0u);

  compile_result_cleanup(&r, allocator);
}
