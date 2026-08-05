#include "core/strmap.h"
#include "core/string.h"
#include "core/token_writer.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "engine/def.h"
#include "engine/def_collector.h"
#include "engine/name.h"
#include "engine/name_collector.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

class it_def_collector : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;

  /** Parse source, run name + def collection, returning the module. */
  module_t parse_and_collect(const char *source, const char *filename) {
    char *owned_source = strdup(source);
    vec_t tokens = resolve_token_list(ctx, filename, owned_source);
    EXPECT_NE(tokens, nullptr);
    size_t pos = 0;
    node_t program = read_program_node(ctx, tokens, &pos, filename);
    EXPECT_NE(program, nullptr);
    module_t mod =
        module_create(allocator, ctx->global_scope, filename, owned_source, tokens, program);
    name_collector_run(ctx, mod);
    def_collector_run(ctx, mod);
    return mod;
  }

  name_t find_name(module_t mod, const char *name_str) {
    return (name_t)strmap_find(mod->root_scope->names, name_str);
  }
};

/* ---- func def ---- */

TEST_F(it_def_collector, func_def) {
  const char *source = "func add(a: i32, b: i32): i32 { return a + b; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "add");
  ASSERT_NE(name, nullptr);
  ASSERT_NE(name->ref, nullptr);

  def_t def = (def_t)name->ref;
  EXPECT_EQ(def->kind, DEF_FUNC);
  EXPECT_NE(def->node, nullptr);
  EXPECT_EQ(def->node->kind, CUBEC_NODE_STATEMENT_FUNCTION);

  module_dispose(mod);
}

/* ---- var def ---- */

TEST_F(it_def_collector, var_def) {
  const char *source = "var x: i32 = 42;";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "x");
  ASSERT_NE(name, nullptr);
  ASSERT_NE(name->ref, nullptr);

  def_t def = (def_t)name->ref;
  EXPECT_EQ(def->kind, DEF_VAR);
  EXPECT_NE(def->node, nullptr);

  module_dispose(mod);
}

/* ---- struct def with generic params ---- */

TEST_F(it_def_collector, struct_def_no_generic) {
  const char *source = "struct Point { x: f64; y: f64; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "Point");
  ASSERT_NE(name, nullptr);
  ASSERT_NE(name->ref, nullptr);

  struct_def_t def = (struct_def_t)name->ref;
  EXPECT_EQ(def->super.kind, DEF_STRUCT);
  ASSERT_NE(def->params, nullptr);
  EXPECT_EQ(strmap_get_size(def->params), 0);
  EXPECT_EQ(def->implements, nullptr);
  EXPECT_EQ(def->members, nullptr);

  module_dispose(mod);
}

TEST_F(it_def_collector, struct_def_with_generic) {
  const char *source = "struct Container[T] { value: T; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  struct_def_t def = (struct_def_t)find_name(mod, "Container")->ref;
  ASSERT_NE(def->params, nullptr);
  EXPECT_EQ(strmap_get_size(def->params), 1);
  ASSERT_NE(strmap_find(def->params, "T"), nullptr);

  module_dispose(mod);
}

TEST_F(it_def_collector, struct_def_multi_generic) {
  const char *source = "struct Map[K, V] { key: K; val: V; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  struct_def_t def = (struct_def_t)find_name(mod, "Map")->ref;
  EXPECT_EQ(strmap_get_size(def->params), 2);
  ASSERT_NE(strmap_find(def->params, "K"), nullptr);
  ASSERT_NE(strmap_find(def->params, "V"), nullptr);

  module_dispose(mod);
}

/* ---- enum def ---- */

TEST_F(it_def_collector, enum_def) {
  const char *source = "enum Color { Red, Green, Blue }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "Color");
  ASSERT_NE(name, nullptr);
  ASSERT_NE(name->ref, nullptr);

  def_t def = (def_t)name->ref;
  EXPECT_EQ(def->kind, DEF_ENUM);
  EXPECT_NE(def->node, nullptr);

  module_dispose(mod);
}

/* ---- union def ---- */

