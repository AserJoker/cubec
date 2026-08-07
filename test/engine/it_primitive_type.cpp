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
  EXPECT_EQ(type->instance.size, 1);
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

/* ---- structural hash ---- */

TEST_F(it_primitive_type, primitive_hash_deterministic) {
  stype_t i32 = ctx->t_i32;
  stype_t f64 = ctx->t_f64;
  EXPECT_NE(i32->instance.hash, 0);
  EXPECT_NE(f64->instance.hash, 0);
  EXPECT_NE(i32->instance.hash, f64->instance.hash);
}

TEST_F(it_primitive_type, same_kind_same_hash) {
  uint64_t h1 = stype_compute_primitive_hash(TYPE_I32);
  uint64_t h2 = stype_compute_primitive_hash(TYPE_I32);
  EXPECT_EQ(h1, h2);
  EXPECT_EQ(h1, ctx->t_i32->instance.hash);
}

TEST_F(it_primitive_type, different_kind_different_hash) {
  uint64_t hi32 = stype_compute_primitive_hash(TYPE_I32);
  uint64_t hu32 = stype_compute_primitive_hash(TYPE_U32);
  uint64_t hf32 = stype_compute_primitive_hash(TYPE_F32);
  EXPECT_NE(hi32, hu32);
  EXPECT_NE(hi32, hf32);
  EXPECT_NE(hu32, hf32);
}

/* ---- comptime_value create / extract ---- */

TEST_F(it_primitive_type, integer_create_and_get_value) {
  comptime_value_t val = integer_type_create_value(ctx, TYPE_I32, 42);
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(val->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(integer_type_get_value(val), 42);
  comptime_value_dispose(val);
}

TEST_F(it_primitive_type, integer_negative_value) {
  comptime_value_t val = integer_type_create_value(ctx, TYPE_I32, (uint64_t)-1);
  EXPECT_EQ(integer_type_get_value(val), (uint64_t)-1);
  comptime_value_dispose(val);
}

TEST_F(it_primitive_type, integer_u64_max) {
  comptime_value_t val = integer_type_create_value(ctx, TYPE_U64, UINT64_MAX);
  EXPECT_EQ(integer_type_get_value(val), UINT64_MAX);
  comptime_value_dispose(val);
}

TEST_F(it_primitive_type, float_create_and_get_value) {
  comptime_value_t val = float_type_create_value(ctx, TYPE_F64, 3.14);
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(val->kind, COMPTIME_VALUE_FLOAT);
  EXPECT_DOUBLE_EQ(float_type_get_value(val), 3.14);
  comptime_value_dispose(val);
}

TEST_F(it_primitive_type, bool_create_and_get_value) {
  comptime_value_t val_true = bool_type_create_value(ctx, true);
  ASSERT_NE(val_true, nullptr);
  EXPECT_EQ(val_true->kind, COMPTIME_VALUE_BOOL);
  EXPECT_TRUE(bool_type_get_value(val_true));

  comptime_value_t val_false = bool_type_create_value(ctx, false);
  EXPECT_FALSE(bool_type_get_value(val_false));

  comptime_value_dispose(val_true);
  comptime_value_dispose(val_false);
}

TEST_F(it_primitive_type, char_create_and_get_value) {
  comptime_value_t val = char_type_create_value(ctx, 'A');
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(val->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(char_type_get_value(val), 'A');
  comptime_value_dispose(val);
}

TEST_F(it_primitive_type, str_create_and_get_value) {
  comptime_value_t val = str_type_create_value_cstr(ctx, "hello");
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(val->kind, COMPTIME_VALUE_STRING);
  string_t s = str_type_get_value(val);
  ASSERT_NE(s, nullptr);
  EXPECT_STREQ(string_get(s), "hello");
  comptime_value_dispose(val);
}

TEST_F(it_primitive_type, nil_create_value) {
  comptime_value_t val = nil_type_create_value(ctx);
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(val->kind, COMPTIME_VALUE_NIL);
  comptime_value_dispose(val);
}

TEST_F(it_primitive_type, comptime_value_clone_int) {
  comptime_value_t val = integer_type_create_value(ctx, TYPE_I32, 99);
  comptime_value_t cloned = comptime_value_clone(ctx->allocator, val);
  ASSERT_NE(cloned, nullptr);
  EXPECT_EQ(cloned->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(integer_type_get_value(cloned), 99);
  comptime_value_dispose(cloned);
  comptime_value_dispose(val);
}

TEST_F(it_primitive_type, comptime_value_clone_string) {
  comptime_value_t val = str_type_create_value_cstr(ctx, "test");
  comptime_value_t cloned = comptime_value_clone(ctx->allocator, val);
  ASSERT_NE(cloned, nullptr);
  string_t s = str_type_get_value(cloned);
  ASSERT_NE(s, nullptr);
  EXPECT_STREQ(string_get(s), "test");
  comptime_value_dispose(cloned);
  comptime_value_dispose(val);
}

TEST_F(it_primitive_type, comptime_value_type_points_to_stype) {
  comptime_value_t val = integer_type_create_value(ctx, TYPE_I32, 0);
  EXPECT_EQ(comptime_value_get_type(val), ctx->t_i32);
  comptime_value_dispose(val);

  comptime_value_t bval = bool_type_create_value(ctx, true);
  EXPECT_EQ(comptime_value_get_type(bval), ctx->t_bool);
  comptime_value_dispose(bval);
}

TEST_F(it_primitive_type, all_basic_types_registered) {
  const char *names[] = {"void", "bool", "char", "str"};
  for (int i = 0; i < 4; i++) {
    name_t name = scope_lookup(ctx->global_scope, names[i]);
    ASSERT_NE(name, nullptr) << "missing type: " << names[i];
  }
}
