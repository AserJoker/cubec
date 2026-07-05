#ifndef _H_CUBEC_TEST_COMMON_
#define _H_CUBEC_TEST_COMMON_

#include "core/allocator.h"
#include "core/error.h"
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

/**
 * @brief Base test fixture that auto-clears thread-local g_error.
 *        Ensures no error state leaks between tests.
 */
class CubecTest : public ::testing::Test {
protected:
  void SetUp() override { error_clear(); }
  void TearDown() override { error_clear(); }
};

#endif