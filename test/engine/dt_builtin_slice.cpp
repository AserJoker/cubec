#include "engine/context.h"
#include "engine/builtin.h"
#include "engine/builtin_slice.h"
#include "engine/symbol.h"
#include "engine/diagnostic.h"
#include "engine/semantic_type.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <string>

using ::testing::Test;

/* ===== helpers ===== */

#define BUILTIN_ASSERT "builtin func assert(condition: bool): void;\n"
#define BUILTIN_CAST "builtin func cast[T,K](expr:K):T;\n"
#define BUILTIN_LENGTH "builtin func length[T](list: T): u64;\n"
#define BUILTIN_MAKESLICE "builtin func makeSlice[T](pointer: *T, start: u64, len: u64): []T;\n"

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

class dt_builtin_slice : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== Builtin table ===== */

TEST_F(dt_builtin_slice, table_lookup_makeSlice) {
  context_t ctx = context_create(allocator);
  builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, "makeSlice");
  ASSERT_NE(be, nullptr);
  EXPECT_NE(be->eval_call, nullptr);
  context_dispose(ctx);
}

/* ===== Declaration validation ===== */

TEST_F(dt_builtin_slice, makeSlice_declared_correctly) {
  const char *src = "builtin func makeSlice[T](pointer: *T, start: u64, len: u64): []T;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);

  struct symbol *sym = scope_lookup(r.ctx->global_scope, "makeSlice");
  ASSERT_NE(sym, nullptr);
  EXPECT_TRUE(sym->is_builtin);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin_slice, makeSlice_unknown_builtin) {
  /* Typo in name — should be 'makeSlice' not 'makeslice' */
  const char *src = "builtin func makeslice[T](pointer: *T, start: u64, len: u64): []T;\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== Slice type checks ===== */

TEST_F(dt_builtin_slice, slice_layout_3_fields) {
  context_t ctx = context_create(allocator);
  semantic_type_t sl = semantic_type_create_slice(allocator, ctx->builtin_i32);
  type_hash_ensure(sl);
  type_layout_compute(sl, 8);
  /* slice = { data(8) + start(8) + length(8) } = 24 */
  EXPECT_EQ(semantic_type_get_size(sl), 24u);
  EXPECT_EQ(semantic_type_get_alignment(sl), 8u);
  allocator_free(allocator, &sl);
  context_dispose(ctx);
}

/* ===== makeSlice usage in code ===== */

TEST_F(dt_builtin_slice, makeSlice_basic_usage) {
  const char *src = BUILTIN_ASSERT BUILTIN_LENGTH BUILTIN_MAKESLICE
    "test \"makeslice_basic\" {\n"
    "  var x: i32 = 42;\n"
    "  var s: []i32 = makeSlice[i32](x.&, 0, 1);\n"
    "  assert(length(s) == 1);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin_slice, makeSlice_with_start_offset) {
  const char *src = BUILTIN_ASSERT BUILTIN_LENGTH BUILTIN_MAKESLICE
    "test \"makeslice_offset\" {\n"
    "  var arr: [4]i32 = .[4]i32{1, 2, 3, 4};\n"
    "  var s: []i32 = makeSlice[i32](arr[0].&, 1, 2);\n"
    "  assert(length(s) == 2);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== slice → pointer cast ===== */

TEST_F(dt_builtin_slice, slice_to_pointer_cast_allowed) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST BUILTIN_MAKESLICE
    "test \"slice_to_ptr\" {\n"
    "  var x: i32 = 42;\n"
    "  var s: []i32 = makeSlice[i32](x.&, 0, 1);\n"
    "  var p: *i32 = cast[*i32](s);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_builtin_slice, slice_to_wrong_pointer_cast_error) {
  /* []i32 → *f64 should fail (element type mismatch) */
  const char *src = BUILTIN_ASSERT BUILTIN_CAST BUILTIN_MAKESLICE
    "test \"slice_wrong_ptr\" {\n"
    "  var x: i32 = 42;\n"
    "  var s: []i32 = makeSlice[i32](x.&, 0, 1);\n"
    "  var p: *f64 = cast[*f64](s);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_GT(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== length on slice ===== */

TEST_F(dt_builtin_slice, length_on_slice) {
  const char *src = BUILTIN_ASSERT BUILTIN_LENGTH BUILTIN_MAKESLICE
    "test \"slice_length\" {\n"
    "  var x: i32 = 42;\n"
    "  var s: []i32 = makeSlice[i32](x.&, 0, 5);\n"
    "  assert(length(s) == 5);\n"
    "}\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== implicit conversions: nil → slice ===== */

TEST_F(dt_builtin_slice, nil_to_slice) {
  const char *src = BUILTIN_ASSERT
    "var s: []i32 = nil;\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== str → const []u8 auto-decay ===== */

TEST_F(dt_builtin_slice, str_to_const_slice_u8) {
  const char *src = BUILTIN_ASSERT
    "var s: const []u8 = \"hello\";\n";
  auto r = compile_source(ctx, src);
  EXPECT_EQ(context_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}
