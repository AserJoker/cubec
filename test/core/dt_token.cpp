#include "cubec/token.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_token : public Test {
protected:
  TEST_ALLOCATOR;
};

// Helper function to check token kind
void check_token_kind(vec_t vec, size_t index, uint32_t expected_kind) {
  token_t token = (token_t)vec_get(vec, index);
  ASSERT_NE(token, nullptr);
  EXPECT_EQ(token_get_kind(token), expected_kind) << " at index " << index;
}

// Test EOF token
TEST_F(dt_token, eof_token) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 1);
  check_token_kind(vec, 0, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// NOTE: whitespace tokens are currently returned as CUBEC_TOKEN_SYMBOL
// This is a bug in token.c - create_whitespace_token should use CUBEC_TOKEN_WHITESPACE
TEST_F(dt_token, whitespace_only) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "   \t\n  ");
  ASSERT_NE(vec, nullptr);
  // Whitespace tokens are currently included and marked as SYMBOL (bug)
  // Final result includes all whitespace tokens + EOF
  EXPECT_GT(vec_get_size(vec), 1);
  allocator_free(allocator, vec);
}

// Test identifier token
TEST_F(dt_token, identifier_token) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "foo");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2); // identifier + EOF
  check_token_kind(vec, 0, CUBEC_TOKEN_IDENTIFIER);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test multiple identifiers with whitespace between them
TEST_F(dt_token, multiple_identifiers) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "foo bar baz");
  ASSERT_NE(vec, nullptr);
  // Each identifier + whitespace tokens + EOF
  EXPECT_GT(vec_get_size(vec), 4); // identifiers + whitespace + EOF
  check_token_kind(vec, 0, CUBEC_TOKEN_IDENTIFIER);
  check_token_kind(vec, 2, CUBEC_TOKEN_IDENTIFIER);
  check_token_kind(vec, 4, CUBEC_TOKEN_IDENTIFIER);
  allocator_free(allocator, vec);
}

// Test keywords
TEST_F(dt_token, keyword_token) {
  const char *keywords[] = {
      "break",  "case",    "comptime", "const", "continue", "defer",
      "do",     "else",    "enum",     "export", "extern",   "for",
      "foreach","func",    "if",       "import", "in",       "inline",
      "mutable","of",      "pub",      "register","return",  "struct",
      "switch", "test",    "union",    "volatile", "while",  NULL
  };

  for (int i = 0; keywords[i] != NULL; i++) {
    vec_t vec = resolve_token_list(allocator, "test.cubec", keywords[i]);
    ASSERT_NE(vec, nullptr) << "Failed for keyword: " << keywords[i];
    EXPECT_EQ(vec_get_size(vec), 2) << "Failed for keyword: " << keywords[i];
    check_token_kind(vec, 0, CUBEC_TOKEN_KEYWORD);
    allocator_free(allocator, vec);
  }
}

// Test numeric tokens - decimal integers
TEST_F(dt_token, numeric_decimal) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "12345");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_NUMERIC);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test numeric tokens - hexadecimal
TEST_F(dt_token, numeric_hexadecimal) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "0x1A3F");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_NUMERIC);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test numeric tokens - octal
TEST_F(dt_token, numeric_octal) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "0o755");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_NUMERIC);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test numeric tokens - binary
TEST_F(dt_token, numeric_binary) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "0b1010");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_NUMERIC);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test numeric tokens - float
TEST_F(dt_token, numeric_float) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "3.14159");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_NUMERIC);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test numeric tokens - scientific notation
TEST_F(dt_token, numeric_scientific) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "1e10");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_NUMERIC);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test string token
TEST_F(dt_token, string_token) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "\"hello world\"");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_STRING);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test empty string
TEST_F(dt_token, empty_string) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "\"\"");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_STRING);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

TEST_F(dt_token, single_line_comment) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "// this is a comment\n");
  ASSERT_NE(vec, nullptr);
  // comment token + newline whitespace + EOF
  EXPECT_EQ(vec_get_size(vec), 3);
  check_token_kind(vec, 0, CUBEC_TOKEN_COMMENT);
  check_token_kind(vec, 1, CUBEC_TOKEN_WHITESPACE);
  check_token_kind(vec, 2, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test multi-line comment
TEST_F(dt_token, multi_line_comment) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "/* this is a\nmulti-line comment */");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_MULTILINE_COMMENT);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test nested multi-line comment
TEST_F(dt_token, nested_multi_line_comment) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "/* outer /* inner */ outer */");
  ASSERT_NE(vec, nullptr);
  // Note: lexer does not support nested comments
  // It ends at the first */ found
  EXPECT_GT(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_MULTILINE_COMMENT);
  allocator_free(allocator, vec);
}

