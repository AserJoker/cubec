
#include "core/allocator.h"
#include <gtest/gtest.h>
#include <vcruntime_new.h>
class test_allocator : public testing::Test {};
TEST_F(test_allocator, alloc_and_free) {
  static size_t counter = 0;
  cubec_allocator_initialize_t initialize = {};
  initialize.alloc = [](size_t size) -> void * {
    counter++;
    return ::operator new(size);
  };
  initialize.free = [](void *data) -> void {
    counter--;
    return ::operator delete(data);
  };
  cubec_allocator_t allocator = cubec_create_allocator(&initialize);
  ASSERT_EQ(counter, 1);
  void *data = cubec_allocator_alloc(allocator, 4, NULL);
  ASSERT_EQ(counter, 2);
  cubec_allocator_free(allocator, data);
  ASSERT_EQ(counter, 1);
  cubec_delete_allocator(allocator);
  ASSERT_EQ(counter, 0);
}

TEST_F(test_allocator, dispose) {
  static size_t counter = 0;
  cubec_allocator_t allocator = cubec_create_allocator(NULL);
  void *data = cubec_allocator_alloc(
      allocator, 4, [](void *data, cubec_allocator_t allocator) { counter++; });
  cubec_allocator_free(allocator, data);
  cubec_delete_allocator(allocator);
  ASSERT_EQ(counter, 1);
}