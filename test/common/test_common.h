#ifndef _H_CUBEC_TEST_COMMON_
#define _H_CUBEC_TEST_COMMON_

#include "core/allocator.h"
#include <gtest/gtest.h>

class TestAllocator {
public:
  allocator_t allocator;

  TestAllocator() { allocator = create_allocator(NULL, NULL); }
  ~TestAllocator() { delete_allocator(allocator); }
};

#define TEST_ALLOCATOR                                                      \
  TestAllocator test_allocator;                                             \
  allocator_t allocator = test_allocator.allocator

#endif