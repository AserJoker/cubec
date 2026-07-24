/**
 * @file dt_export_from.cpp
 * @brief Tests for the export * from / export { } from re-export syntax.
 */

#include "engine/checker.h"
#include "engine/symbol.h"
#include "engine/diagnostic.h"
#include "engine/module.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "core/error.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

using ::testing::Test;

/* ===== helpers ===== */

struct compile_result {
  checker_t ctx;
  node_t prog;
  vec_t tokens;
};

static struct compile_result compile_file(allocator_t allocator,
                                          const char *filename,
                                          const char *source) {
  vec_t tokens = resolve_token_list(allocator, filename, source);
  size_t pos = 0;
  node_t prog = read_program_node(allocator, tokens, &pos, filename);

  if (g_error) {
    std::string err_msg(g_error->message);
    error_clear();
    GTEST_MESSAGE_AT_(__FILE__, __LINE__,
        ("Parsing failed: " + err_msg).c_str(),
        ::testing::TestPartResult::kFatalFailure);
    return (struct compile_result){NULL, prog, tokens};
  }

  checker_t ctx = checker_create(allocator);
  ctx->current_file = filename;
  source_cache_load(ctx->sources, filename, source, false);

  checker_check_program(ctx, prog);
  return (struct compile_result){ctx, prog, tokens};
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  if (r->ctx) checker_dispose(r->ctx);
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

static char *write_temp_file(const char *dir, const char *name,
                              const char *content) {
  char path[512];
  snprintf(path, sizeof(path), "%s/%s", dir, name);
  FILE *f = fopen(path, "w");
  if (!f) return NULL;
  fputs(content, f);
  fclose(f);
  return strdup(path);
}

static char *make_temp_dir(void) {
#ifdef _WIN32
  char tmp[MAX_PATH];
  GetTempPathA(MAX_PATH, tmp);
  char dir[MAX_PATH];
  snprintf(dir, sizeof(dir), "%scubec_exp_test_%d", tmp, (int)GetCurrentProcessId());
  CreateDirectoryA(dir, NULL);
#else
  char dir[] = "/tmp/cubec_exp_test_XXXXXX";
  mkdtemp(dir);
#endif
  return strdup(dir);
}

class dt_export_from : public CubecTest {
protected:
  TEST_ALLOCATOR;
  char *temp_dir;

  void SetUp() override {
    CubecTest::SetUp();
    temp_dir = make_temp_dir();
  }

  void TearDown() override {
    if (temp_dir) {
#ifdef _WIN32
      char cmd[512];
      snprintf(cmd, sizeof(cmd), "rmdir /s /q \"%s\" 2>NUL", temp_dir);
#else
      char cmd[512];
      snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", temp_dir);
#endif
      system(cmd);
      free(temp_dir);
    }
    error_clear();
    CubecTest::TearDown();
  }
};

/* ===== export * from "path" ===== */

TEST_F(dt_export_from, reexport_star) {
  /* Module "math" exports a function */
  const char *math_src =
    "export func add(a: i32, b: i32): i32 { return a + b; }\n";
  char *math_path = write_temp_file(temp_dir, "math.cubec", math_src);
  ASSERT_NE(math_path, nullptr);

  /* Module "api" re-exports everything from math */
  char api_path[512];
  snprintf(api_path, sizeof(api_path), "%s/api.cubec", temp_dir);
  const char *api_src =
    "export * from \"./math\";\n";

  auto r = compile_file(allocator, api_path, api_src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);

  /* Verify "add" is in the current module's global scope with is_export=true */
  struct symbol *add_sym = scope_lookup_local(r.ctx->global_scope, "add");
  ASSERT_NE(add_sym, nullptr);
  EXPECT_EQ(add_sym->kind, SYMBOL_FUNCTION);
  EXPECT_TRUE(add_sym->is_export);

  compile_result_cleanup(&r, allocator);
  free(math_path);
}

TEST_F(dt_export_from, reexport_star_type_and_func) {
  /* Module "lib" exports a struct and a function */
  const char *lib_src =
    "export struct Point { x: i32; y: i32; }\n"
    "export func identity(x: i32): i32 { return x; }\n"
    "func internal_helper(): void {}\n";  /* NOT exported */
  char *lib_path = write_temp_file(temp_dir, "lib.cubec", lib_src);
  ASSERT_NE(lib_path, nullptr);

  /* Module "public" re-exports everything from lib */
  char public_path[512];
  snprintf(public_path, sizeof(public_path), "%s/public.cubec", temp_dir);
  const char *public_src =
    "export * from \"./lib\";\n";

  auto r = compile_file(allocator, public_path, public_src);
  ASSERT_NE(r.ctx, nullptr);
  if (checker_get_error_count(r.ctx) > 0) {
    diagnostic_list_t diags = r.ctx->diagnostics;
    if (diags) {
      size_t dcount = diagnostic_list_get_size(diags);
      for (size_t i = 0; i < dcount; i++) {
        struct diagnostic *d = diagnostic_list_get(diags, i);
        if (d) printf("  DIAG: %s\n", d->message);
      }
    }
  }
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);

  /* "Point" and "identity" should be re-exported */
  struct symbol *point_sym = scope_lookup_local(r.ctx->global_scope, "Point");
  ASSERT_NE(point_sym, nullptr);
  EXPECT_TRUE(point_sym->is_export);

  struct symbol *id_sym = scope_lookup_local(r.ctx->global_scope, "identity");
  ASSERT_NE(id_sym, nullptr);
  EXPECT_TRUE(id_sym->is_export);

  /* "internal_helper" should NOT be re-exported (not exported in source) */
  struct symbol *helper_sym = scope_lookup_local(r.ctx->global_scope, "internal_helper");
  EXPECT_EQ(helper_sym, nullptr);

  compile_result_cleanup(&r, allocator);
  free(lib_path);
}

