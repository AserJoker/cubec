#ifndef _H_CUBEC_TEST_COMMON_
#define _H_CUBEC_TEST_COMMON_

#include "core/allocator.h"
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

#endif