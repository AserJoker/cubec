/**
 * @file dt_comptime_string.cpp
 * @brief Tests for comptime str type, operations, and toString builtin.
 */

#include "engine/checker.h"
#include "engine/builtin.h"
#include "engine/comptime_eval.h"
#include "engine/comptime_value.h"
#include "engine/symbol.h"
#include "engine/diagnostic.h"
#include "cubec/ast_factory.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "core/error.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <string>

using ::testing::Test;

/* ===== helpers ===== */

#define BUILTIN_ASSERT "builtin func assert(condition: bool): void;\n"
#define BUILTIN_LENGTH "builtin func length[T](list: T): u64;\n"
#define BUILTIN_TOSTRING "builtin func toString[T](obj: T): str;\n"

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
  struct compile_result cr;
  if (g_error) {
    std::string err_msg(g_error->message);
    error_clear();
    GTEST_MESSAGE_AT_(__FILE__, __LINE__,
        ("Parsing failed: " + err_msg).c_str(),
        ::testing::TestPartResult::kFatalFailure);
    cr.ctx = NULL; cr.prog = prog; cr.tokens = tokens;
    return cr;
  }
  checker_t ctx = checker_create(allocator);
  source_cache_load(ctx->sources, "test.cubec", source, false);
  checker_check_program(ctx, prog);
  cr.ctx = ctx; cr.prog = prog; cr.tokens = tokens;
  return cr;
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  if (r->ctx) checker_dispose(r->ctx);
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

class dt_comptime_string : public CubecTest {
protected:
  TEST_ALLOCATOR;
  void TearDown() override {
    error_clear();
    CubecTest::TearDown();
  }
};

/* ===== str literal ===== */

TEST_F(dt_comptime_string, str_literal_type) {
  const char *src = BUILTIN_ASSERT
    "test \"str_literal\" {\n"
    "  var s: str = \"hello\";\n"
    "  assert(true);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_string, str_literal_comptime_eval) {
  checker_t ctx = checker_create(allocator);
  node_t s = cubec_ast_create_string(allocator,
      (location_t){.filename = "<test>", .begin = {1,1,NULL}, .end = {1,1,NULL}},
      "hello");
  comptime_value_t v = comptime_eval_expr(ctx->comptime_eval, ctx, s);
  ASSERT_NE(v, nullptr);
  EXPECT_EQ(v->kind, COMPTIME_VALUE_STRING);
  EXPECT_STREQ(comptime_value_get_string(v), "hello");
  /* str literal type should be builtin_str, not builtin_string */
  ASSERT_NE(v->type, nullptr);
  EXPECT_EQ(v->type->impl->kind, TYPE_STR);
  allocator_free(allocator, &s);
  checker_dispose(ctx);
}

/* ===== str + str concatenation ===== */