/* ===== export { a, b } from "path" ===== */

TEST_F(dt_export_from, reexport_selective) {
  /* Module "collections" exports Vec and Map */
  const char *coll_src =
    "export struct Vec { data: i32; }\n"
    "export struct Map { key: i32; }\n"
    "export struct Set { elem: i32; }\n";
  char *coll_path = write_temp_file(temp_dir, "collections.cubec", coll_src);
  ASSERT_NE(coll_path, nullptr);

  /* Module "api" selectively re-exports only Vec and Map */
  char api_path[512];
  snprintf(api_path, sizeof(api_path), "%s/api.cubec", temp_dir);
  const char *api_src =
    "export { Vec, Map } from \"./collections\";\n";

  auto r = compile_file(allocator, api_path, api_src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);

  /* Vec and Map should be re-exported */
  struct symbol *vec_sym = scope_lookup_local(r.ctx->global_scope, "Vec");
  ASSERT_NE(vec_sym, nullptr);
  EXPECT_TRUE(vec_sym->is_export);

  struct symbol *map_sym = scope_lookup_local(r.ctx->global_scope, "Map");
  ASSERT_NE(map_sym, nullptr);
  EXPECT_TRUE(map_sym->is_export);

  /* Set should NOT be re-exported (not in the list) */
  struct symbol *set_sym = scope_lookup_local(r.ctx->global_scope, "Set");
  EXPECT_EQ(set_sym, nullptr);

  compile_result_cleanup(&r, allocator);
  free(coll_path);
}

/* ===== error: re-export non-exported symbol ===== */

TEST_F(dt_export_from, reexport_non_exported_error) {
  /* Module "lib" has a non-exported function */
  const char *lib_src =
    "func internal(): void {}\n";
  char *lib_path = write_temp_file(temp_dir, "lib.cubec", lib_src);
  ASSERT_NE(lib_path, nullptr);

  /* Try to selectively re-export a non-exported symbol */
  char api_path[512];
  snprintf(api_path, sizeof(api_path), "%s/api.cubec", temp_dir);
  const char *api_src =
    "export { internal } from \"./lib\";\n";

  auto r = compile_file(allocator, api_path, api_src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);

  compile_result_cleanup(&r, allocator);
  free(lib_path);
}

/* ===== error: re-export non-existent symbol ===== */

TEST_F(dt_export_from, reexport_nonexistent_error) {
  /* Module "lib" has only one symbol */
  const char *lib_src =
    "export func foo(): void {}\n";
  char *lib_path = write_temp_file(temp_dir, "lib.cubec", lib_src);
  ASSERT_NE(lib_path, nullptr);

  /* Try to selectively re-export a symbol that doesn't exist */
  char api_path[512];
  snprintf(api_path, sizeof(api_path), "%s/api.cubec", temp_dir);
  const char *api_src =
    "export { bar } from \"./lib\";\n";

  auto r = compile_file(allocator, api_path, api_src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);

  compile_result_cleanup(&r, allocator);
  free(lib_path);
}

/* ===== export * combined with import ===== */

TEST_F(dt_export_from, import_and_reexport) {
  /* Module "math" exports add */
  const char *math_src =
    "export func add(a: i32, b: i32): i32 { return a + b; }\n";
  char *math_path = write_temp_file(temp_dir, "math.cubec", math_src);
  ASSERT_NE(math_path, nullptr);

  /* Module "api" imports math and re-exports */
  char api_path[512];
  snprintf(api_path, sizeof(api_path), "%s/api.cubec", temp_dir);
  const char *api_src =
    "import math from \"./math\";\n"
    "export * from \"./math\";\n";

  auto r = compile_file(allocator, api_path, api_src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);

  /* "math" module symbol should exist */
  struct symbol *math_sym = scope_lookup_local(r.ctx->global_scope, "math");
  ASSERT_NE(math_sym, nullptr);
  EXPECT_EQ(math_sym->kind, SYMBOL_MODULE);

  /* "add" should be re-exported */
  struct symbol *add_sym = scope_lookup_local(r.ctx->global_scope, "add");
  ASSERT_NE(add_sym, nullptr);
  EXPECT_TRUE(add_sym->is_export);

  compile_result_cleanup(&r, allocator);
  free(math_path);
}
