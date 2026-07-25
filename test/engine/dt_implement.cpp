#include "engine/context.h"
#include "engine/comptime_eval.h"
#include "engine/comptime_value.h"
#include "engine/diagnostic.h"
#include "engine/symbol.h"
#include "engine/semantic_type.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "common/test_common.h"
#include <gtest/gtest.h>
#include <string>

using ::testing::Test;

/* ===== helpers ===== */

struct compile_result {
  context_t ctx;
  node_t prog;
  vec_t tokens;
};

static struct compile_result compile_source(context_t ctx,
                                            const char *source) {
  allocator_t allocator = ctx->allocator;
  vec_t tokens = resolve_token_list(ctx, "test.cubec", source);
  size_t pos = 0;
  node_t prog = read_program_node(ctx, tokens, &pos, "test.cubec");

  if (!prog || !tokens) {
    GTEST_MESSAGE_AT_(__FILE__, __LINE__,
        "Parsing failed",
        ::testing::TestPartResult::kFatalFailure);
    return (struct compile_result){NULL, prog, tokens};
  }

  source_cache_load(ctx->sources, "test.cubec", source, false);

  context_check_program(ctx, prog);
  return (struct compile_result){ctx, prog, tokens};
}

static void compile_result_cleanup(struct compile_result *r,
                                   allocator_t allocator) {
  allocator_free(allocator, &r->prog);
  allocator_free(allocator, &r->tokens);
}

/* ===== test fixture ===== */

class dt_implement : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== struct implement satisfied ===== */

TEST_F(dt_implement, struct_satisfied) {
  const char *src =
      "interface Printable {\n"
      "    func to_string(self): str;\n"
      "}\n"
      "struct Foo implement Printable {\n"
      "    func to_string(self): str { return \"Foo\"; }\n"
      "}\n";
  struct compile_result r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);

  /* Verify implements vec is populated */
  struct symbol *sym = scope_lookup_local(r.ctx->global_scope, "Foo");
  ASSERT_NE(sym, nullptr);
  ASSERT_EQ(sym->kind, SYMBOL_TYPE);
  semantic_type_t t = sym->type.type;
  ASSERT_NE(t, nullptr);
  ASSERT_NE(t->implements, nullptr);
  EXPECT_EQ(vec_get_size(t->implements), 1u);

  compile_result_cleanup(&r, allocator);
}

/* ===== struct implement missing method ===== */

TEST_F(dt_implement, struct_missing_method) {
  const char *src =
      "interface Printable {\n"
      "    func to_string(self): str;\n"
      "}\n"
      "struct Foo implement Printable {\n"
      "}\n";
  struct compile_result r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0u);

  compile_result_cleanup(&r, allocator);
}

/* ===== struct implement non-interface type ===== */

TEST_F(dt_implement, struct_non_interface) {
  const char *src =
      "struct Foo implement i32 {\n"
      "}\n";
  struct compile_result r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_GT(r.ctx->error_count, 0u);

  compile_result_cleanup(&r, allocator);
}

/* ===== struct implement multiple interfaces ===== */

TEST_F(dt_implement, struct_multiple) {
  const char *src =
      "interface A {\n"
      "    func a(self): i32;\n"
      "}\n"
      "interface B {\n"
      "    func b(self): f64;\n"
      "}\n"
      "struct Foo implement A, B {\n"
      "    func a(self): i32 { return 1; }\n"
      "    func b(self): f64 { return 2.0; }\n"
      "}\n";
  struct compile_result r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);

  struct symbol *sym = scope_lookup_local(r.ctx->global_scope, "Foo");
  ASSERT_NE(sym, nullptr);
  semantic_type_t t = sym->type.type;
  ASSERT_NE(t->implements, nullptr);
  EXPECT_EQ(vec_get_size(t->implements), 2u);

  compile_result_cleanup(&r, allocator);
}

/* ===== union implement satisfied ===== */

