#include "core/strmap.h"
#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "cubec/statement_function.h"
#include "engine/name.h"
#include "engine/name_collector.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_name_collector : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;

  /** Parse source and run name collection, returning the module. */
  module_t parse_and_collect(const char *source, const char *filename) {
    /* module_create takes ownership of source (allocator_free), so clone it */
    char *owned_source = cstring_clone(allocator, source);
    vec_t tokens = resolve_token_list(ctx, filename, owned_source);
    EXPECT_NE(tokens, nullptr);
    size_t pos = 0;
    node_t program = read_program_node(ctx, tokens, &pos, filename);
    EXPECT_NE(program, nullptr);
    module_t mod =
        module_create(allocator, vm_get_global_scope(ctx->vm), filename, owned_source, tokens, program);
    name_collector_run(ctx, mod);
    return mod;
  }

  /** Look up a name in the module's root scope. */
  name_t find_name(module_t mod, const char *name_str) {
    return (name_t)strmap_find(mod->root_scope->names, name_str);
  }
};

/* ---- Collect function names ---- */

TEST_F(it_name_collector, function_name) {
  const char *source = "func add(a: i32, b: i32): i32 { return a + b; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "add");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_FUNCTION);

  module_dispose(mod);
}

/* ---- Collect variable names (static scope) ---- */

TEST_F(it_name_collector, variable_name) {
  const char *source = "var x: i32 = 42;";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "x");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_VARIABLE);

  module_dispose(mod);
}

/* ---- Collect struct names ---- */

TEST_F(it_name_collector, struct_name) {
  const char *source = "struct Point { x: f64; y: f64; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "Point");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_TYPE);

  module_dispose(mod);
}

/* ---- Collect enum names ---- */

TEST_F(it_name_collector, enum_name) {
  const char *source = "enum Color { Red, Green, Blue }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "Color");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_TYPE);

  module_dispose(mod);
}

/* ---- Collect type alias names ---- */

TEST_F(it_name_collector, type_alias_name) {
  const char *source = "type MyInt = i32;";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "MyInt");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_TYPE);

  module_dispose(mod);
}

/* ---- Collect union names ---- */

TEST_F(it_name_collector, union_name) {
  const char *source = "union Option[T] { value: T; empty: void; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "Option");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_TYPE);

  module_dispose(mod);
}

/* ---- Collect interface names ---- */

TEST_F(it_name_collector, interface_name) {
  const char *source = "interface Printable { func to_string(self): string; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "Printable");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_TYPE);

  module_dispose(mod);
}

/* ---- Collect cunion names ---- */

TEST_F(it_name_collector, cunion_name) {
  const char *source = "cunion Data { int_val: i32; float_val: f64; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "Data");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_TYPE);

  module_dispose(mod);
}

/* ---- Collect import as namespace ---- */

TEST_F(it_name_collector, import_namespace) {
  /* Create a dependency module file */
  const char *dep_path = "/tmp/cubec_import_ns_dep.cubec";
  FILE *f = fopen(dep_path, "w");
  ASSERT_NE(f, nullptr);
  fputs("func helper(): void {}", f);
  fclose(f);

  const char *source = "import dep from \"/tmp/cubec_import_ns_dep.cubec\";";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "dep");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_NAMESPACE);

  module_dispose(mod);
}

/* ---- Mixed declarations ---- */

TEST_F(it_name_collector, mixed_declarations) {
  const char *source =
      "var count: i32 = 0;\n"
      "func inc(): void { count = count + 1; }\n"
      "struct Point { x: f64; y: f64; }\n"
      "type Vec3[T] = Point;\n";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t n_count = find_name(mod, "count");
  name_t n_inc = find_name(mod, "inc");
  name_t n_point = find_name(mod, "Point");
  name_t n_vec3 = find_name(mod, "Vec3");

  ASSERT_NE(n_count, nullptr);
  EXPECT_EQ(n_count->kind, NAME_VARIABLE);

  ASSERT_NE(n_inc, nullptr);
  EXPECT_EQ(n_inc->kind, NAME_FUNCTION);

  ASSERT_NE(n_point, nullptr);
  EXPECT_EQ(n_point->kind, NAME_TYPE);

  ASSERT_NE(n_vec3, nullptr);
  EXPECT_EQ(n_vec3->kind, NAME_TYPE);

  module_dispose(mod);
}

