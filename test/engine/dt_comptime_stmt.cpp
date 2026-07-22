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

class dt_comptime_stmt : public CubecTest {
protected:
  TEST_ALLOCATOR;
  void TearDown() override {
    error_clear();
    CubecTest::TearDown();
  }
};

/* ===== comptime var ===== */

TEST_F(dt_comptime_stmt, comptime_var) {
  const char *src =
    "comptime var x = 1 + 2;\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, comptime_var_bool) {
  const char *src =
    "comptime var flag = true;\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, comptime_var_string) {
  const char *src =
    "comptime var name = \"hello\";\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, comptime_var_undefined_error) {
  /* Global comptime var cannot be initialized with undefined */
  const char *src =
    "comptime var x: i32 = undefined;\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_stmt, comptime_var_no_init_error) {
  /* Global comptime var must have an initializer — enforced at parse time */
  const char *src =
    "comptime var x: i32;\n";
  vec_t tokens = resolve_token_list(allocator, "test.cubec", src);
  size_t pos = 0;
  node_t prog = read_program_node(allocator, tokens, &pos, "test.cubec");
  /* Parse-time error: g_error should be set */
  EXPECT_NE(g_error, nullptr);
  error_clear();
  allocator_free(allocator, &prog);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_comptime_stmt, comptime_var_implicit_const) {
  /* Global comptime var is implicitly const — type auto-wrapped to const */
  const char *src =
    "comptime var x = 42;\n"
    "comptime var y: i32 = 10;\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  /* Verify the types are const */
  struct symbol *x_sym = scope_lookup_local(r.ctx->global_scope, "x");
  ASSERT_NE(x_sym, nullptr);
  EXPECT_TRUE(semantic_type_is_const(x_sym->variable.type));
  struct symbol *y_sym = scope_lookup_local(r.ctx->global_scope, "y");
  ASSERT_NE(y_sym, nullptr);
  EXPECT_TRUE(semantic_type_is_const(y_sym->variable.type));
  compile_result_cleanup(&r, allocator);
}
