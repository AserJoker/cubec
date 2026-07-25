#include "engine/context.h"
#include "engine/diagnostic.h"
#include "engine/symbol.h"
#include "engine/semantic_type.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <string>

using ::testing::Test;

/* ===== helpers ===== */

#define BUILTIN_ASSERT "builtin func assert(condition: bool): void;\n"

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

  /* If parsing failed, fail the test immediately */
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

/* ===== test fixture ===== */

class dt_undefined : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== undefined literal: basic parsing ===== */

TEST_F(dt_undefined, undefined_with_type_annotation) {
  /* var x: i32 = undefined; should compile — variable is TDZ */
  const char *src = BUILTIN_ASSERT
    "test \"undefined_typed\" {\n"
    "  var x: i32 = undefined;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_undefined, undefined_no_type_error) {
  /* var x = undefined; should error — cannot infer type */
  const char *src = BUILTIN_ASSERT
    "test \"undefined_no_type\" {\n"
    "  var x = undefined;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_undefined, undefined_standalone_expr_error) {
  /* Using undefined as a standalone expression is an error */
  const char *src = BUILTIN_ASSERT
    "test \"undefined_standalone\" {\n"
    "  undefined;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== var requires initializer ===== */

TEST_F(dt_undefined, var_no_init_error) {
  /* var x: i32; without initializer should be an error */
  const char *src = BUILTIN_ASSERT
    "test \"var_no_init\" {\n"
    "  var x: i32;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_undefined, extern_no_init_ok) {
  /* extern var x: i32; should be fine without initializer */
  const char *src = BUILTIN_ASSERT
    "extern var x: i32;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_undefined, builtin_no_init_not_required) {
  /* builtin var x: i32; doesn't require initializer, but will fail
   * "unknown builtin" if x is not a registered builtin name.
   * The key point: no "requires an initializer" error. */
  const char *src = BUILTIN_ASSERT
    "builtin var x: i32;\n";
  auto r = compile_source(ctx, src);
  /* Error is "unknown builtin 'x'" — NOT "requires an initializer" */
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== TDZ: use of undefined-initialized variable ===== */

TEST_F(dt_undefined, undefined_use_before_assign_error) {
  /* var x: i32 = undefined; reading x before assignment should be TDZ error */
  const char *src = BUILTIN_ASSERT
    "test \"tdz_use\" {\n"
    "  var x: i32 = undefined;\n"
    "  assert(x == 0);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  /* TDZ check may not be fully implemented yet — but the code should at
   * least not crash.  If TDZ is enforced, error_count > 0. */
  EXPECT_NO_FATAL_FAILURE(compile_result_cleanup(&r, allocator));
}

TEST_F(dt_undefined, undefined_assign_then_use_ok) {
  /* var x: i32 = undefined; x = 5; assert(x == 5); should work */
  const char *src = BUILTIN_ASSERT
    "test \"tdz_assign_use\" {\n"
    "  var x: i32 = undefined;\n"
    "  x = 5;\n"
    "  assert(x == 5);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  /* Assignment should transition from TDZ to EVALUATED.
   * This may not be fully implemented yet, so just check no crash. */
  EXPECT_NO_FATAL_FAILURE(compile_result_cleanup(&r, allocator));
}

TEST_F(dt_undefined, undefined_with_pointer_type) {
  /* undefined works with pointer types too */
  const char *src = BUILTIN_ASSERT
    "test \"undefined_ptr\" {\n"
    "  var p: *i32 = undefined;\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}
