#include "engine/checker.h"
#include "engine/checker_evaluate.h"
#include "engine/symbol.h"
#include "cubec/token.h"
#include "cubec/statement.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ===== helpers ===== */

static checker_t parse_and_check(allocator_t allocator, const char *source) {
  vec_t tokens = resolve_token_list(allocator, "test.cubec", source);
  size_t position = 0;
  node_t prog = read_program_node(allocator, tokens, &position, "test.cubec");
  checker_t ctx = checker_create(allocator);
  checker_check_program(ctx, prog);
  allocator_free(allocator, &prog);
  allocator_free(allocator, &tokens);
  return ctx;
}

class dt_using : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ---- using requires __dispose__ on type ---- */

TEST_F(dt_using, type_without_dispose_error) {
  const char *source =
    "struct Item { count: i32; }\n"
    "test \"using_no_dispose\" {\n"
    "  using a:Item = .{};\n"
    "}\n";
  checker_t ctx = parse_and_check(allocator, source);
  EXPECT_GT(ctx->error_count, 0u);
  checker_dispose(ctx);
}

/* ---- using not allowed at module scope ---- */

TEST_F(dt_using, module_scope_error) {
  const char *source =
    "struct Item { count: i32; }\n"
    "using a:Item = .{};\n";
  checker_t ctx = parse_and_check(allocator, source);
  EXPECT_GT(ctx->error_count, 0u);
  checker_dispose(ctx);
}

/* ---- using with undefined is disallowed ---- */

TEST_F(dt_using, undefined_error) {
  const char *source =
    "struct Item { count: i32; }\n"
    "test \"using_undefined\" {\n"
    "  using a:Item = undefined;\n"
    "}\n";
  checker_t ctx = parse_and_check(allocator, source);
  EXPECT_GT(ctx->error_count, 0u);
  checker_dispose(ctx);
}

/* ---- defer captures TDZ variable error ---- */

TEST_F(dt_using, defer_capture_tdz_error) {
  const char *source =
    "builtin func assert(condition:bool):void;\n"
    "test \"defer_tdz\" {\n"
    "  var x:i32 = undefined;\n"
    "  defer |x| {\n"
    "    x = 1;\n"
    "  }\n"
    "  assert(x == 1);\n"
    "}\n";
  checker_t ctx = parse_and_check(allocator, source);
  EXPECT_GT(ctx->error_count, 0u);
  checker_dispose(ctx);
}

/* ---- closure captures TDZ variable error ---- */

TEST_F(dt_using, closure_capture_tdz_error) {
  const char *source =
    "builtin func assert(condition:bool):void;\n"
    "test \"closure_tdz\" {\n"
    "  var x:i32 = undefined;\n"
    "  func |x| testfn():void {\n"
    "    x = x + 1;\n"
    "  }\n"
    "  testfn();\n"
    "  assert(x == 1);\n"
    "}\n";
  checker_t ctx = parse_and_check(allocator, source);
  EXPECT_GT(ctx->error_count, 0u);
  checker_dispose(ctx);
}
