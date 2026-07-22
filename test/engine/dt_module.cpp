/**
 * @file dt_module.cpp
 * @brief Tests for the module import/export system.
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

/* Helper: write a temp .cubec file and return its path (caller must free) */
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

/* Helper: create a temp directory */
static char *make_temp_dir(void) {
#ifdef _WIN32
  char tmp[MAX_PATH];
  GetTempPathA(MAX_PATH, tmp);
  char dir[MAX_PATH];
  snprintf(dir, sizeof(dir), "%scubec_mod_test_%d", tmp, (int)GetCurrentProcessId());
  CreateDirectoryA(dir, NULL);
#else
  char dir[] = "/tmp/cubec_mod_test_XXXXXX";
  mkdtemp(dir);
#endif
  return strdup(dir);
}

class dt_module : public CubecTest {
protected:
  TEST_ALLOCATOR;
  char *temp_dir;

  void SetUp() override {
    CubecTest::SetUp();
    temp_dir = make_temp_dir();
  }

  void TearDown() override {
    /* Clean up temp files */
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

/* ===== module_resolve_path tests ===== */

TEST_F(dt_module, resolve_path_relative) {
  char *resolved = module_resolve_path("./vec", "/home/user/main.cubec");
  ASSERT_NE(resolved, nullptr);
  EXPECT_STREQ(resolved, "/home/user/vec.cubec");
  free(resolved);
}

TEST_F(dt_module, resolve_path_parent) {
  char *resolved = module_resolve_path("../utils", "/home/user/src/main.cubec");
  ASSERT_NE(resolved, nullptr);
  EXPECT_STREQ(resolved, "/home/user/utils.cubec");
  free(resolved);
}

TEST_F(dt_module, resolve_path_already_has_ext) {
  char *resolved = module_resolve_path("./vec.cubec", "/home/user/main.cubec");
  ASSERT_NE(resolved, nullptr);
  EXPECT_STREQ(resolved, "/home/user/vec.cubec");
  free(resolved);
}

/* ===== basic import/export ===== */

TEST_F(dt_module, import_export_func) {
  /* Create a module file that exports a function */
  const char *math_src =
    "export func add(a: i32, b: i32): i32 { return a + b; }\n";
  char *math_path = write_temp_file(temp_dir, "math.cubec", math_src);
  ASSERT_NE(math_path, nullptr);

  /* Create a main file that imports and uses the function */
  char main_path[512];
  snprintf(main_path, sizeof(main_path), "%s/main.cubec", temp_dir);
  const char *main_src =
    "import math from \"./math\";\n"
    "var x: i32 = math::add(1, 2);\n";

  auto r = compile_file(allocator, main_path, main_src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);

  compile_result_cleanup(&r, allocator);
  free(math_path);
}

TEST_F(dt_module, import_export_type) {
  /* Create a module that exports a type */
  const char *types_src =
    "export type Point = struct { x: i32; y: i32; };\n";
  char *types_path = write_temp_file(temp_dir, "types.cubec", types_src);
  ASSERT_NE(types_path, nullptr);

  char main_path[512];
  snprintf(main_path, sizeof(main_path), "%s/main.cubec", temp_dir);
  const char *main_src =
    "import types from \"./types\";\n"
    "func use_point(p: types::Point): void {}\n";

  auto r = compile_file(allocator, main_path, main_src);
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

  /* Verify the type is accessible via module scope */
  struct symbol *mod_sym = scope_lookup_local(r.ctx->global_scope, "types");
  ASSERT_NE(mod_sym, nullptr);
  EXPECT_EQ(mod_sym->kind, SYMBOL_MODULE);
  scope_t mod_scope = mod_sym->module.scope;
  ASSERT_NE(mod_scope, nullptr);
  struct symbol *point_sym = scope_lookup_local(mod_scope, "Point");
  ASSERT_NE(point_sym, nullptr);
  EXPECT_TRUE(point_sym->is_export);

  compile_result_cleanup(&r, allocator);
  free(types_path);
}

TEST_F(dt_module, import_export_struct_init) {
  /* Cross-module struct initialization: .module::TypeName{.field = value} */
  const char *types_src =
    "export type Point = struct { x: i32; y: i32; };\n";
  char *types_path = write_temp_file(temp_dir, "types.cubec", types_src);
  ASSERT_NE(types_path, nullptr);

  char main_path[512];
  snprintf(main_path, sizeof(main_path), "%s/main.cubec", temp_dir);
  const char *main_src =
    "import types from \"./types\";\n"
    "var p = .types::Point{.x = 10, .y = 20};\n";

  auto r = compile_file(allocator, main_path, main_src);
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

  compile_result_cleanup(&r, allocator);
  free(types_path);
}

TEST_F(dt_module, import_export_struct_init_alias) {
  /* Cross-module struct init with import alias: .alias::TypeName{.field = value} */
  const char *vec_src =
    "export type Vec2 = struct { x: f64; y: f64; };\n";
  char *vec_path = write_temp_file(temp_dir, "vec.cubec", vec_src);
  ASSERT_NE(vec_path, nullptr);

  char main_path[512];
  snprintf(main_path, sizeof(main_path), "%s/main.cubec", temp_dir);
  const char *main_src =
    "import vec as v from \"./vec\";\n"
    "var p = .v::Vec2{.x = 1.5, .y = 2.5};\n";

  auto r = compile_file(allocator, main_path, main_src);
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

  compile_result_cleanup(&r, allocator);
  free(vec_path);
}

/* ===== non-exported symbol not visible ===== */

TEST_F(dt_module, non_exported_symbol_invisible) {
  const char *mod_src =
    "type Helper = i32;\n"
    "export type PubType = i32;\n";
  char *mod_path = write_temp_file(temp_dir, "mod.cubec", mod_src);
  ASSERT_NE(mod_path, nullptr);

  char main_path[512];
  snprintf(main_path, sizeof(main_path), "%s/main.cubec", temp_dir);
  /* Access a non-exported type in type position — namespace access must reject it */
  const char *main_src =
    "import mod from \"./mod\";\n"
    "var x: mod::Helper = 0;\n";

  auto r = compile_file(allocator, main_path, main_src);
  ASSERT_NE(r.ctx, nullptr);
  /* mod::Helper should fail because Helper is not exported */
  EXPECT_GT(checker_get_error_count(r.ctx), 0);

  compile_result_cleanup(&r, allocator);
  free(mod_path);
}

/* ===== import alias ===== */

TEST_F(dt_module, import_alias) {
  const char *vec_src =
    "export type Vec2 = struct { x: f64; y: f64; };\n";
  char *vec_path = write_temp_file(temp_dir, "vec.cubec", vec_src);
  ASSERT_NE(vec_path, nullptr);

  char main_path[512];
  snprintf(main_path, sizeof(main_path), "%s/main.cubec", temp_dir);
  const char *main_src =
    "import vec as v from \"./vec\";\n"
    "func use_vec(p: v::Vec2): void {}\n";

  auto r = compile_file(allocator, main_path, main_src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(checker_get_error_count(r.ctx), 0);

  /* Verify the alias works: 'v' should resolve to the module */
  struct symbol *alias_sym = scope_lookup_local(r.ctx->global_scope, "v");
  ASSERT_NE(alias_sym, nullptr);
  EXPECT_EQ(alias_sym->kind, SYMBOL_MODULE);

  compile_result_cleanup(&r, allocator);
  free(vec_path);
}

/* ===== file not found ===== */

TEST_F(dt_module, import_file_not_found) {
  char main_path[512];
  snprintf(main_path, sizeof(main_path), "%s/main.cubec", temp_dir);
  const char *main_src =
    "import missing from \"./nonexistent\";\n";

  auto r = compile_file(allocator, main_path, main_src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);

  /* Check that the error message mentions "cannot read module" */
  bool found = false;
  diagnostic_list_t diags = r.ctx->diagnostics;
  if (diags) {
    size_t count = diagnostic_list_get_size(diags);
    for (size_t i = 0; i < count; i++) {
      struct diagnostic *d = diagnostic_list_get(diags, i);
      if (d && strstr(d->message, "cannot read module")) {
        found = true;
        break;
      }
    }
  }
  EXPECT_TRUE(found);

  compile_result_cleanup(&r, allocator);
}

/* ===== module entry create/dispose ===== */

TEST_F(dt_module, module_entry_lifecycle) {
  module_entry_t entry = module_entry_create("/tmp/test.cubec");
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ(entry->state, MODULE_PARSING);
  EXPECT_NE(entry->resolved_path, nullptr);
  EXPECT_STREQ(entry->resolved_path, "/tmp/test.cubec");
  EXPECT_EQ(entry->checker, nullptr);
  EXPECT_EQ(entry->scope, nullptr);

  module_entry_dispose(entry);
}

/* ===== export flag on symbols ===== */

TEST_F(dt_module, export_flag_set_on_symbols) {
  const char *src =
    "export func pub_func(): void {}\n"
    "func priv_func(): void {}\n"
    "export type PubType = struct { x: i32; };\n"
    "type PrivType = struct { y: i32; };\n"
    "export var pub_var: i32 = 1;\n"
    "var priv_var: i32 = 2;\n";

  auto r = compile_file(allocator, "test.cubec", src);
  ASSERT_NE(r.ctx, nullptr);

  struct symbol *pub_fn = scope_lookup_local(r.ctx->global_scope, "pub_func");
  ASSERT_NE(pub_fn, nullptr);
  EXPECT_TRUE(pub_fn->is_export);

  struct symbol *priv_fn = scope_lookup_local(r.ctx->global_scope, "priv_func");
  ASSERT_NE(priv_fn, nullptr);
  EXPECT_FALSE(priv_fn->is_export);

  struct symbol *pub_type = scope_lookup_local(r.ctx->global_scope, "PubType");
  ASSERT_NE(pub_type, nullptr);
  EXPECT_TRUE(pub_type->is_export);

  struct symbol *priv_type = scope_lookup_local(r.ctx->global_scope, "PrivType");
  ASSERT_NE(priv_type, nullptr);
  EXPECT_FALSE(priv_type->is_export);

  struct symbol *pub_var = scope_lookup_local(r.ctx->global_scope, "pub_var");
  ASSERT_NE(pub_var, nullptr);
  EXPECT_TRUE(pub_var->is_export);

  struct symbol *priv_var = scope_lookup_local(r.ctx->global_scope, "priv_var");
  ASSERT_NE(priv_var, nullptr);
  EXPECT_FALSE(priv_var->is_export);

  compile_result_cleanup(&r, allocator);
}
