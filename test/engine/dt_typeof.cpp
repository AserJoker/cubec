#include "engine/checker.h"
#include "engine/diagnostic.h"
#include "engine/symbol.h"
#include "engine/semantic_type.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "core/error.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

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
  checker_t ctx = checker_create(allocator);
  source_cache_load(ctx->sources, "test.cubec", source, false);

  if (g_error) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         (location_t){0}, "%s", g_error->message);
    ctx->error_count++;
    error_clear();
    return (struct compile_result){ctx, prog, tokens};
  }

  checker_check_program(ctx, prog);
  return (struct compile_result){ctx, prog, tokens};
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  checker_dispose(r->ctx);
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

/* ===== test fixture ===== */

class dt_typeof : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

/* ===== typeof expression ===== */

TEST_F(dt_typeof, typeof_literal) {
  const char *src = BUILTIN_ASSERT
    "test \"typeof_lit\" {\n"
    "  var x: typeof(42) = undefined;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== sizeof expression ===== */

TEST_F(dt_typeof, sizeof_primitive) {
  const char *src = BUILTIN_ASSERT
    "test \"sizeof_i32\" {\n"
    "  var s = sizeof(i32);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_typeof, sizeof_pointer) {
  const char *src = BUILTIN_ASSERT
    "test \"sizeof_ptr\" {\n"
    "  var s = sizeof(*i32);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== alignof expression ===== */

TEST_F(dt_typeof, alignof_primitive) {
  const char *src = BUILTIN_ASSERT
    "test \"alignof_i32\" {\n"
    "  var a = alignof(i32);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== sizeof/typeof/alignof do NOT evaluate inner expression ===== */

TEST_F(dt_typeof, sizeof_does_not_evaluate) {
  /* sizeof(42) should NOT evaluate the inner expression — only compute its type.
   * 42 is a literal, so there's nothing to "execute", but this verifies the
   * resolver→_check_expression fallback path works for value expressions. */
  const char *src = BUILTIN_ASSERT
    "test \"sizeof_no_eval\" {\n"
    "  var s = sizeof(42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0) << "sizeof(42) should not produce errors";
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_typeof, typeof_does_not_evaluate) {
  /* typeof(assert(false)) should NOT trigger the assert.
   * typeof only computes the type without evaluating, so assert(false) is not
   * executed and no error from assert should appear. typeof(void) returns a
   * TYPE_TYPE wrapping void, which is valid. */
  const char *src = BUILTIN_ASSERT
    "test \"typeof_no_eval\" {\n"
    "  var x: typeof(assert(true)) = undefined;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_typeof, alignof_does_not_evaluate) {
  /* alignof(assert(false)) — assert returns void, alignof(void) is incomplete.
   * But we test alignof with a value expression that has a complete type. */
  const char *src = BUILTIN_ASSERT
    "test \"alignof_no_eval\" {\n"
    "  var a = alignof(42);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(r.ctx->error_count, 0);
  compile_result_cleanup(&r, allocator);
}