/* ---- name ref is set after def collection phase 1 ---- */

TEST_F(it_name_collector, name_ref_null_in_phase1) {
  const char *source = "func foo(): void {}";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "foo");
  ASSERT_NE(name, nullptr);
  /* Phase 1: ref is NULL — will be filled during definition collection */
  EXPECT_EQ(name->ref, nullptr);

  module_dispose(mod);
}

/* ---- Duplicate names: error ---- */

TEST_F(it_name_collector, duplicate_name_error) {
  const char *source =
      "func foo(): void {}\n"
      "func foo(): i32 { return 1; }\n";
  module_t mod = parse_and_collect(source, "test.cubec");

  /* Second declaration should produce a duplicate error */
  EXPECT_GT(context_get_error_count(ctx), 0);

  /* First declaration should still be in scope (not overwritten) */
  name_t name = find_name(mod, "foo");
  ASSERT_NE(name, nullptr);

  module_dispose(mod);
}

/* ---- context_import ---- */

TEST_F(it_name_collector, context_import_creates_module) {
  /* Write a temporary file to import */
  const char *tmp_path = "/tmp/cubec_import_test.cubec";
  FILE *f = fopen(tmp_path, "w");
  ASSERT_NE(f, nullptr);
  fputs("func hello(): void {}", f);
  fclose(f);

  /* context_import loads the module; name collection is separate */
  module_t mod = context_import(ctx, tmp_path);
  ASSERT_NE(mod, nullptr);
  EXPECT_NE(mod->program, nullptr);

  /* Run name collection explicitly */
  name_collector_run(ctx, mod);

  name_t name = (name_t)strmap_find(mod->root_scope->names, "hello");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_FUNCTION);
}

TEST_F(it_name_collector, context_import_caches_by_abs_path) {
  const char *tmp_path = "/tmp/cubec_import_test.cubec";
  FILE *f = fopen(tmp_path, "w");
  ASSERT_NE(f, nullptr);
  fputs("func hello(): void {}", f);
  fclose(f);

  module_t mod1 = context_import(ctx, tmp_path);
  ASSERT_NE(mod1, nullptr);

  module_t mod2 = context_import(ctx, tmp_path);
  EXPECT_EQ(mod1, mod2); /* same pointer, cached */
}

/* ---- Export: exported func appears in module exports ---- */

TEST_F(it_name_collector, export_function) {
  const char *source = "export func add(a: i32, b: i32): i32 { return a + b; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "add");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_FUNCTION);

  /* Also check exports table */
  name_t exported = (name_t)strmap_find(mod->exports, "add");
  ASSERT_NE(exported, nullptr);
  EXPECT_EQ(exported->kind, NAME_FUNCTION);

  module_dispose(mod);
}

/* ---- Export: exported struct appears in module exports ---- */

TEST_F(it_name_collector, export_struct) {
  const char *source = "export struct Point { x: f64; y: f64; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t exported = (name_t)strmap_find(mod->exports, "Point");
  ASSERT_NE(exported, nullptr);
  EXPECT_EQ(exported->kind, NAME_TYPE);

  module_dispose(mod);
}

/* ---- Export: non-exported name does NOT appear in exports ---- */

TEST_F(it_name_collector, non_exported_not_in_exports) {
  const char *source =
      "func internal(): void {}\n"
      "export func api(): void {}\n";
  module_t mod = parse_and_collect(source, "test.cubec");

  /* internal is in scope names but not in exports */
  name_t scope_name = find_name(mod, "internal");
  ASSERT_NE(scope_name, nullptr);
  name_t exported_internal = (name_t)strmap_find(mod->exports, "internal");
  EXPECT_EQ(exported_internal, nullptr);

  /* api is in both scope names and exports */
  name_t exported_api = (name_t)strmap_find(mod->exports, "api");
  ASSERT_NE(exported_api, nullptr);
  EXPECT_EQ(exported_api->kind, NAME_FUNCTION);

  module_dispose(mod);
}

/* ---- Export: re-export from dependency (export *) ---- */

