#ifndef _H_CUBEC_TEST_COMMON_
#define _H_CUBEC_TEST_COMMON_

#include "core/allocator.h"
#include "engine/context.h"
#include <gtest/gtest.h>

class test_allocator {
public:
  allocator_t allocator;

  test_allocator() { allocator = create_allocator(NULL, NULL); }
  ~test_allocator() { delete_allocator(allocator); }
};

#define TEST_ALLOCATOR                                                         \
  test_allocator test_allocator_instance;                                      \
  allocator_t allocator = test_allocator_instance.allocator

class test_context {
public:
  allocator_t allocator;
  context_t ctx;
  test_context() { allocator = create_allocator(NULL, NULL); ctx = context_create(allocator); }
  ~test_context() { context_dispose(ctx); delete_allocator(allocator); }
};

/**
 * @brief Base test fixture. Provides common setup/teardown.
 */
class CubecTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

#endif
