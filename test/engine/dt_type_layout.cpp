#include "engine/semantic_type.h"
#include "engine/type_layout.h"
#include "engine/symbol.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_type_layout : public CubecTest {
protected:
  TEST_ALLOCATOR;
  static constexpr size_t PTR64 = 8;
};

TEST_F(dt_type_layout, primitive_sizes) {
  semantic_type_t i32 =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  type_layout_compute(i32, PTR64);
  EXPECT_EQ(semantic_type_get_size(i32), 4u);
  EXPECT_EQ(semantic_type_get_alignment(i32), 4u);

  semantic_type_t u8 =
      semantic_type_create_named(allocator, "u8", TYPE_U8);
  type_layout_compute(u8, PTR64);
  EXPECT_EQ(semantic_type_get_size(u8), 1u);
  EXPECT_EQ(semantic_type_get_alignment(u8), 1u);

  semantic_type_t f64 =
      semantic_type_create_named(allocator, "f64", TYPE_F64);
  type_layout_compute(f64, PTR64);
  EXPECT_EQ(semantic_type_get_size(f64), 8u);
  EXPECT_EQ(semantic_type_get_alignment(f64), 8u);

  allocator_free(allocator, &f64);
  allocator_free(allocator, &u8);
  allocator_free(allocator, &i32);
}

TEST_F(dt_type_layout, pointer_size) {
  semantic_type_t base =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t ptr = semantic_type_create_pointer(allocator, base);
  type_layout_compute(ptr, PTR64);
  EXPECT_EQ(semantic_type_get_size(ptr), 8u);
  EXPECT_EQ(semantic_type_get_alignment(ptr), 8u);

  allocator_free(allocator, &ptr);
  allocator_free(allocator, &base);
}

TEST_F(dt_type_layout, pointer_size_32bit) {
  semantic_type_t base =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t ptr = semantic_type_create_pointer(allocator, base);
  type_layout_compute(ptr, 4);
  EXPECT_EQ(semantic_type_get_size(ptr), 4u);
  EXPECT_EQ(semantic_type_get_alignment(ptr), 4u);

  allocator_free(allocator, &ptr);
  allocator_free(allocator, &base);
}

TEST_F(dt_type_layout, slice_size) {
  semantic_type_t elem =
      semantic_type_create_named(allocator, "u8", TYPE_U8);
  semantic_type_t sl = semantic_type_create_slice(allocator, elem);
  type_layout_compute(sl, PTR64);
  /* slice = { data, start, length } = 3 * 8 = 24 */
  EXPECT_EQ(semantic_type_get_size(sl), 24u);
  EXPECT_EQ(semantic_type_get_alignment(sl), 8u);

  allocator_free(allocator, &sl);
  allocator_free(allocator, &elem);
}

TEST_F(dt_type_layout, array_size) {
  semantic_type_t elem =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  type_layout_compute(elem, PTR64);
  semantic_type_t arr = semantic_type_create_array(allocator, elem, 10, NULL);
  type_layout_compute(arr, PTR64);
  EXPECT_EQ(semantic_type_get_size(arr), 40u);
  EXPECT_EQ(semantic_type_get_alignment(arr), 4u);

  allocator_free(allocator, &arr);
  allocator_free(allocator, &elem);
}

TEST_F(dt_type_layout, struct_layout) {
  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 1, NULL},
                    .end = {1, 1, NULL}};

  /* struct { u8 a; i32 b; } — expect padding after a */
  semantic_type_t st =
      semantic_type_create_named(allocator, "S", TYPE_STRUCT);

  struct symbol *f1 = symbol_create(allocator, "a", SYMBOL_FIELD, loc);
  f1->field.type = semantic_type_create_named(allocator, "u8", TYPE_U8);
  type_layout_compute(f1->field.type, PTR64);

  struct symbol *f2 = symbol_create(allocator, "b", SYMBOL_FIELD, loc);
  f2->field.type = semantic_type_create_named(allocator, "i32", TYPE_I32);
  type_layout_compute(f2->field.type, PTR64);

  vec_init_t vi = {.auto_dispose = false};
  st->impl->struct_type.fields =
      (vec_t)allocator_create(allocator, &g_vec_type, &vi);
  vec_push(st->impl->struct_type.fields, f1);
  vec_push(st->impl->struct_type.fields, f2);

  type_layout_compute(st, PTR64);

  EXPECT_EQ(f1->field.offset, 0u);
  EXPECT_EQ(f2->field.offset, 4u); /* 1 + 3 padding = 4 */
  EXPECT_EQ(semantic_type_get_size(st), 8u);  /* 4 + 4 = 8 */
  EXPECT_EQ(semantic_type_get_alignment(st), 4u);

  allocator_free(allocator, &f2->field.type);
  allocator_free(allocator, &f1->field.type);
  allocator_free(allocator, &f2);
  allocator_free(allocator, &f1);
  allocator_free(allocator, &st);
}

TEST_F(dt_type_layout, union_layout) {
  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 1, NULL},
                    .end = {1, 1, NULL}};

  semantic_type_t un =
      semantic_type_create_named(allocator, "U", TYPE_UNION);

  struct symbol *f1 = symbol_create(allocator, "a", SYMBOL_FIELD, loc);
  f1->field.type = semantic_type_create_named(allocator, "u8", TYPE_U8);
  type_layout_compute(f1->field.type, PTR64);

  struct symbol *f2 = symbol_create(allocator, "b", SYMBOL_FIELD, loc);
  f2->field.type = semantic_type_create_named(allocator, "i32", TYPE_I32);
  type_layout_compute(f2->field.type, PTR64);

  vec_init_t vi = {.auto_dispose = false};
  un->impl->struct_type.fields =
      (vec_t)allocator_create(allocator, &g_vec_type, &vi);
  vec_push(un->impl->struct_type.fields, f1);
  vec_push(un->impl->struct_type.fields, f2);

  type_layout_compute(un, PTR64);

  /* Tagged union: fields start at offset 8 (after u64 tag),
     size = 8 + align_up(max_field_size, alignment) */
  EXPECT_EQ(f1->field.offset, 8u);
  EXPECT_EQ(f2->field.offset, 8u);
  EXPECT_EQ(semantic_type_get_size(un), 16u); /* 8 (tag) + align_up(4, 8) = 16 */
  EXPECT_EQ(semantic_type_get_alignment(un), 8u);

  allocator_free(allocator, &f2->field.type);
  allocator_free(allocator, &f1->field.type);
  allocator_free(allocator, &f2);
  allocator_free(allocator, &f1);
  allocator_free(allocator, &un);
}

TEST_F(dt_type_layout, enum_default_i32) {
  semantic_type_t en =
      semantic_type_create_named(allocator, "E", TYPE_ENUM);
  type_layout_compute(en, PTR64);
  EXPECT_EQ(semantic_type_get_size(en), 4u);
  EXPECT_EQ(semantic_type_get_alignment(en), 4u);

  allocator_free(allocator, &en);
}