TEST_F(it_name_collector, re_export_star) {
  /* Create a dependency module file */
  const char *dep_path = "/tmp/cubec_reexport_dep.cubec";
  FILE *f = fopen(dep_path, "w");
  ASSERT_NE(f, nullptr);
  fputs("export func dep_func(): void {}", f);
  fclose(f);

  const char *source = "export * from \"/tmp/cubec_reexport_dep.cubec\";";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t exported = (name_t)strmap_find(mod->exports, "dep_func");
  ASSERT_NE(exported, nullptr);
  EXPECT_EQ(exported->kind, NAME_FUNCTION);

  module_dispose(mod);
}

/* ---- Export: re-export selected names from dependency ---- */

TEST_F(it_name_collector, re_export_selected_names) {
  /* Create a dependency module file */
  const char *dep_path = "/tmp/cubec_reexport_sel_dep.cubec";
  FILE *f = fopen(dep_path, "w");
  ASSERT_NE(f, nullptr);
  fputs("export func foo(): void {}\nexport func bar(): void {}", f);
  fclose(f);

  const char *source = "export { foo } from \"/tmp/cubec_reexport_sel_dep.cubec\";";
  module_t mod = parse_and_collect(source, "test.cubec");

  /* foo is re-exported */
  name_t exported_foo = (name_t)strmap_find(mod->exports, "foo");
  ASSERT_NE(exported_foo, nullptr);
  EXPECT_EQ(exported_foo->kind, NAME_FUNCTION);

  /* bar is NOT re-exported (not in the export list) */
  name_t exported_bar = (name_t)strmap_find(mod->exports, "bar");
  EXPECT_EQ(exported_bar, nullptr);

  module_dispose(mod);
}

/* ---- Diagnostic: import with bare module name is an error ---- */

TEST_F(it_name_collector, import_bare_module_name_error) {
  const char *source = "import std from \"std\";";
  module_t mod = parse_and_collect(source, "test.cubec");

  EXPECT_GT(context_get_error_count(ctx), 0);

  module_dispose(mod);
}

/* ---- Diagnostic: import non-existent file is an error ---- */

TEST_F(it_name_collector, import_nonexistent_file_error) {
  const char *source = "import foo from \"/tmp/cubec_nonexistent.cubec\";";
  module_t mod = parse_and_collect(source, "test.cubec");

  EXPECT_GT(context_get_error_count(ctx), 0);
  /* foo should not be registered as a namespace */
  name_t name = find_name(mod, "foo");
  EXPECT_EQ(name, nullptr);

  module_dispose(mod);
}

/* ======== Semantic object creation tests ======== */

/* ---- variable names: ref=NULL in phase 1 ---- */

TEST_F(it_name_collector, var_name_ref_null_in_phase1) {
  const char *source = "var x: i32 = 42;";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "x");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_VARIABLE);
  EXPECT_EQ(name->ref, nullptr); /* phase 1: not set */

  module_dispose(mod);
}

/* ---- function names: ref=NULL in phase 1 ---- */

TEST_F(it_name_collector, function_name_ref_null_in_phase1) {
  const char *source = "func add(a: i32, b: i32): i32 { return a + b; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "add");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_FUNCTION);
  EXPECT_EQ(name->ref, nullptr); /* phase 1: not set */

  module_dispose(mod);
}

/* ---- type names: ref=NULL in phase 1 ---- */

TEST_F(it_name_collector, type_name_ref_null_in_phase1) {
  const char *source = "struct Point { x: f64; y: f64; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "Point");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_TYPE);
  EXPECT_EQ(name->ref, nullptr); /* phase 1: not set */

  module_dispose(mod);
}

/* ---- namespace names: ref=NULL in phase 1 ---- */

TEST_F(it_name_collector, namespace_name_ref_null_in_phase1) {
  const char *dep_path = "/tmp/cubec_ns_ref_dep.cubec";
  FILE *f = fopen(dep_path, "w");
  ASSERT_NE(f, nullptr);
  fputs("func helper(): void {}", f);
  fclose(f);

  const char *source = "import dep from \"/tmp/cubec_ns_ref_dep.cubec\";";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "dep");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_NAMESPACE);
  EXPECT_EQ(name->ref, nullptr); /* phase 1: not set */

  module_dispose(mod);
}
