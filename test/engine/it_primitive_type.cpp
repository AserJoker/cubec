#include "engine/void_type.h"
#include "engine/bool_type.h"
#include "engine/char_type.h"
#include "engine/str_type.h"
#include "engine/nil_type.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "engine/stype.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_primitive_type : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ---- scope_lookup finds basic types ---- */

TEST_F(it_primitive_type, lookup_void) {
  name_t name = scope_lookup(ctx->global_scope, "void");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_TYPE);
  stype_t type = (stype_t)name->ref;
  EXPECT_EQ(type->type_kind, TYPE_VOID);
  EXPECT_STREQ(type->instance.name, "void");
  EXPECT_EQ(type->instance.size, 0);
}

TEST_F(it_primitive_type, lookup_bool) {
  name_t name = scope_lookup(ctx->global_scope, "bool");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_TYPE);
  stype_t type = (stype_t)name->ref;
  EXPECT_EQ(type->type_kind, TYPE_BOOL);
  EXPECT_STREQ(type->instance.name, "bool");
  EXPECT_EQ(type->instance.size, 1);
}

TEST_F(it_primitive_type, lookup_char) {
  name_t name = scope_lookup(ctx->global_scope, "char");
  ASSERT_NE(name, nullptr);
  stype_t type = (stype_t)name->ref;
  EXPECT_EQ(type->type_kind, TYPE_CHAR);
  EXPECT_EQ(type->instance.size, 4);
}

TEST_F(it_primitive_type, lookup_str) {
  name_t name = scope_lookup(ctx->global_scope, "str");
  ASSERT_NE(name, nullptr);
  stype_t type = (stype_t)name->ref;
  EXPECT_EQ(type->type_kind, TYPE_STR);
}

TEST_F(it_primitive_type, lookup_nil_not_in_scope) {
  /* nil type is internal, not visible to cubec code */
  name_t name = scope_lookup(ctx->global_scope, "nil");
  EXPECT_EQ(name, nullptr);
}

/* ---- scope_lookup finds integer types ---- */

TEST_F(it_primitive_type, lookup_i32) {
  name_t name = scope_lookup(ctx->global_scope, "i32");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_TYPE);
  stype_t type = (stype_t)name->ref;
  EXPECT_EQ(type->type_kind, TYPE_I32);
  EXPECT_STREQ(type->instance.name, "i32");
  EXPECT_EQ(type->instance.size, 4);
  EXPECT_EQ(type->instance.align, 4);
}

TEST_F(it_primitive_type, lookup_u64) {
  name_t name = scope_lookup(ctx->global_scope, "u64");
  ASSERT_NE(name, nullptr);
  stype_t type = (stype_t)name->ref;
  EXPECT_EQ(type->type_kind, TYPE_U64);
  EXPECT_EQ(type->instance.size, 8);
}

/* ---- scope_lookup finds float types ---- */

TEST_F(it_primitive_type, lookup_f64) {
  name_t name = scope_lookup(ctx->global_scope, "f64");
  ASSERT_NE(name, nullptr);
  stype_t type = (stype_t)name->ref;
  EXPECT_EQ(type->type_kind, TYPE_F64);
  EXPECT_EQ(type->instance.size, 8);
}

TEST_F(it_primitive_type, lookup_f32) {
  name_t name = scope_lookup(ctx->global_scope, "f32");
  ASSERT_NE(name, nullptr);
  stype_t type = (stype_t)name->ref;
  EXPECT_EQ(type->type_kind, TYPE_F32);
  EXPECT_EQ(type->instance.size, 4);
}

/* ---- type_get functions ---- */

TEST_F(it_primitive_type, void_type_get) {
  stype_t type = void_type_get(ctx);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(type->type_kind, TYPE_VOID);
}

TEST_F(it_primitive_type, bool_type_get) {
  stype_t type = bool_type_get(ctx);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(type->type_kind, TYPE_BOOL);
}

TEST_F(it_primitive_type, char_type_get) {
  stype_t type = char_type_get(ctx);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(type->type_kind, TYPE_CHAR);
}

TEST_F(it_primitive_type, str_type_get) {
  stype_t type = str_type_get(ctx);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(type->type_kind, TYPE_STR);
}

TEST_F(it_primitive_type, nil_type_get) {
  stype_t type = nil_type_get(ctx);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(type->type_kind, TYPE_NIL);
  EXPECT_EQ(type->instance.size, 8);
  EXPECT_EQ(type->instance.align, 8);
}

TEST_F(it_primitive_type, integer_type_get_i16) {
  stype_t type = integer_type_get(ctx, TYPE_I16);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(type->type_kind, TYPE_I16);
  EXPECT_EQ(type->instance.size, 2);
}

TEST_F(it_primitive_type, float_type_get_f16) {
  stype_t type = float_type_get(ctx, TYPE_F16);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(type->type_kind, TYPE_F16);
  EXPECT_EQ(type->instance.size, 2);
}

/* ---- type_kind predicates ---- */

TEST_F(it_primitive_type, type_kind_is_integer) {
  EXPECT_TRUE(type_kind_is_integer(TYPE_I32));
  EXPECT_TRUE(type_kind_is_integer(TYPE_U64));
  EXPECT_FALSE(type_kind_is_integer(TYPE_F64));
  EXPECT_FALSE(type_kind_is_integer(TYPE_BOOL));
}

TEST_F(it_primitive_type, type_kind_is_float) {
  EXPECT_TRUE(type_kind_is_float(TYPE_F32));
  EXPECT_FALSE(type_kind_is_float(TYPE_I32));
}

/* ---- child scope lookup falls through to global ---- */

TEST_F(it_primitive_type, child_scope_lookup_finds_i32) {
  scope_t child = scope_create(allocator, SCOPE_BLOCK, ctx->global_scope, NULL);
  name_t name = scope_lookup(child, "i32");
  ASSERT_NE(name, nullptr);
  EXPECT_EQ(name->kind, NAME_TYPE);
  stype_t type = (stype_t)name->ref;
  EXPECT_EQ(type->type_kind, TYPE_I32);
  allocator_free(allocator, &child);
}

/* ---- all primitive types registered ---- */

TEST_F(it_primitive_type, all_integer_types_registered) {
  const char *names[] = {"i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64"};
  for (int i = 0; i < 8; i++) {
    name_t name = scope_lookup(ctx->global_scope, names[i]);
    ASSERT_NE(name, nullptr) << "missing type: " << names[i];
    EXPECT_EQ(name->kind, NAME_TYPE) << names[i];
  }
}

TEST_F(it_primitive_type, all_float_types_registered) {
  const char *names[] = {"f16", "f32", "f64"};
  for (int i = 0; i < 3; i++) {
    name_t name = scope_lookup(ctx->global_scope, names[i]);
    ASSERT_NE(name, nullptr) << "missing type: " << names[i];
  }
}

TEST_F(it_primitive_type, all_basic_types_registered) {
  const char *names[] = {"void", "bool", "char", "str"};
  for (int i = 0; i < 4; i++) {
    name_t name = scope_lookup(ctx->global_scope, names[i]);
    ASSERT_NE(name, nullptr) << "missing type: " << names[i];
  }
}