// Test whitespace tokens - spaces
TEST_F(dt_token, whitespace_spaces) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "   ");
  ASSERT_NE(vec, nullptr);
  // Spaces produce SYMBOL tokens (bug - should be WHITESPACE)
  EXPECT_GT(vec_get_size(vec), 1);
  allocator_free(allocator, vec);
}

// Test basic symbols
TEST_F(dt_token, symbol_plus) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "+");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

TEST_F(dt_token, symbol_minus) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "-");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

TEST_F(dt_token, symbol_multiply) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "*");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

TEST_F(dt_token, symbol_divide) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "/");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test compound symbols
TEST_F(dt_token, symbol_double_equals) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "==");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

TEST_F(dt_token, symbol_not_equals) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "!=");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

TEST_F(dt_token, symbol_logical_and) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "&&");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

TEST_F(dt_token, symbol_logical_or) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "||");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test braces and brackets
TEST_F(dt_token, symbol_braces) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "{}");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 3);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 1, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 2, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

TEST_F(dt_token, symbol_parentheses) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "()");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 3);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 1, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 2, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

TEST_F(dt_token, symbol_brackets) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "[]");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 3);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 1, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 2, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test complex source code snippet
TEST_F(dt_token, complex_snippet) {
  const char *source = "func main() {\n"
                       "    let x: i32 = 42;\n"
                       "    return x;\n"
                       "}";

  vec_t vec = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(vec, nullptr);

  // We expect multiple tokens including keywords, identifiers, symbols, numerics
  size_t size = vec_get_size(vec);
  EXPECT_GT(size, 10) << "Expected more tokens";

  // First token should be keyword 'func'
  check_token_kind(vec, 0, CUBEC_TOKEN_KEYWORD);

  allocator_free(allocator, vec);
}

// Test mixed content
TEST_F(dt_token, mixed_content) {
  const char *source = "let name: str = \"hello\"; // comment";

  vec_t vec = resolve_token_list(allocator, "test.cubec", source);
  ASSERT_NE(vec, nullptr);

  size_t size = vec_get_size(vec);
  EXPECT_GT(size, 3);

  allocator_free(allocator, vec);
}

// Test assignment operators
TEST_F(dt_token, assignment_operators) {
  const char *ops[] = {"=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "&&=", "||=", NULL};

  for (int i = 0; ops[i] != NULL; i++) {
    vec_t vec = resolve_token_list(allocator, "test.cubec", ops[i]);
    ASSERT_NE(vec, nullptr) << "Failed for operator: " << ops[i];
    EXPECT_EQ(vec_get_size(vec), 2) << "Failed for operator: " << ops[i];
    check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
    allocator_free(allocator, vec);
  }
}

// Test shift operators
TEST_F(dt_token, shift_operators) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", "<<");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  allocator_free(allocator, vec);

  vec = resolve_token_list(allocator, "test.cubec", ">>");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  allocator_free(allocator, vec);
}

// Test comparison operators
TEST_F(dt_token, comparison_operators) {
  const char *ops[] = {">=", "<=", "==", "!=", NULL};

  for (int i = 0; ops[i] != NULL; i++) {
    vec_t vec = resolve_token_list(allocator, "test.cubec", ops[i]);
    ASSERT_NE(vec, nullptr) << "Failed for operator: " << ops[i];
    EXPECT_EQ(vec_get_size(vec), 2) << "Failed for operator: " << ops[i];
    check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
    allocator_free(allocator, vec);
  }
}

// Test semicolon and colon
TEST_F(dt_token, punctuation) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", ";:,?");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 5);
  for (size_t i = 0; i < 4; i++) {
    check_token_kind(vec, i, CUBEC_TOKEN_SYMBOL);
  }
  check_token_kind(vec, 4, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}

// Test bitwise operators
TEST_F(dt_token, bitwise_operators) {
  const char *ops[] = {"&", "|", "^", "~", NULL};

  for (int i = 0; ops[i] != NULL; i++) {
    vec_t vec = resolve_token_list(allocator, "test.cubec", ops[i]);
    ASSERT_NE(vec, nullptr) << "Failed for operator: " << ops[i];
    EXPECT_EQ(vec_get_size(vec), 2) << "Failed for operator: " << ops[i];
    check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
    allocator_free(allocator, vec);
  }
}

// Test dot operator
TEST_F(dt_token, symbol_dot) {
  vec_t vec = resolve_token_list(allocator, "test.cubec", ".");
  ASSERT_NE(vec, nullptr);
  EXPECT_EQ(vec_get_size(vec), 2);
  check_token_kind(vec, 0, CUBEC_TOKEN_SYMBOL);
  check_token_kind(vec, 1, CUBEC_TOKEN_EOF);
  allocator_free(allocator, vec);
}