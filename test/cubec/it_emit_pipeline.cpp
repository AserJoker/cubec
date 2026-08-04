#include "core/emit_context.h"
#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/token.h"
#include "cubec/node.h"
#include "cubec/statement.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_emit_pipeline : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;

  /* Helper: parse + emit a statement and return the output string */
  string_t emit_statement_str(const char *source) {
    vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
    emit_context_t ectx = emit_context_create(allocator, tokens);

    size_t position = 0;
    node_t node = read_statement(ctx, tokens, &position, "test.cubec");
    if (!node) {
      emit_context_dispose(ectx);
      allocator_free(allocator, &tokens);
      return nullptr;
    }

    emit_statement(ectx, node);
    emit_newline(ectx);

    string_t output = token_writer_render(allocator, ectx->output_tokens);

    allocator_free(allocator, &node);
    emit_context_dispose(ectx);
    allocator_free(allocator, &tokens);
    return output;
  }

  /* Helper: parse + emit a full program and return the output string */
  string_t emit_program_str(const char *source) {
    vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
    emit_context_t ectx = emit_context_create(allocator, tokens);

    size_t position = 0;
    node_t node = read_program_node(ctx, tokens, &position, "test.cubec");
    if (!node) {
      emit_context_dispose(ectx);
      allocator_free(allocator, &tokens);
      return nullptr;
    }

    emit_program(ectx, node);

    string_t output = token_writer_render(allocator, ectx->output_tokens);

    allocator_free(allocator, &node);
    emit_context_dispose(ectx);
    allocator_free(allocator, &tokens);
    return output;
  }
};

/* ==========================================================================
 *  Statement emit tests
 * ========================================================================== */

TEST_F(it_emit_pipeline, emit_var_declaration) {
  string_t output = emit_statement_str("var x: i32 = 42;");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "var x: i32 = 42;\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_empty_statement) {
  string_t output = emit_statement_str(";");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), ";\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_return_expression) {
  string_t output = emit_statement_str("return x + 1;");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "return x + 1;\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_return_void) {
  string_t output = emit_statement_str("return;");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "return;\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_break) {
  string_t output = emit_statement_str("break;");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "break;\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_continue) {
  string_t output = emit_statement_str("continue;");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "continue;\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_if_simple) {
  string_t output = emit_statement_str("if(x) { }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "if (x) {\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_if_else) {
  string_t output = emit_statement_str("if(x) { } else { }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "if (x) {\n} else {\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_while) {
  string_t output = emit_statement_str("while(x) { }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "while (x) {\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_do_while) {
  string_t output = emit_statement_str("do { } while(x);");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "do {\n} while (x);\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_for) {
  string_t output = emit_statement_str("for(var i = 0; i < 10; i = i + 1) { }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "for (var i = 0; i < 10; i = i + 1) {\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_foreach) {
  string_t output = emit_statement_str("foreach(item of items) { }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "foreach (item of items) {\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_block_empty) {
  string_t output = emit_statement_str("{ }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "{\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_block_with_stmts) {
  string_t output = emit_statement_str("{ var x = 1; var y = 2; }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "{\n  var x = 1;\n  var y = 2;\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_import) {
  string_t output = emit_statement_str("import foo from \"path\";");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "import foo from \"path\";\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_export_wildcard) {
  string_t output = emit_statement_str("export * from \"path\";");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "export * from \"path\";\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_defer_block) {
  string_t output = emit_statement_str("defer { var x = 1; }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "defer {\n  var x = 1;\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_switch) {
  string_t output = emit_statement_str("switch(x) { case(1) -> { } else -> { } }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "switch (x) {\n  case(1) -> {\n  }\n  else -> {\n  }\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_struct) {
  string_t output = emit_statement_str("struct Point { x: f64; y: f64; }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "struct Point {\n  x: f64;\n  y: f64;\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_enum_) {
  string_t output = emit_statement_str("enum Color { Red, Green, Blue }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "enum Color {\n  Red,\n  Green,\n  Blue,\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_union) {
  string_t output = emit_statement_str("union Result[E, T] { value: T; error: E; }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "union Result[E, T] {\n  value: T;\n  error: E;\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_cunion) {
  string_t output = emit_statement_str("cunion Data { int_val: i32; float_val: f64; }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "cunion Data {\n  int_val: i32;\n  float_val: f64;\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_interface) {
  string_t output = emit_statement_str("interface Printable { func print(): void; }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "interface Printable {\n  func print(): void;\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_function) {
  string_t output = emit_statement_str("func add(a: i32, b: i32): i32 { return a + b; }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "func add(a: i32, b: i32): i32 {\n  return a + b;\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_comptime_if) {
  string_t output = emit_statement_str("comptime if(debug) { var x = 1; }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "comptime if (debug) {\n  var x = 1;\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, emit_comptime_foreach) {
  string_t output = emit_statement_str("comptime foreach(item of items) { }");
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "comptime foreach (item of items) {\n}\n");
  allocator_free(allocator, &output);
}

/* ==========================================================================
 *  Comment preservation
 * ========================================================================== */

TEST_F(it_emit_pipeline, preserve_comment_before_statement) {
  string_t output = emit_statement_str("/* comment */ var x = 1;");
  ASSERT_NE(output, nullptr);
  /* Comment should be recovered before the var keyword */
  EXPECT_STREQ(string_get(output), "/* comment */ var x = 1;\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, preserve_comment_inside_block) {
  string_t output = emit_statement_str("{ /* inner */ var x = 1; }");
  ASSERT_NE(output, nullptr);
  /* Comment should be recovered inside the block */
  EXPECT_STREQ(string_get(output), "{\n  /* inner */ var x = 1;\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, preserve_comment_between_if_else) {
  string_t output = emit_statement_str("if(x) { } /* comment */ else { }");
  ASSERT_NE(output, nullptr);
  /* Comment should be recovered between if and else */
  EXPECT_STREQ(string_get(output), "if (x) {\n} /* comment */ else {\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, preserve_multiline_comment) {
  string_t output = emit_statement_str("/* line1\nline2 */ var x = 1;");
  ASSERT_NE(output, nullptr);
  /* Multiline comment should be preserved */
  EXPECT_STREQ(string_get(output), "/* line1\nline2 */ var x = 1;\n");
  allocator_free(allocator, &output);
}

/* ==========================================================================
 *  Program round-trip
 * ========================================================================== */

TEST_F(it_emit_pipeline, program_roundtrip_simple) {
  const char *source = "var x: i32 = 1;\nvar y: i32 = 2;\n";
  string_t output = emit_program_str(source);
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "var x: i32 = 1;\nvar y: i32 = 2;\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, program_roundtrip_with_struct) {
  const char *source = "struct Point {\n  x: f64;\n  y: f64;\n}\n";
  string_t output = emit_program_str(source);
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "struct Point {\n  x: f64;\n  y: f64;\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, program_roundtrip_with_if_else) {
  const char *source = "if(x > 0) {\n  var y = 1;\n} else {\n  var z = 2;\n}\n";
  string_t output = emit_program_str(source);
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "if (x > 0) {\n  var y = 1;\n} else {\n  var z = 2;\n}\n");
  allocator_free(allocator, &output);
}

TEST_F(it_emit_pipeline, program_roundtrip_with_comments) {
  const char *source = "/* header */\nvar x: i32 = 1;\n/* footer */\nvar y: i32 = 2;\n";
  string_t output = emit_program_str(source);
  ASSERT_NE(output, nullptr);
  EXPECT_STREQ(string_get(output), "/* header */ var x: i32 = 1;\n/* footer */ var y: i32 = 2;\n");
  allocator_free(allocator, &output);
}
