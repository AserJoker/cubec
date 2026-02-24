#include "core/allocator.h"
#include <cstddef>
#include <gtest/gtest.h>
class cubec_test : public ::testing::Test {
protected:
  static int32_t count;
  cubec_allocator_t allocator = NULL;
  void SetUp() override {
    cubec_allocator_initialize_t initialize = {
        .alloc = [](size_t size) -> void * {
          size_t *data = (size_t *)malloc(size + sizeof(size_t));
          *data = size;
          count += size;
          return &data[1];
        },
        .free = [](void *data) -> void {
          size_t *chunk = (size_t *)((uint8_t *)data - sizeof(size_t));
          count -= *chunk;
          free(chunk);
        },
    };
    allocator = cubec_create_allocator(&initialize);
  }
  void TearDown() override {
    cubec_delete_allocator(allocator);
    EXPECT_EQ(count, 0);
  }
};