TEST_F(it_def_collector, union_def_no_generic) {
  const char *source = "union Option { value: i32; empty: void; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  union_def_t def = (union_def_t)find_name(mod, "Option")->ref;
  EXPECT_EQ(def->super.kind, DEF_UNION);
  ASSERT_NE(def->params, nullptr);
  EXPECT_EQ(strmap_get_size(def->params), 0);
  EXPECT_EQ(def->implements, nullptr);
  EXPECT_EQ(def->members, nullptr);

  module_dispose(mod);
}

TEST_F(it_def_collector, union_def_with_generic) {
  const char *source = "union Option[T] { value: T; empty: void; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  union_def_t def = (union_def_t)find_name(mod, "Option")->ref;
  ASSERT_NE(def->params, nullptr);
  EXPECT_EQ(strmap_get_size(def->params), 1);
  ASSERT_NE(strmap_find(def->params, "T"), nullptr);

  module_dispose(mod);
}

TEST_F(it_def_collector, union_def_multi_generic) {
  const char *source = "union Either[A, B] { left: A; right: B; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  union_def_t def = (union_def_t)find_name(mod, "Either")->ref;
  EXPECT_EQ(strmap_get_size(def->params), 2);
  ASSERT_NE(strmap_find(def->params, "A"), nullptr);
  ASSERT_NE(strmap_find(def->params, "B"), nullptr);

  module_dispose(mod);
}

/* ---- interface def ---- */

TEST_F(it_def_collector, interface_def_no_generic) {
  const char *source = "interface Printable { func to_string(self): string; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  interface_def_t def = (interface_def_t)find_name(mod, "Printable")->ref;
  EXPECT_EQ(def->super.kind, DEF_INTERFACE);
  ASSERT_NE(def->params, nullptr);
  EXPECT_EQ(strmap_get_size(def->params), 0);
  EXPECT_EQ(def->members, nullptr);

  module_dispose(mod);
}

TEST_F(it_def_collector, interface_def_with_generic) {
  const char *source = "interface Container[T] { func get(self): T; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  interface_def_t def = (interface_def_t)find_name(mod, "Container")->ref;
  ASSERT_NE(def->params, nullptr);
  EXPECT_EQ(strmap_get_size(def->params), 1);
  ASSERT_NE(strmap_find(def->params, "T"), nullptr);

  module_dispose(mod);
}

/* ---- cunion def ---- */

TEST_F(it_def_collector, cunion_def) {
  const char *source = "cunion Data { int_val: i32; float_val: f64; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "Data");
  ASSERT_NE(name, nullptr);
  ASSERT_NE(name->ref, nullptr);

  def_t def = (def_t)name->ref;
  EXPECT_EQ(def->kind, DEF_CUNION);
  EXPECT_NE(def->node, nullptr);

  module_dispose(mod);
}

/* ---- type alias def ---- */

TEST_F(it_def_collector, type_alias_def_no_generic) {
  const char *source = "type MyInt = i32;";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "MyInt");
  ASSERT_NE(name, nullptr);
  ASSERT_NE(name->ref, nullptr);

  def_t def = (def_t)name->ref;
  EXPECT_EQ(def->kind, DEF_TYPE_ALIAS);
  EXPECT_NE(def->node, nullptr);

  type_alias_def_t ta = (type_alias_def_t)def;
  EXPECT_EQ(ta->type_value, nullptr);
  EXPECT_FALSE(ta->is_builtin);
  ASSERT_NE(ta->params, nullptr);
  EXPECT_EQ(strmap_get_size(ta->params), 0);

  module_dispose(mod);
}

TEST_F(it_def_collector, type_alias_def_with_generic) {
  const char *source = "type Vec3[T] = Vec[Vec[Vec[T]]];";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "Vec3");
  ASSERT_NE(name, nullptr);
  ASSERT_NE(name->ref, nullptr);

  type_alias_def_t ta = (type_alias_def_t)name->ref;
  ASSERT_NE(ta->params, nullptr);
  EXPECT_EQ(strmap_get_size(ta->params), 1);
  param_def_t pd = (param_def_t)strmap_find(ta->params, "T");
  ASSERT_NE(pd, nullptr);

  module_dispose(mod);
}

