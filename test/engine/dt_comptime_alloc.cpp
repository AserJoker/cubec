#include "engine/comptime_alloc.h"
#include "engine/context.h"
#include "engine/comptime_value.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_comptime_alloc : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

/* ===== lifecycle ===== */

TEST_F(dt_comptime_alloc, create_and_dispose) {
  comptime_allocator_t a = comptime_allocator_create(allocator);
  ASSERT_NE(a, nullptr);
  comptime_allocator_dispose(a);
}

/* ===== allocate / read / write ===== */

TEST_F(dt_comptime_alloc, allocate_and_read) {
  context_t ctx = context_create(allocator);
  comptime_allocator_t a = comptime_allocator_create(allocator);

  comptime_value_t val = comptime_value_create_int(allocator, 42, 42, 32, true,
                                                     ctx->builtin_i32);
  uint64_t addr = comptime_alloc_allocate(a, val, 0);
  EXPECT_NE(addr, 0u);

  comptime_value_t read = comptime_alloc_read(a, addr);
  ASSERT_NE(read, nullptr);
  EXPECT_EQ(read->kind, COMPTIME_VALUE_INT);
  EXPECT_EQ(read->int_val.s, 42);

  comptime_allocator_dispose(a);
  context_dispose(ctx);
}

TEST_F(dt_comptime_alloc, write_overwrites) {
  context_t ctx = context_create(allocator);
  comptime_allocator_t a = comptime_allocator_create(allocator);

  comptime_value_t v1 = comptime_value_create_int(allocator, 10, 10, 32, true,
                                                    ctx->builtin_i32);
  uint64_t addr = comptime_alloc_allocate(a, v1, 0);

  comptime_value_t v2 = comptime_value_create_int(allocator, 20, 20, 32, true,
                                                    ctx->builtin_i32);
  EXPECT_TRUE(comptime_alloc_write(a, addr, v2));

  comptime_value_t read = comptime_alloc_read(a, addr);
  ASSERT_NE(read, nullptr);
  EXPECT_EQ(read->int_val.s, 20);

  comptime_allocator_dispose(a);
  context_dispose(ctx);
}

TEST_F(dt_comptime_alloc, read_null_addr) {
  comptime_allocator_t a = comptime_allocator_create(allocator);
  EXPECT_EQ(comptime_alloc_read(a, 0), nullptr);
  comptime_allocator_dispose(a);
}

TEST_F(dt_comptime_alloc, write_null_addr) {
  comptime_allocator_t a = comptime_allocator_create(allocator);
  context_t ctx = context_create(allocator);
  comptime_value_t v = comptime_value_create_int(allocator, 1, 1, 32, true,
                                                   ctx->builtin_i32);
  EXPECT_FALSE(comptime_alloc_write(a, 0, v));
  allocator_free(allocator, &v);
  comptime_allocator_dispose(a);
  context_dispose(ctx);
}

TEST_F(dt_comptime_alloc, write_unknown_addr) {
  comptime_allocator_t a = comptime_allocator_create(allocator);
  context_t ctx = context_create(allocator);
  comptime_value_t v = comptime_value_create_int(allocator, 1, 1, 32, true,
                                                   ctx->builtin_i32);
  EXPECT_FALSE(comptime_alloc_write(a, 999, v));
  allocator_free(allocator, &v);
  comptime_allocator_dispose(a);
  context_dispose(ctx);
}

/* ===== free ===== */

TEST_F(dt_comptime_alloc, free_makes_addr_invalid) {
  context_t ctx = context_create(allocator);
  comptime_allocator_t a = comptime_allocator_create(allocator);

  comptime_value_t val = comptime_value_create_int(allocator, 42, 42, 32, true,
                                                     ctx->builtin_i32);
  uint64_t addr = comptime_alloc_allocate(a, val, 0);
  comptime_alloc_free(a, addr);
  EXPECT_EQ(comptime_alloc_read(a, addr), nullptr);

  comptime_allocator_dispose(a);
  context_dispose(ctx);
}

/* ===== scope lifecycle ===== */

TEST_F(dt_comptime_alloc, scope_leave_frees_allocations) {
  context_t ctx = context_create(allocator);
  comptime_allocator_t a = comptime_allocator_create(allocator);

  comptime_alloc_enter_scope(a); /* depth 1 */

  comptime_value_t v = comptime_value_create_int(allocator, 99, 99, 32, true,
                                                   ctx->builtin_i32);
  uint64_t addr = comptime_alloc_allocate(a, v, 1);
  EXPECT_NE(comptime_alloc_read(a, addr), nullptr);

  comptime_alloc_leave_scope(a); /* back to 0, frees depth >= 1 */
  EXPECT_EQ(comptime_alloc_read(a, addr), nullptr); /* dangling */

  comptime_allocator_dispose(a);
  context_dispose(ctx);
}

TEST_F(dt_comptime_alloc, nested_scopes) {
  context_t ctx = context_create(allocator);
  comptime_allocator_t a = comptime_allocator_create(allocator);

  comptime_alloc_enter_scope(a); /* depth 1 */
  comptime_value_t v1 = comptime_value_create_int(allocator, 1, 1, 32, true,
                                                    ctx->builtin_i32);
  uint64_t a1 = comptime_alloc_allocate(a, v1, 1);

  comptime_alloc_enter_scope(a); /* depth 2 */
  comptime_value_t v2 = comptime_value_create_int(allocator, 2, 2, 32, true,
                                                    ctx->builtin_i32);
  uint64_t a2 = comptime_alloc_allocate(a, v2, 2);

  /* both visible */
  EXPECT_NE(comptime_alloc_read(a, a1), nullptr);
  EXPECT_NE(comptime_alloc_read(a, a2), nullptr);

  comptime_alloc_leave_scope(a); /* back to 1, frees depth >= 2 */
  EXPECT_NE(comptime_alloc_read(a, a1), nullptr);
  EXPECT_EQ(comptime_alloc_read(a, a2), nullptr);

  comptime_alloc_leave_scope(a); /* back to 0, frees depth >= 1 */
  EXPECT_EQ(comptime_alloc_read(a, a1), nullptr);

  comptime_allocator_dispose(a);
  context_dispose(ctx);
}

TEST_F(dt_comptime_alloc, free_null_addr_noop) {
  comptime_allocator_t a = comptime_allocator_create(allocator);
  comptime_alloc_free(a, 0); /* should not crash */
  comptime_allocator_dispose(a);
}