TEST_F(dt_implement, union_satisfied) {
  const char *src =
      "interface HasValue {\n"
      "    func get_value(self): i32;\n"
      "}\n"
      "union Option implement HasValue {\n"
      "    value: i32;\n"
      "    empty: void;\n"
      "    func get_value(self): i32 { return 0; }\n"
      "}\n";
  struct compile_result r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);

  struct symbol *sym = scope_lookup_local(r.ctx->global_scope, "Option");
  ASSERT_NE(sym, nullptr);
  semantic_type_t t = sym->type.type;
  ASSERT_NE(t->implements, nullptr);
  EXPECT_EQ(vec_get_size(t->implements), 1u);

  compile_result_cleanup(&r, allocator);
}

/* ===== struct implement generic interface ===== */

TEST_F(dt_implement, struct_generic_interface) {
  const char *src =
      "interface Container[T] {\n"
      "    func get(self): T;\n"
      "}\n"
      "struct Box[T] implement Container[T] {\n"
      "    value: T;\n"
      "    func get(self): T { return self.value; }\n"
      "}\n";
  struct compile_result r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  /* Generic structs resolve interface type but skip constraint check */
  EXPECT_EQ(r.ctx->error_count, 0u);

  /* Verify implements vec is populated even for generic */
  struct symbol *sym = scope_lookup_local(r.ctx->global_scope, "Box");
  ASSERT_NE(sym, nullptr);
  ASSERT_EQ(sym->kind, SYMBOL_TYPE);
  semantic_type_t t = sym->type.type;
  ASSERT_NE(t->implements, nullptr);
  EXPECT_EQ(vec_get_size(t->implements), 1u);

  compile_result_cleanup(&r, allocator);
}

/* ===== struct without implement — no implements vec ===== */

TEST_F(dt_implement, struct_no_implement) {
  const char *src =
      "struct Foo {\n"
      "    x: i32;\n"
      "}\n";
  struct compile_result r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);

  struct symbol *sym = scope_lookup_local(r.ctx->global_scope, "Foo");
  ASSERT_NE(sym, nullptr);
  semantic_type_t t = sym->type.type;
  EXPECT_EQ(t->implements, nullptr);

  compile_result_cleanup(&r, allocator);
}

/* ===== multi-constraint extends: T extends A & B ===== */

TEST_F(dt_implement, multi_constraint_satisfied) {
  const char *src =
      "interface Printable {\n"
      "    func to_string(self): str;\n"
      "}\n"
      "interface Serializable {\n"
      "    func serialize(self): str;\n"
      "}\n"
      "struct Foo implement Printable, Serializable {\n"
      "    func to_string(self): str { return \"Foo\"; }\n"
      "    func serialize(self): str { return \"Foo\"; }\n"
      "}\n"
      "func process[T extends Printable & Serializable](x: T): str {\n"
      "    return x.to_string();\n"
      "}\n";
  struct compile_result r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  EXPECT_EQ(r.ctx->error_count, 0u);

  compile_result_cleanup(&r, allocator);
}

TEST_F(dt_implement, multi_constraint_partial_fail) {
  const char *src =
      "interface Printable {\n"
      "    func to_string(self): str;\n"
      "}\n"
      "interface Serializable {\n"
      "    func serialize(self): str;\n"
      "}\n"
      "struct Foo implement Printable {\n"
      "    func to_string(self): str { return \"Foo\"; }\n"
      "}\n"
      "func process[T extends Printable & Serializable](x: T): str {\n"
      "    return x.to_string();\n"
      "}\n"
      "test \"t\" { process(.Foo {}); }\n";
  struct compile_result r = compile_source(ctx, src);
  ASSERT_NE(r.ctx, nullptr);
  /* Foo only implements Printable, not Serializable — should fail */
  EXPECT_GT(r.ctx->error_count, 0u);

  compile_result_cleanup(&r, allocator);
}
