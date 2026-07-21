#include "engine/checker.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/semantic_type.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_resolver : public CubecTest {
protected:
  TEST_ALLOCATOR;
  checker_t ctx;

  void SetUp() override {
    CubecTest::SetUp();
    ctx = checker_create(allocator);
  }

  void TearDown() override {
    checker_dispose(ctx);
    CubecTest::TearDown();
  }
};

/* Test resolver via checker context — the resolver is tested through
   checker's type_name_table and scope infrastructure, since creating
   AST nodes from C++ tests requires C-only internal types. */

TEST_F(dt_resolver, builtin_lookup_via_name_table) {
  /* Verify the type_name_table works for resolver lookups */
  void *found = strmap_find(ctx->type_name_table, "i32");
  ASSERT_NE(found, nullptr);
  semantic_type_t t = (semantic_type_t)found;
  EXPECT_EQ(semantic_type_get_kind(t), TYPE_I32);
  EXPECT_EQ(semantic_type_get_size(t), 4u);
}

TEST_F(dt_resolver, all_builtins_in_name_table) {
  const char *names[] = {"void", "bool", "i8",   "i16",  "i32",
                          "i64", "u8",   "u16",  "u32",  "u64",
                          "f16", "f32",  "f64",  "char", "str", "nil"};
  for (auto name : names) {
    void *found = strmap_find(ctx->type_name_table, name);
    EXPECT_NE(found, nullptr) << "builtin '" << name << "' not in name table";
  }
}

TEST_F(dt_resolver, unknown_type_not_in_name_table) {
  void *found = strmap_find(ctx->type_name_table, "UnknownType");
  EXPECT_EQ(found, nullptr);
}

TEST_F(dt_resolver, pointer_type_construction) {
  /* Test that we can construct pointer types using checker context */
  semantic_type_t ptr_i32 =
      semantic_type_create_pointer(allocator, ctx->builtin_i32);
  type_hash_ensure(ptr_i32);
  type_layout_compute(ptr_i32, 8);

  EXPECT_EQ(semantic_type_get_kind(ptr_i32), TYPE_POINTER);
  EXPECT_EQ(semantic_type_get_size(ptr_i32), 8u);
  EXPECT_EQ(semantic_type_get_alignment(ptr_i32), 8u);

  allocator_free(allocator, &ptr_i32);
}

TEST_F(dt_resolver, slice_type_construction) {
  semantic_type_t sl_u8 =
      semantic_type_create_slice(allocator, ctx->builtin_u8);
  type_hash_ensure(sl_u8);
  type_layout_compute(sl_u8, 8);

  EXPECT_EQ(semantic_type_get_kind(sl_u8), TYPE_SLICE);
  EXPECT_EQ(semantic_type_get_size(sl_u8), 16u);

  allocator_free(allocator, &sl_u8);
}

TEST_F(dt_resolver, qualifier_type_construction) {
  semantic_type_t const_i32 =
      semantic_type_create_qualifier(allocator, ctx->builtin_i32, true, false);
  type_hash_ensure(const_i32);
  type_layout_compute(const_i32, 8);

  EXPECT_EQ(semantic_type_get_kind(const_i32), TYPE_QUALIFIER);
  EXPECT_EQ(semantic_type_get_size(const_i32), 4u);

  allocator_free(allocator, &const_i32);
}

TEST_F(dt_resolver, function_type_construction) {
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(allocator, &g_vec_type, &vi);
  vec_push(params, ctx->builtin_i32);
  vec_push(params, ctx->builtin_f64);

  semantic_type_t ft = semantic_type_create_function(
      allocator, ctx->builtin_void, params, false);
  type_hash_ensure(ft);

  EXPECT_EQ(semantic_type_get_kind(ft), TYPE_FUNCTION);
  EXPECT_FALSE(ft->is_incomplete);

  allocator_free(allocator, &ft);
}
