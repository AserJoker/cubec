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
#include "engine/context.h"
#include "engine/value.h"
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

/* ---- value creation with raw buffer ---- */

TEST_F(it_primitive_type, create_int_value) {
  value_t val = context_create_int_value(ctx, ctx->t_i32, 42);
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(val->stype, ctx->t_i32);
  EXPECT_NE(val->data, nullptr);
  /* Read back the raw bytes */
  int32_t raw;
  memcpy(&raw, val->data, sizeof(int32_t));
  EXPECT_EQ(raw, 42);
  allocator_free(allocator, &val);
}

TEST_F(it_primitive_type, create_i64_value) {
  value_t val = context_create_int_value(ctx, ctx->t_i64, (uint64_t)-1);
  ASSERT_NE(val, nullptr);
  uint64_t raw;
  memcpy(&raw, val->data, sizeof(uint64_t));
  EXPECT_EQ(raw, (uint64_t)-1);
  allocator_free(allocator, &val);
}

TEST_F(it_primitive_type, create_float_value) {
  value_t val = context_create_float_value(ctx, ctx->t_f64, 3.14);
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(val->stype, ctx->t_f64);
  double raw;
  memcpy(&raw, val->data, sizeof(double));
  EXPECT_DOUBLE_EQ(raw, 3.14);
  allocator_free(allocator, &val);
}

TEST_F(it_primitive_type, create_bool_value) {
  value_t val_true = context_create_bool_value(ctx, ctx->t_bool, true);
  ASSERT_NE(val_true, nullptr);
  bool raw;
  memcpy(&raw, val_true->data, sizeof(bool));
  EXPECT_TRUE(raw);

  value_t val_false = context_create_bool_value(ctx, ctx->t_bool, false);
  memcpy(&raw, val_false->data, sizeof(bool));
  EXPECT_FALSE(raw);

  allocator_free(allocator, &val_true);
  allocator_free(allocator, &val_false);
}

TEST_F(it_primitive_type, create_char_value) {
  value_t val = context_create_char_value(ctx, ctx->t_char, 'A');
  ASSERT_NE(val, nullptr);
  uint8_t raw;
  memcpy(&raw, val->data, sizeof(uint8_t));
  EXPECT_EQ(raw, 'A');
  allocator_free(allocator, &val);
}

TEST_F(it_primitive_type, create_nil_value) {
  value_t val = context_create_nil_value(ctx, ctx->t_nil);
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(val->stype, ctx->t_nil);
  /* nil data is NULL (no payload) */
  EXPECT_EQ(val->data, nullptr);
  allocator_free(allocator, &val);
}

TEST_F(it_primitive_type, value_type_hash_for_primitive) {
  value_t val = context_create_int_value(ctx, ctx->t_i32, 0);
  EXPECT_EQ(val->type_hash, ctx->t_i32->instance.hash);
  allocator_free(allocator, &val);
}

/* ---- rbtree lookup ---- */

TEST_F(it_primitive_type, rbtree_lookup_by_hash) {
  /* Verify we can find a primitive type by its hash via rbtree */
  stype_t i32 = ctx->t_i32;
  void *found = rbtree_find(ctx->types, i32->instance.hash);
  ASSERT_NE(found, nullptr);
  EXPECT_EQ((stype_t)found, i32);
}

TEST_F(it_primitive_type, rbtree_all_primitives_findable) {
  stype_t types[] = {ctx->t_void, ctx->t_bool, ctx->t_i8, ctx->t_i16,
                     ctx->t_i32, ctx->t_i64, ctx->t_u8, ctx->t_u16,
                     ctx->t_u32, ctx->t_u64, ctx->t_f16, ctx->t_f32,
                     ctx->t_f64, ctx->t_char, ctx->t_str, ctx->t_nil};
  for (int i = 0; i < 16; i++) {
    void *found = rbtree_find(ctx->types, types[i]->instance.hash);
    ASSERT_NE(found, nullptr) << "type not found in rbtree: " << types[i]->instance.name;
    EXPECT_EQ((stype_t)found, types[i]);
  }
}

TEST_F(it_primitive_type, all_basic_types_registered) {
  const char *names[] = {"void", "bool", "char", "str"};
  for (int i = 0; i < 4; i++) {
    name_t name = scope_lookup(ctx->global_scope, names[i]);
    ASSERT_NE(name, nullptr) << "missing type: " << names[i];
  }
}
