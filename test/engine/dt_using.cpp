#include "engine/context.h"
#include "engine/checker_evaluate.h"
#include "engine/symbol.h"
#include "cubec/token.h"
#include "cubec/statement.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ===== helpers ===== */

static context_t parse_and_check(context_t ctx, const char *source) {
  allocator_t allocator = ctx->allocator;
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  size_t position = 0;
  node_t prog = read_program_node(ctx, tokens, &position, "test.cubec");
  context_check_program(ctx, prog);
  allocator_free(allocator, &prog);
  allocator_free(allocator, &tokens);
  return ctx;
}

class dt_using : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ---- using requires __dispose__ on type ---- */

TEST_F(dt_using, type_without_dispose_error) {
  const char *source =
    "struct Item { count: i32; }\n"
    "test \"using_no_dispose\" {\n"
    "  using a:Item = .{};\n"
    "}\n";
  parse_and_check(ctx, source);
  EXPECT_GT(ctx->error_count, 0u);
}

/* ---- using not allowed at module scope ---- */

TEST_F(dt_using, module_scope_error) {
  const char *source =
    "struct Item { count: i32; }\n"
    "using a:Item = .{};\n";
  parse_and_check(ctx, source);
  EXPECT_GT(ctx->error_count, 0u);
}

/* ---- using with undefined is disallowed ---- */

TEST_F(dt_using, undefined_error) {
  const char *source =
    "struct Item { count: i32; }\n"
    "test \"using_undefined\" {\n"
    "  using a:Item = undefined;\n"
    "}\n";
  parse_and_check(ctx, source);
  EXPECT_GT(ctx->error_count, 0u);
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
  parse_and_check(ctx, source);
  EXPECT_GT(ctx->error_count, 0u);
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
  parse_and_check(ctx, source);
  EXPECT_GT(ctx->error_count, 0u);
}

/* ---- using with __dispose__ — no error ---- */

TEST_F(dt_using, type_with_dispose_ok) {
  const char *source =
    "struct Item { count: i32; func __dispose__(self:*Item):void {} }\n"
    "test \"using_with_dispose\" {\n"
    "  using a:Item = .Item{};\n"
    "}\n";
  parse_and_check(ctx, source);
  EXPECT_EQ(ctx->error_count, 0u);
}

/* ---- using with undefined disallowed even if type has __dispose__ ---- */

TEST_F(dt_using, undefined_with_dispose_still_error) {
  const char *source =
    "struct Item { count: i32; func __dispose__(self:*Item):void {} }\n"
    "test \"using_undefined_dispose\" {\n"
    "  using a:Item = undefined;\n"
    "}\n";
  parse_and_check(ctx, source);
  EXPECT_GT(ctx->error_count, 0u);
}

/* ---- __dispose__ must return void ---- */

TEST_F(dt_using, dispose_non_void_error) {
  const char *source =
    "struct Item { count: i32; func __dispose__(self:*Item):i32 { return 0; } }\n"
    "test \"dispose_non_void\" {\n"
    "  using a:Item = .Item{};\n"
    "}\n";
  parse_and_check(ctx, source);
  EXPECT_GT(ctx->error_count, 0u);
}

/* ---- export using at module scope still errors ---- */

TEST_F(dt_using, export_using_module_scope_error) {
  const char *source =
    "struct Item { count: i32; func __dispose__(self:*Item):void {} }\n"
    "export using a:Item = .Item{};\n";
  parse_and_check(ctx, source);
  EXPECT_GT(ctx->error_count, 0u);
}

/* ---- comptime: using __dispose__ is actually called ---- */

TEST_F(dt_using, comptime_dispose_called) {
  const char *source =
    "builtin func assert(condition:bool):void;\n"
    "struct Item { func __dispose__(self:*Item):void { assert(false); } }\n"
    "test \"dispose_is_called\" {\n"
    "  using a:Item = .Item{};\n"
    "}\n";
  parse_and_check(ctx, source);
  /* __dispose__ calls assert(false) → error_count > 0 proves it ran */
  EXPECT_GT(ctx->error_count, 0u);
}

/* ---- comptime: using with __dispose__ that succeeds ---- */

TEST_F(dt_using, comptime_dispose_success) {
  const char *source =
    "builtin func assert(condition:bool):void;\n"
    "struct Counter { count: i32; func get(self:*Counter):i32 { return self.count; } func __dispose__(self:*Counter):void { self.count = 0; } }\n"
    "test \"using_dispose_ok\" {\n"
    "  using c:Counter = .Counter{.count = 99};\n"
    "  assert(c.get() == 99);\n"
    "}\n";
  parse_and_check(ctx, source);
  EXPECT_EQ(ctx->error_count, 0u);
}

/* ---- comptime: using + defer LIFO order ---- */

TEST_F(dt_using, comptime_using_defer_lifo) {
  /* defer runs after using dispose — LIFO: defer pushed first, dispose pushed after,
   * so dispose runs first, then defer */
  const char *source =
    "builtin func assert(condition:bool):void;\n"
    "struct Item { val: i32; func __dispose__(self:*Item):void { assert(self.val == 10); } }\n"
    "test \"using_defer_lifo\" {\n"
    "  using a:Item = .Item{.val = 10};\n"
    "  defer { assert(false); }\n"
    "}\n";
  /* defer runs first (LIFO), assert(false) → error, proving LIFO order */
  parse_and_check(ctx, source);
  EXPECT_GT(ctx->error_count, 0u);
}
