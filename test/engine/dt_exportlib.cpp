#include "engine/context.h"
#include "engine/symbol.h"
#include "engine/diagnostic.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ===== helpers ===== */

struct compile_result {
  context_t ctx;
  node_t prog;
  vec_t tokens;
};

static struct compile_result compile_source(context_t ctx,
                                            const char *source) {
  allocator_t allocator = ctx->allocator;
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  size_t pos = 0;
  node_t prog = read_program_node(ctx, tokens, &pos, "test.cubec");

  if (!prog || !tokens) {
    GTEST_MESSAGE_AT_(__FILE__, __LINE__,
        "Parsing failed",
        ::testing::TestPartResult::kFatalFailure);
    return (struct compile_result){NULL, prog, tokens};
  }

  source_cache_load(ctx->sources, "test.cubec", source, false);

  context_check_program(ctx, prog);
  return (struct compile_result){ctx, prog, tokens};
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

class dt_exportlib : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== Parsing and valid usage ===== */

TEST_F(dt_exportlib, exportlib_func_basic) {
  const char *src = "exportlib func foo(): void {}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_exportlib, exportlib_var_basic) {
  const char *src = "exportlib var x: i32 = 42;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_exportlib, exportlib_implies_export) {
  /* exportlib symbols should be visible across modules (is_export = true) */
  const char *src = "exportlib func foo(): void {}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  struct symbol *sym = scope_lookup(r.ctx->global_scope, "foo");
  ASSERT_NE(sym, nullptr);
  EXPECT_TRUE(sym->is_export);
  EXPECT_TRUE(sym->is_exportlib);
  compile_result_cleanup(&r, allocator);
}

/* ===== Mutually exclusive modifiers ===== */

TEST_F(dt_exportlib, exportlib_with_export_error) {
  const char *src = "export exportlib func foo(): void {}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_exportlib, exportlib_with_extern_error) {
  const char *src = "exportlib extern func foo(): void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_exportlib, exportlib_with_builtin_error) {
  const char *src = "exportlib builtin func assert(condition: bool): void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_exportlib, exportlib_with_inline_error) {
  const char *src = "exportlib inline func foo(): void {}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Body / initializer requirements ===== */

TEST_F(dt_exportlib, exportlib_func_no_body_error) {
  /* exportlib func must have a body (definition, not declaration) */
  const char *src = "exportlib func foo(): void;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_exportlib, exportlib_var_no_init_error) {
  /* exportlib var must have an initializer */
  const char *src = "exportlib var x: i32;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Not allowed on type declarations ===== */

TEST_F(dt_exportlib, exportlib_struct_error) {
  const char *src = "exportlib struct Foo { x: i32; }\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_exportlib, exportlib_enum_error) {
  const char *src = "exportlib enum Color { Red, Green, Blue }\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}