TEST_F(dt_comptime_string, str_concat) {
  const char *src = BUILTIN_ASSERT BUILTIN_LENGTH
    "test \"str_concat\" {\n"
    "  var s: str = \"hello\" + \" world\";\n"
    "  assert(length(s) == 11);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_string, str_concat_three) {
  const char *src = BUILTIN_ASSERT
    "test \"str_concat_three\" {\n"
    "  var s: str = \"a\" + \"b\" + \"c\";\n"
    "  assert(s == \"abc\");\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== str index read ===== */

TEST_F(dt_comptime_string, str_index_read) {
  const char *src = BUILTIN_ASSERT
    "test \"str_index_read\" {\n"
    "  var s: str = \"hello\";\n"
    "  var c: char = s[0];\n"
    "  assert(c == 'h');\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== str index write (comptime) ===== */

TEST_F(dt_comptime_string, str_index_write_comptime) {
  const char *src = BUILTIN_ASSERT
    "test \"str_index_write\" {\n"
    "  comptime {\n"
    "    var s: str = \"hello\";\n"
    "    s[0] = 'H';\n"
    "    assert(s == \"Hello\");\n"
    "  }\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== str length ===== */

TEST_F(dt_comptime_string, str_length) {
  const char *src = BUILTIN_ASSERT BUILTIN_LENGTH
    "test \"str_length\" {\n"
    "  assert(length(\"hello\") == 5);\n"
    "  assert(length(\"\") == 0);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== str equality ===== */

TEST_F(dt_comptime_string, str_equals) {
  const char *src = BUILTIN_ASSERT
    "test \"str_equals\" {\n"
    "  assert(\"abc\" == \"abc\");\n"
    "  assert(!(\"abc\" == \"def\"));\n"
    "  assert(\"abc\" != \"def\");\n"
    "  assert(!(\"abc\" != \"abc\"));\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== toString ===== */

TEST_F(dt_comptime_string, toString_bool) {
  const char *src = BUILTIN_ASSERT BUILTIN_TOSTRING
    "test \"toString_bool\" {\n"
    "  assert(toString(true) == \"true\");\n"
    "  assert(toString(false) == \"false\");\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_string, toString_int) {
  const char *src = BUILTIN_ASSERT BUILTIN_TOSTRING
    "test \"toString_int\" {\n"
    "  assert(toString(42) == \"42\");\n"
    "  assert(toString(0) == \"0\");\n"
    "  assert(toString(-7) == \"-7\");\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_string, toString_float) {
  const char *src = BUILTIN_ASSERT BUILTIN_TOSTRING
    "test \"toString_float\" {\n"
    "  assert(toString(3.14) == \"3.140000\");\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_string, toString_char) {
  const char *src = BUILTIN_ASSERT BUILTIN_TOSTRING
    "test \"toString_char\" {\n"
    "  assert(toString('a') == \"a\");\n"
    "  assert(toString('Z') == \"Z\");\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_string, toString_str_identity) {
  const char *src = BUILTIN_ASSERT BUILTIN_TOSTRING
    "test \"toString_str\" {\n"
    "  assert(toString(\"hello\") == \"hello\");\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== toString pointer ===== */

TEST_F(dt_comptime_string, toString_pointer) {
  const char *src = BUILTIN_ASSERT BUILTIN_TOSTRING
    "test \"toString_pointer\" {\n"
    "  var x: i32 = 42;\n"
    "  var p: *i32 = x.&;\n"
    "  var s: str = toString(p);\n"
    "  assert(true);\n"  /* just verify it compiles and produces a str */
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== toString not supported ===== */

TEST_F(dt_comptime_string, toString_array) {
  const char *src = BUILTIN_ASSERT BUILTIN_TOSTRING
    "test \"toString_array\" {\n"
    "  var s: str = toString(.[3]i32{1,2,3});\n"
    "  assert(s == \"1, 2, 3\");\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== str auto decay to const []u8 ===== */

TEST_F(dt_comptime_string, str_decay_to_const_slice) {
  const char *src = BUILTIN_ASSERT
    "test \"str_decay_const\" {\n"
    "  var s: str = \"hello\";\n"
    "  var v: const []u8 = s;\n"
    "  assert(true);\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_comptime_string, str_no_decay_to_mutable_slice) {
  const char *src = BUILTIN_ASSERT
    "test \"str_no_mutable\" {\n"
    "  var s: str = \"hello\";\n"
    "  var v: []u8 = s;\n"
    "}\n";
  auto r = compile_source(allocator, src);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r, allocator);
}

/* ===== builtin table: toString registered ===== */

TEST_F(dt_comptime_string, table_lookup_toString) {
  checker_t ctx = checker_create(allocator);
  builtin_entry_t be = builtin_table_lookup(ctx->builtin_table, "toString");
  ASSERT_NE(be, nullptr);
  EXPECT_NE(be->eval_call, nullptr);
  checker_dispose(ctx);
}

/* ===== str type in checker ===== */

TEST_F(dt_comptime_string, builtin_str_registered) {
  checker_t ctx = checker_create(allocator);
  ASSERT_NE(ctx->builtin_str, nullptr);
  EXPECT_EQ(ctx->builtin_str->impl->kind, TYPE_STR);
  checker_dispose(ctx);
}
