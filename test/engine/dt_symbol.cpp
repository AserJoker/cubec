#include "engine/symbol.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_symbol : public CubecTest {
protected:
  test_context test_context_instance;
  allocator_t allocator = test_context_instance.allocator;
  context_t ctx = test_context_instance.ctx;
};

TEST_F(dt_symbol, create_variable) {
  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 1, NULL},
                    .end = {1, 5, NULL}};
  struct symbol *sym =
      symbol_create(allocator, "x", SYMBOL_VARIABLE, loc);

  EXPECT_STREQ(sym->name, "x");
  EXPECT_EQ(sym->kind, SYMBOL_VARIABLE);
  EXPECT_EQ(sym->state, SYMBOL_TDZ);
  EXPECT_EQ(sym->variable.type, nullptr);
  EXPECT_EQ(sym->variable.is_comptime, false);

  allocator_free(allocator, &sym);
}

TEST_F(dt_symbol, create_function) {
  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 1, NULL},
                    .end = {1, 10, NULL}};
  struct symbol *sym =
      symbol_create(allocator, "main", SYMBOL_FUNCTION, loc);

  EXPECT_STREQ(sym->name, "main");
  EXPECT_EQ(sym->kind, SYMBOL_FUNCTION);
  EXPECT_EQ(sym->state, SYMBOL_TDZ);
  EXPECT_EQ(sym->function.type, nullptr);

  allocator_free(allocator, &sym);
}

TEST_F(dt_symbol, create_type) {
  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 1, NULL},
                    .end = {1, 10, NULL}};
  struct symbol *sym =
      symbol_create(allocator, "MyStruct", SYMBOL_TYPE, loc);

  EXPECT_EQ(sym->kind, SYMBOL_TYPE);
  EXPECT_EQ(sym->type.type, nullptr);

  allocator_free(allocator, &sym);
}

TEST_F(dt_symbol, state_transition) {
  location_t loc = {.filename = "test.cubec",
                    .begin = {1, 1, NULL},
                    .end = {1, 5, NULL}};
  struct symbol *sym =
      symbol_create(allocator, "x", SYMBOL_VARIABLE, loc);

  EXPECT_EQ(sym->state, SYMBOL_TDZ);
  sym->state = SYMBOL_NAME_KNOWN;
  EXPECT_EQ(sym->state, SYMBOL_NAME_KNOWN);
  sym->state = SYMBOL_EVALUATED;
  EXPECT_EQ(sym->state, SYMBOL_EVALUATED);

  allocator_free(allocator, &sym);
}
