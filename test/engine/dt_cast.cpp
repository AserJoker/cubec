#include "engine/checker.h"
#include "engine/builtin.h"
#include "engine/symbol.h"
#include "engine/diagnostic.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "core/error.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ===== helpers ===== */

#define BUILTIN_ASSERT "builtin func assert(condition: bool): void;\n"
#define BUILTIN_CAST "builtin func cast[T,K](expr:K):T;\n"

class dt_cast : public CubecTest {
protected:
  TEST_ALLOCATOR;

  struct compile_result {
    checker_t ctx;
    node_t prog;
    vec_t tokens;
  };

  struct compile_result compile_source(const char *source) {
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
    }

    checker_check_program(ctx, prog);
    return (struct compile_result){ctx, prog, tokens};
  }

  void compile_result_cleanup(struct compile_result *r) {
    checker_dispose(r->ctx);
    allocator_free(allocator, &r->prog);
    allocator_free(allocator, &r->tokens);
  }
};

/* ===== Numeric: float → int ===== */

TEST_F(dt_cast, float_to_int) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "test \"cast_f2i\" {\n"
    "  var x: i32 = cast[i32](3.14);\n"
    "  assert(x == 3);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

TEST_F(dt_cast, int_narrowing) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "test \"cast_narrow\" {\n"
    "  var x: i8 = cast[i8](300);\n"
    "  assert(x == 44);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

TEST_F(dt_cast, float_narrowing) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "test \"cast_fnarrow\" {\n"
    "  var x: f32 = cast[f32](3.141592653589793);\n"
    "  assert(x != 0.0);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

/* ===== Bool ↔ int ===== */

TEST_F(dt_cast, bool_to_int) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "test \"cast_bool2i\" {\n"
    "  var x: i32 = cast[i32](true);\n"
    "  assert(x == 1);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

TEST_F(dt_cast, int_to_bool) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "test \"cast_i2bool\" {\n"
    "  var x: bool = cast[bool](42);\n"
    "  assert(x);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

/* ===== Enum ↔ int ===== */

TEST_F(dt_cast, enum_to_int) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "enum Color { Red, Green, Blue }\n"
    "test \"cast_e2i\" {\n"
    "  var v: i32 = cast[i32](Color.Red);\n"
    "  assert(v == 0);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

TEST_F(dt_cast, int_to_enum) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "enum Color { Red, Green, Blue }\n"
    "test \"cast_i2e\" {\n"
    "  var v: Color = cast[Color](1);\n"
    "  assert(v == Color.Green);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

/* ===== Char ↔ int ===== */

TEST_F(dt_cast, char_to_int) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "test \"cast_c2i\" {\n"
    "  var x: i32 = cast[i32]('A');\n"
    "  assert(x == 65);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

TEST_F(dt_cast, int_to_char) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "test \"cast_i2c\" {\n"
    "  var x: char = cast[char](65);\n"
    "  assert(x == 'A');\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

/* ===== Pointer: opaque → pointer ===== */

TEST_F(dt_cast, opaque_to_pointer) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "var x: i32 = 0;\n"
    "test \"cast_op2ptr\" {\n"
    "  var p: opaque = x.&;\n"
    "  var q: *i32 = cast[*i32](p);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

/* ===== Pointer → int ===== */

TEST_F(dt_cast, pointer_to_int) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "var x: i32 = 0;\n"
    "test \"cast_ptr2int\" {\n"
    "  var p: *i32 = x.&;\n"
    "  var addr: u64 = cast[u64](p);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

/* ===== Struct pointer downcast ===== */

TEST_F(dt_cast, struct_ptr_downcast) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "struct Base { x: i32; }\n"
    "struct Ext { x: i32; y: i64; }\n"
    "test \"cast_downcast\" {\n"
    "  var b: Base = .{.x = 1};\n"
    "  var p: *Ext = cast[*Ext](b.&);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

/* ===== Container: array → tuple ===== */

TEST_F(dt_cast, array_to_tuple) {
  const char *src = BUILTIN_ASSERT BUILTIN_CAST
    "test \"cast_arr2tup\" {\n"
    "  var a: [3]i32 = .{1, 2, 3};\n"
    "  var t: <i32, i32, i32> = cast[<i32, i32, i32>](a);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

/* ===== Invalid cast → error ===== */

TEST_F(dt_cast, invalid_string_to_int) {
  const char *src = BUILTIN_CAST
    "test \"cast_invalid\" {\n"
    "  var x: i32 = cast[i32](\"hello\");\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}

TEST_F(dt_cast, invalid_ptr_to_unrelated_ptr) {
  const char *src = BUILTIN_CAST
    "var x: i32 = 0;\n"
    "test \"cast_badptr\" {\n"
    "  var p: *i32 = x.&;\n"
    "  var q: *f64 = cast[*f64](p);\n"
    "}\n";
  auto r = compile_source(src);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);
  compile_result_cleanup(&r);
}
