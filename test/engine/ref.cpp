#include "engine/ref.h"
#include "common/cubec_test.hpp"
#include "core/allocator.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <gtest/gtest.h>
#include <string>
class ref_test : public cubec_test {};
TEST_F(ref_test, ref_get) {
  context_t ctx = create_context(allocator);
  value_t val = create_comptime_bool(ctx, true, false, NULL);
  value_t ref = create_ref_value(ctx, val);
  value_t val2 = ref_get_value(ctx, ref);
  ASSERT_EQ((bool *)value_get_data(val2), (bool *)value_get_data(val));
  allocator_free(allocator, ctx);
}
TEST_F(ref_test, mul_ref_get) {
  context_t ctx = create_context(allocator);
  value_t val = create_comptime_bool(ctx, true, false, NULL);
  value_t ref = create_ref_value(ctx, val);
  value_t ref2 = create_ref_value(ctx, ref);
  value_t val2 = ref_get_value(ctx, ref2);
  ASSERT_EQ((bool *)value_get_data(val2), (bool *)value_get_data(val));
  allocator_free(allocator, ctx);
}

TEST_F(ref_test, ref_name) {
  context_t ctx = create_context(allocator);
  type_t bool_t = context_load_type(ctx, "bool");
  type_t ref_t = create_ref_type(ctx, bool_t);
  std::string name = type_get_name(ref_t);
  ASSERT_EQ(name, "&bool");
  allocator_free(allocator, ctx);
}

TEST_F(ref_test, mul_ref_name) {
  context_t ctx = create_context(allocator);
  type_t bool_t = context_load_type(ctx, "bool");
  type_t ref_t = create_ref_type(ctx, bool_t);
  type_t ref2_t = create_ref_type(ctx, ref_t);
  std::string name = type_get_name(ref2_t);
  ASSERT_EQ(name, "&bool");
  allocator_free(allocator, ctx);
}