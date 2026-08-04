#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/statement.h"
#include "cubec/statement_import.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_string.h"
#include "cubec/node.h"
#include "cubec/node_error.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include "core/emit_context.h"

using ::testing::Test;

class dt_statement_import : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ---- Simple import: import std from "std"; ---- */

TEST_F(dt_statement_import, simple_import) {
  const char *source = "import std from \"std\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_import(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_IMPORT);

  cubec_statement_import_t imp = (cubec_statement_import_t)node;
  EXPECT_NE(imp->module_name, nullptr);
  EXPECT_EQ(imp->module_name->kind, CUBEC_NODE_LITERAL_IDENTIFIER);
  EXPECT_NE(imp->path, nullptr);
  EXPECT_EQ(imp->path->kind, CUBEC_NODE_LITERAL_STRING);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Relative path: import io from "./io"; ---- */

TEST_F(dt_statement_import, relative_path) {
  const char *source = "import io from \"./io\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_import(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_IMPORT);

  cubec_statement_import_t imp = (cubec_statement_import_t)node;
  EXPECT_NE(imp->path, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Parent path: import parent from "../parent"; ---- */

TEST_F(dt_statement_import, parent_path) {
  const char *source = "import parent from \"../parent\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_import(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_IMPORT);

  cubec_statement_import_t imp = (cubec_statement_import_t)node;
  EXPECT_NE(imp->path, nullptr);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Multi-segment path: import vec from "std/vec"; ---- */

TEST_F(dt_statement_import, multi_segment_path) {
  const char *source = "import vec from \"std/vec\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_import(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_IMPORT);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Consume all tokens ---- */

TEST_F(dt_statement_import, consume_all_tokens) {
  const char *source = "import std from \"std\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_import(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  /* import, std, from, "std", ;, + whitespace/comment tokens + EOF */
  EXPECT_EQ(position, vec_get_size(tokens) - 1);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Missing 'from' keyword error ---- */

TEST_F(dt_statement_import, missing_from_keyword) {
  const char *source = "import std \"std\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_import(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Missing semicolon error ---- */

TEST_F(dt_statement_import, missing_semicolon) {
  const char *source = "import std from \"std\"";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_import(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Non-import keyword returns NULL ---- */

TEST_F(dt_statement_import, non_import_returns_null) {
  const char *source = "var x = 1;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_import(ctx, tokens, &position, "test.cubec");
  EXPECT_EQ(node, nullptr);

  allocator_free(allocator, &tokens);
}

/* ---- Missing module name error ---- */

TEST_F(dt_statement_import, missing_module_name) {
  const char *source = "import from \"std\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_import(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Missing path error ---- */

TEST_F(dt_statement_import, missing_path) {
  const char *source = "import std from ;";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_import(ctx, tokens, &position, "test.cubec");
  EXPECT_TRUE(node_is_error(node));

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Clone ---- */

TEST_F(dt_statement_import, clone) {
  const char *source = "import vec from \"std/vec\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_import(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t cloned = (node_t)value_clone(allocator, node);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, CUBEC_NODE_STATEMENT_IMPORT);

  cubec_statement_import_t imp = (cubec_statement_import_t)cloned;
  EXPECT_NE(imp->module_name, nullptr);
  EXPECT_NE(imp->path, nullptr);

  allocator_free(allocator, &cloned);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Move ---- */

TEST_F(dt_statement_import, move) {
  const char *source = "import vec from \"std/vec\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement_import(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);

  node_t moved = (node_t)value_move(allocator, node);
  ASSERT_NE(moved, nullptr);
  EXPECT_EQ(moved->kind, CUBEC_NODE_STATEMENT_IMPORT);

  cubec_statement_import_t imp = (cubec_statement_import_t)moved;
  EXPECT_NE(imp->module_name, nullptr);
  EXPECT_NE(imp->path, nullptr);

  allocator_free(allocator, &moved);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Via read_statement dispatcher ---- */

TEST_F(dt_statement_import, via_statement_dispatcher) {
  const char *source = "import std from \"std\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_STATEMENT_IMPORT);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

/* ---- Via read_program_node (top-level) ---- */

TEST_F(dt_statement_import, via_program_node) {
  const char *source = "import std from \"std\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);

  size_t position = 0;
  node_t node = read_program_node(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->kind, CUBEC_NODE_PROGRAM);

  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}

TEST_F(dt_statement_import, write_import) {
  const char *source = "import std from \"std\";";
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  ASSERT_NE(tokens, nullptr);
  size_t position = 0;
  node_t node = read_statement(ctx, tokens, &position, "test.cubec");
  ASSERT_NE(node, nullptr);
  emit_context_t ectx = emit_context_create(allocator, tokens);
  emit_statement(ectx, node);
  emit_newline(ectx);
  string_t result = token_writer_render(allocator, ectx->output_tokens);
  emit_context_dispose(ectx);
  const char *output = string_get(result);
  EXPECT_STREQ(output, "import std from \"std\";\n");
  allocator_free(allocator, &result);
  allocator_free(allocator, &node);
  allocator_free(allocator, &tokens);
}
