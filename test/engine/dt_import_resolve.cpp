/**
 * @file dt_import_resolve.cpp
 * @brief Tests for extended import path resolution (std, project deps, global deps).
 */

#include "engine/checker.h"
#include "engine/symbol.h"
#include "engine/diagnostic.h"
#include "engine/module.h"
#include "engine/manifest.h"
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

static char *make_temp_dir(void) {
#ifdef _WIN32
  char tmp[MAX_PATH];
  GetTempPathA(MAX_PATH, tmp);
  char dir[MAX_PATH];
  snprintf(dir, sizeof(dir), "%scubec_imp_test_%d", tmp, (int)GetCurrentProcessId());
  CreateDirectoryA(dir, NULL);
#else
  char dir[] = "/tmp/cubec_imp_test_XXXXXX";
  mkdtemp(dir);
#endif
  return strdup(dir);
}

static void write_temp_file(const char *dir, const char *name,
                            const char *content) {
  char path[512];
  snprintf(path, sizeof(path), "%s/%s", dir, name);
  FILE *f = fopen(path, "w");
  if (!f) return;
  fputs(content, f);
  fclose(f);
}

class dt_import_resolve : public CubecTest {
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

/* ===== module_resolve_import unit tests ===== */

TEST_F(dt_import_resolve, relative_path_unchanged) {
  /* Relative paths still resolve via module_resolve_path */
  bool is_ghost = false;
  char *resolved = module_resolve_import("./vec", "/home/user/main.cubec",
                                          "/home/user", NULL, NULL, &is_ghost);
  ASSERT_NE(resolved, nullptr);
  EXPECT_FALSE(is_ghost);
  /* Should be equivalent to module_resolve_path result */
  char *expected = module_resolve_path("./vec", "/home/user/main.cubec");
  EXPECT_STREQ(resolved, expected);
  free(resolved);
  free(expected);
}

TEST_F(dt_import_resolve, std_prefix_resolution) {
  /* std/io → ${cubec_home}/library/std/io/index.cubec */
  bool is_ghost = false;
  char *resolved = module_resolve_import("std/io", "/home/user/main.cubec",
                                          "/home/user", NULL, NULL, &is_ghost);
  ASSERT_NE(resolved, nullptr);
  EXPECT_FALSE(is_ghost);
  EXPECT_NE(strstr(resolved, "library/std/io"), nullptr);
  free(resolved);
}

TEST_F(dt_import_resolve, project_dep_resolution) {
  /* With project_root and manifest_deps, collections/vec resolves to project */
  bool is_ghost = false;
  /* Create a simple strmap with "collections" as a dep */
  strmap_init_t si = {.value_auto_dispose = false};
  strmap_t deps = (strmap_t)allocator_create(allocator, &g_strmap_type, &si);
  strmap_insert(deps, "collections", (void *)(intptr_t)1);

  char *resolved = module_resolve_import("collections/vec", "/home/user/main.cubec",
                                          "/opt/cubec", "/home/user",
                                          deps, &is_ghost);
  ASSERT_NE(resolved, nullptr);
  EXPECT_FALSE(is_ghost);
  EXPECT_NE(strstr(resolved, "/home/user/library/collections/vec"), nullptr);

  allocator_free(allocator, &deps);
  free(resolved);
}

TEST_F(dt_import_resolve, global_dep_fallback) {
  /* Without project_root, collections/vec resolves via cubec_home */
  bool is_ghost = false;
  char *resolved = module_resolve_import("collections/vec", "/home/user/main.cubec",
                                          "/opt/cubec", NULL, NULL, &is_ghost);
  ASSERT_NE(resolved, nullptr);
  EXPECT_FALSE(is_ghost);
  EXPECT_NE(strstr(resolved, "/opt/cubec/library/collections/vec"), nullptr);
  free(resolved);
}

TEST_F(dt_import_resolve, ghost_dep_detected) {
  /* Dep not in manifest_deps → is_ghost=true */
  bool is_ghost = false;
  strmap_init_t si = {.value_auto_dispose = false};
  strmap_t deps = (strmap_t)allocator_create(allocator, &g_strmap_type, &si);
  strmap_insert(deps, "allowed_dep", (void *)(intptr_t)1);

  char *resolved = module_resolve_import("unknown_dep/mod", "/home/user/main.cubec",
                                          "/opt/cubec", "/home/user",
                                          deps, &is_ghost);
  /* Should still resolve, but mark as ghost */
  ASSERT_NE(resolved, nullptr);
  EXPECT_TRUE(is_ghost);

  allocator_free(allocator, &deps);
  free(resolved);
}

TEST_F(dt_import_resolve, std_not_ghost) {
  /* std/ prefix is never ghost regardless of manifest_deps */
  bool is_ghost = false;
  strmap_init_t si = {.value_auto_dispose = false};
  strmap_t deps = (strmap_t)allocator_create(allocator, &g_strmap_type, &si);
  /* Empty deps — nothing declared */

  char *resolved = module_resolve_import("std/io", "/home/user/main.cubec",
                                          "/opt/cubec", "/home/user",
                                          deps, &is_ghost);
  ASSERT_NE(resolved, nullptr);
  EXPECT_FALSE(is_ghost);  /* std is always allowed */

  allocator_free(allocator, &deps);
  free(resolved);
}

/* ===== manifest_find_root tests ===== */

TEST_F(dt_import_resolve, manifest_find_root_found) {
  /* Create a temp dir with manifest.json */
  write_temp_file(temp_dir, "manifest.json",
    "{\"name\":\"test\",\"version\":\"0.1.0\"}");

  char main_path[512];
  snprintf(main_path, sizeof(main_path), "%s/src/main.cubec", temp_dir);
#ifdef _WIN32
  CreateDirectoryA((std::string(temp_dir) + "/src").c_str(), NULL);
#else
  mkdir((std::string(temp_dir) + "/src").c_str(), 0755);
#endif

  char *root = manifest_find_root(main_path);
  ASSERT_NE(root, nullptr);
  EXPECT_NE(strstr(root, temp_dir), nullptr);

  free(root);
}

TEST_F(dt_import_resolve, manifest_find_root_not_found) {
  char *root = manifest_find_root("/nonexistent/path/file.cubec");
  EXPECT_EQ(root, nullptr);
}

/* ===== manifest_parse tests ===== */

TEST_F(dt_import_resolve, manifest_parse_basic) {
  write_temp_file(temp_dir, "manifest.json",
    "{\"name\":\"myapp\",\"version\":\"0.1.0\",\"deps\":{\"std_io\":{\"url\":\"file://.\"},\"collections\":{\"url\":\"file://.\"}}}");

  char *name = NULL;
  char **dep_names = NULL;
  int result = manifest_parse(temp_dir, &name, &dep_names);
  EXPECT_EQ(result, 0);
  ASSERT_NE(name, nullptr);
  EXPECT_STREQ(name, "myapp");
  ASSERT_NE(dep_names, nullptr);
  /* Should have 2 dep names */
  int count = 0;
  for (int i = 0; dep_names[i]; i++) count++;
  EXPECT_EQ(count, 2);

  manifest_free_dep_names(dep_names);
  free(name);
}

/* ===== integration: import std/io ===== */

TEST_F(dt_import_resolve, import_std_integration) {
  /* Create CUBEC_HOME with std/io/index.cubec */
  char *home = temp_dir;

  /* Create manifest.json so _ensure_project_context finds the project root */
  write_temp_file(home, "manifest.json",
    "{\"name\":\"stdtest\",\"version\":\"0.1.0\",\"deps\":{}}");

  char io_path[512];
  snprintf(io_path, sizeof(io_path), "%s/library/std/io", home);
#ifdef _WIN32
  char lib_path[512];
  snprintf(lib_path, sizeof(lib_path), "%s/library", home);
  CreateDirectoryA(lib_path, NULL);
  char std_path[512];
  snprintf(std_path, sizeof(std_path), "%s/library/std", home);
  CreateDirectoryA(std_path, NULL);
  CreateDirectoryA(io_path, NULL);
#else
  mkdir((std::string(home) + "/library").c_str(), 0755);
  mkdir((std::string(home) + "/library/std").c_str(), 0755);
  mkdir(io_path, 0755);
#endif

  write_temp_file(home, "library/std/io/index.cubec",
    "export func println(msg: str): void;\n");

  /* Create main file that imports std/io */
  char main_path[512];
  snprintf(main_path, sizeof(main_path), "%s/main.cubec", home);
  const char *main_src =
    "import io from \"std/io\";\n";

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
}

/* ===== integration: import project dep ===== */

TEST_F(dt_import_resolve, import_project_dep_integration) {
  /* Create project with manifest.json and library/mylib/vec */
  char *project = temp_dir;
  char lib_path[512], mylib_path[512], vec_path[512];
  snprintf(lib_path, sizeof(lib_path), "%s/library", project);
  snprintf(mylib_path, sizeof(mylib_path), "%s/library/mylib", project);
  snprintf(vec_path, sizeof(vec_path), "%s/library/mylib/vec", project);
#ifdef _WIN32
  CreateDirectoryA(lib_path, NULL);
  CreateDirectoryA(mylib_path, NULL);
  CreateDirectoryA(vec_path, NULL);
#else
  mkdir(lib_path, 0755);
  mkdir(mylib_path, 0755);
  mkdir(vec_path, 0755);
#endif

  write_temp_file(project, "manifest.json",
    "{\"name\":\"app\",\"version\":\"0.1.0\",\"deps\":{\"mylib\":{\"url\":\"file://./library/mylib\"}}}");
  write_temp_file(project, "library/mylib/vec/index.cubec",
    "export struct Vec { data: i32; }\n");

  /* Main file imports mylib/vec */
  char main_path[512];
  snprintf(main_path, sizeof(main_path), "%s/main.cubec", project);
  const char *main_src =
    "import vec from \"mylib/vec\";\n";

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
}

/* ===== integration: ghost dependency error ===== */

TEST_F(dt_import_resolve, ghost_dep_error_integration) {
  /* Project with manifest.json that does NOT declare "unknown" dep */
  write_temp_file(temp_dir, "manifest.json",
    "{\"name\":\"app\",\"version\":\"0.1.0\",\"deps\":{}}");

  char main_path[512];
  snprintf(main_path, sizeof(main_path), "%s/main.cubec", temp_dir);
  const char *main_src =
    "import x from \"unknown/mod\";\n";

  auto r = compile_file(allocator, main_path, main_src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(checker_get_error_count(r.ctx), 0);

  compile_result_cleanup(&r, allocator);
}
