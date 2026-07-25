#include "engine/scope.h"
#include "engine/symbol.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_scope : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_scope, create_global) {
  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 1, NULL},
                    .end = {1, 1, NULL}};
  scope_t global = scope_create(allocator, NULL, SCOPE_GLOBAL, loc);
  ASSERT_NE(global, nullptr);
  EXPECT_EQ(scope_get_parent(global), nullptr);
  EXPECT_EQ(scope_get_kind(global), SCOPE_GLOBAL);
  allocator_free(allocator, &global);
}

TEST_F(dt_scope, push_and_lookup_local) {
  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 1, NULL},
                    .end = {1, 1, NULL}};
  scope_t global = scope_create(allocator, NULL, SCOPE_GLOBAL, loc);

  struct symbol *sym =
      symbol_create(allocator, "x", SYMBOL_VARIABLE, loc);
  scope_push_symbol(global, sym);

  EXPECT_EQ(scope_lookup_local(global, "x"), sym);
  EXPECT_EQ(scope_lookup_local(global, "y"), nullptr);

  /* scope owns symbols via auto_dispose */
  allocator_free(allocator, &global);
}

TEST_F(dt_scope, lookup_walks_parent) {
  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 1, NULL},
                    .end = {1, 1, NULL}};

  scope_t global = scope_create(allocator, NULL, SCOPE_GLOBAL, loc);
  scope_t block = scope_create(allocator, global, SCOPE_BLOCK, loc);

  struct symbol *global_var =
      symbol_create(allocator, "g", SYMBOL_VARIABLE, loc);
  scope_push_symbol(global, global_var);

  /* lookup from block finds global symbol */
  EXPECT_EQ(scope_lookup(block, "g"), global_var);
  EXPECT_EQ(scope_lookup_local(block, "g"), nullptr);

  allocator_free(allocator, &block);
  allocator_free(allocator, &global);
}

TEST_F(dt_scope, shadowing) {
  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 1, NULL},
                    .end = {1, 1, NULL}};

  scope_t global = scope_create(allocator, NULL, SCOPE_GLOBAL, loc);
  scope_t block = scope_create(allocator, global, SCOPE_BLOCK, loc);

  struct symbol *outer =
      symbol_create(allocator, "x", SYMBOL_VARIABLE, loc);
  scope_push_symbol(global, outer);

  struct symbol *inner =
      symbol_create(allocator, "x", SYMBOL_VARIABLE, loc);
  scope_push_symbol(block, inner);

  /* block lookup finds inner */
  EXPECT_EQ(scope_lookup(block, "x"), inner);
  /* global lookup finds outer */
  EXPECT_EQ(scope_lookup(global, "x"), outer);

  allocator_free(allocator, &block);
  allocator_free(allocator, &global);
}