TEST_F(it_def_collector, type_alias_def_multi_generic) {
  const char *source = "type Pair[A, B] = struct { first: A; second: B; };";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "Pair");
  ASSERT_NE(name, nullptr);

  type_alias_def_t ta = (type_alias_def_t)name->ref;
  EXPECT_EQ(strmap_get_size(ta->params), 2);
  ASSERT_NE(strmap_find(ta->params, "A"), nullptr);
  ASSERT_NE(strmap_find(ta->params, "B"), nullptr);

  module_dispose(mod);
}

/* ---- import creates namespace def ---- */

TEST_F(it_def_collector, import_namespace_def) {
  const char *dep_path = "/tmp/cubec_def_import_dep.cubec";
  FILE *f = fopen(dep_path, "w");
  ASSERT_NE(f, nullptr);
  fputs("func helper(): void {}", f);
  fclose(f);

  const char *source = "import dep from \"/tmp/cubec_def_import_dep.cubec\";";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t name = find_name(mod, "dep");
  ASSERT_NE(name, nullptr);
  ASSERT_NE(name->ref, nullptr);

  def_t def = (def_t)name->ref;
  EXPECT_EQ(def->kind, DEF_NAMESPACE);
  namespace_def_t ns_def = (namespace_def_t)def;
  EXPECT_NE(ns_def->module, nullptr);

  module_dispose(mod);
}

/* ---- export def ref in exports table ---- */

TEST_F(it_def_collector, export_func_def_in_exports) {
  const char *source = "export func add(a: i32, b: i32): i32 { return a + b; }";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t exported = (name_t)strmap_find(mod->exports, "add");
  ASSERT_NE(exported, nullptr);
  ASSERT_NE(exported->ref, nullptr);

  def_t def = (def_t)exported->ref;
  EXPECT_EQ(def->kind, DEF_FUNC);

  module_dispose(mod);
}

/* ---- export * copies def refs from dependency ---- */

TEST_F(it_def_collector, re_export_star_def_refs) {
  const char *dep_path = "/tmp/cubec_def_reexport_dep.cubec";
  FILE *f = fopen(dep_path, "w");
  ASSERT_NE(f, nullptr);
  fputs("export func dep_func(): void {}", f);
  fclose(f);

  const char *source = "export * from \"/tmp/cubec_def_reexport_dep.cubec\";";
  module_t mod = parse_and_collect(source, "test.cubec");

  name_t exported = (name_t)strmap_find(mod->exports, "dep_func");
  ASSERT_NE(exported, nullptr);
  ASSERT_NE(exported->ref, nullptr);

  def_t def = (def_t)exported->ref;
  EXPECT_EQ(def->kind, DEF_FUNC);

  module_dispose(mod);
}

/* ---- mixed declarations ---- */

TEST_F(it_def_collector, mixed_declarations) {
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
  ASSERT_NE(n_count->ref, nullptr);
  EXPECT_EQ(((def_t)n_count->ref)->kind, DEF_VAR);

  ASSERT_NE(n_inc, nullptr);
  ASSERT_NE(n_inc->ref, nullptr);
  EXPECT_EQ(((def_t)n_inc->ref)->kind, DEF_FUNC);

  ASSERT_NE(n_point, nullptr);
  ASSERT_NE(n_point->ref, nullptr);
  EXPECT_EQ(((def_t)n_point->ref)->kind, DEF_STRUCT);

  ASSERT_NE(n_vec3, nullptr);
  ASSERT_NE(n_vec3->ref, nullptr);
  EXPECT_EQ(((def_t)n_vec3->ref)->kind, DEF_TYPE_ALIAS);

  module_dispose(mod);
}

/* ---- module state after def collection ---- */

TEST_F(it_def_collector, module_state_resolved) {
  const char *source = "func foo(): void {}";
  module_t mod = parse_and_collect(source, "test.cubec");

  EXPECT_EQ(mod->state, MODULE_RESOLVED);

  module_dispose(mod);
}

/* ---- idempotent: running again is a no-op ---- */

TEST_F(it_def_collector, idempotent) {
  const char *source = "func foo(): void {}";
  module_t mod = parse_and_collect(source, "test.cubec");

  def_t first_ref = (def_t)find_name(mod, "foo")->ref;
  def_collector_run(ctx, mod); /* should be a no-op */
  def_t second_ref = (def_t)find_name(mod, "foo")->ref;
  EXPECT_EQ(first_ref, second_ref);

  module_dispose(mod);
}
