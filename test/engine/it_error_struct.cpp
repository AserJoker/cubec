#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/struct_type.h"
#include "engine/integer_type.h"
#include "engine/array_type.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_error_struct : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);
};

/* ---- Type availability ---- */

TEST_F(it_error_struct, vm_has_error_type) {
  vm_t vm = vm_create(allocator);
  value_t etv = vm_get_error_type(vm);
  EXPECT_NE(etv, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(etv)), TYPE_KIND_TYPE);

  type_t error_type = (type_t)value_get_data(etv);
  EXPECT_EQ(type_get_kind(error_type), TYPE_KIND_STRUCT);
  EXPECT_STREQ(type_get_name(error_type), "error");

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Field layout ---- */

TEST_F(it_error_struct, field_count_and_names) {
  vm_t vm = vm_create(allocator);
  type_t error_type = (type_t)value_get_data(vm_get_error_type(vm));
  struct_type_t st = (struct_type_t)error_type;

  EXPECT_TRUE(struct_type_is_sealed(st));
  vec_t fields = struct_type_get_fields(st);
  EXPECT_EQ(vec_get_size(fields), 4u);

  field_info_t f0 = (field_info_t)vec_get(fields, 0);
  EXPECT_STREQ(field_info_get_name(f0), "message");
  EXPECT_EQ(type_get_kind(field_info_get_type(f0)), TYPE_KIND_ARRAY);

  field_info_t f1 = (field_info_t)vec_get(fields, 1);
  EXPECT_STREQ(field_info_get_name(f1), "error_code");
  EXPECT_EQ(type_get_kind(field_info_get_type(f1)), TYPE_KIND_U64);

  field_info_t f2 = (field_info_t)vec_get(fields, 2);
  EXPECT_STREQ(field_info_get_name(f2), "backtrace");
  EXPECT_EQ(type_get_kind(field_info_get_type(f2)), TYPE_KIND_ARRAY);

  field_info_t f3 = (field_info_t)vec_get(fields, 3);
  EXPECT_STREQ(field_info_get_name(f3), "backtrace_count");
  EXPECT_EQ(type_get_kind(field_info_get_type(f3)), TYPE_KIND_U64);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Array field dimensions ---- */

TEST_F(it_error_struct, message_is_128_u8_array) {
  vm_t vm = vm_create(allocator);
  type_t error_type = (type_t)value_get_data(vm_get_error_type(vm));
  struct_type_t st = (struct_type_t)error_type;
  vec_t fields = struct_type_get_fields(st);

  field_info_t f0 = (field_info_t)vec_get(fields, 0);
  array_type_t at = (array_type_t)field_info_get_type(f0);
  EXPECT_EQ(array_type_get_count(at), 128u);
  EXPECT_EQ(type_get_kind(array_type_get_element_type(at)), TYPE_KIND_U8);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_error_struct, backtrace_is_32_u64_array) {
  vm_t vm = vm_create(allocator);
  type_t error_type = (type_t)value_get_data(vm_get_error_type(vm));
  struct_type_t st = (struct_type_t)error_type;
  vec_t fields = struct_type_get_fields(st);

  field_info_t f2 = (field_info_t)vec_get(fields, 2);
  array_type_t at = (array_type_t)field_info_get_type(f2);
  EXPECT_EQ(array_type_get_count(at), 32u);
  EXPECT_EQ(type_get_kind(array_type_get_element_type(at)), TYPE_KIND_U64);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Error struct size ---- */

TEST_F(it_error_struct, struct_size_is_nonzero) {
  vm_t vm = vm_create(allocator);
  type_t error_type = (type_t)value_get_data(vm_get_error_type(vm));

  /* message[128] = 128 bytes
   * error_code: u64 = 8 bytes
   * backtrace[32]: 32 * 8 = 256 bytes
   * backtrace_count: u64 = 8 bytes
   * Total (no padding worst case) = 400 */
  uint64_t sz = type_get_size(error_type);
  EXPECT_GE(sz, 400u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Create error value ---- */

TEST_F(it_error_struct, create_error_value) {
  vm_t vm = vm_create(allocator);
  type_t error_type = (type_t)value_get_data(vm_get_error_type(vm));
  struct_type_t st = (struct_type_t)error_type;

  /* create a struct value with zeroed data */
  value_t ev = create_struct_value(vm, st, NULL);
  EXPECT_NE(ev, nullptr);
  EXPECT_EQ(value_get_type(ev), error_type);
  EXPECT_TRUE(value_is_own(ev));

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Scope lookup ---- */

TEST_F(it_error_struct, error_name_in_global_scope) {
  vm_t vm = vm_create(allocator);
  scope_t global = vm_get_global_scope(vm);
  name_t n = scope_lookup(global, "error");
  EXPECT_NE(n, nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
