
#include "core/allocator.h"
#include <gtest/gtest.h>
class test_allocator : public testing::Test {};
TEST_F(test_allocator, alloc_and_free) {
  static size_t counter = 0;
  allocator_initialize_t initialize = {};
  initialize.alloc = [](size_t size) -> void * {
    counter++;
    return ::operator new(size);
  };
  initialize.free = [](void *data) -> void {
    counter--;
    return ::operator delete(data);
  };
  allocator_t allocator = create_allocator(&initialize);
  ASSERT_EQ(counter, 1);
  void *data = allocator_alloc(allocator, 4, NULL);
  ASSERT_EQ(counter, 2);
  allocator_free(allocator, data);
  ASSERT_EQ(counter, 1);
  delete_allocator(allocator);
  ASSERT_EQ(counter, 0);
}

TEST_F(test_allocator, dispose) {
  static size_t counter = 0;
  allocator_t allocator = create_allocator(NULL);
  void *data = allocator_alloc(
      allocator, 4, [](void *data, allocator_t allocator) { counter++; });
  allocator_free(allocator, data);
  delete_allocator(allocator);
  ASSERT_EQ(counter, 1);
}