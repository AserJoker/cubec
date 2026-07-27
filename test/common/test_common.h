#ifndef _H_CUBEC_TEST_COMMON_
#define _H_CUBEC_TEST_COMMON_

#include "core/allocator.h"
#include "engine/context.h"
#include <gtest/gtest.h>

class test_allocator {
public:
  allocator_t allocator;

  test_allocator() { allocator = create_allocator_with_limit(NULL, NULL, 256 * 1024 * 1024); }
  ~test_allocator() {
    size_t peak = allocator_get_peak(allocator);
    size_t total = allocator_get_total(allocator);
    size_t allocs = allocator_get_alloc_count(allocator);
    size_t frees = allocator_get_free_count(allocator);
    if (total > 0 || allocs != frees) {
      fprintf(stderr,
              "  [test_allocator] LEAK: peak=%.2fMB, still_allocated=%zu bytes, "
              "allocs=%zu, frees=%zu\n",
              peak / (1024.0 * 1024.0), total, allocs, frees);
    }
    delete_allocator(allocator);
  }
};

#define TEST_ALLOCATOR                                                         \
  test_allocator test_allocator_instance;                                      \
  allocator_t allocator = test_allocator_instance.allocator

class test_context {
public:
  allocator_t allocator;
  context_t ctx;
  test_context() { allocator = create_allocator_with_limit(NULL, NULL, 256 * 1024 * 1024); ctx = context_create(allocator); }
  ~test_context() {
    context_dispose(ctx);
    size_t peak = allocator_get_peak(allocator);
    size_t total = allocator_get_total(allocator);
    size_t allocs = allocator_get_alloc_count(allocator);
    size_t frees = allocator_get_free_count(allocator);
    if (total > 0 || allocs != frees) {
      fprintf(stderr,
              "  [test_context] LEAK: peak=%.2fMB, still_allocated=%zu bytes, "
              "allocs=%zu, frees=%zu\n",
              peak / (1024.0 * 1024.0), total, allocs, frees);
    }
    delete_allocator(allocator);
  }
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
