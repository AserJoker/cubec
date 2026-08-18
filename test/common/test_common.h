#ifndef _H_CUBEC_TEST_COMMON_
#define _H_CUBEC_TEST_COMMON_

#include "core/allocator.h"
#include "engine/vm.h"
#include <gtest/gtest.h>

/**
 * @brief Base test fixture. Provides common setup/teardown.
 *
 * All test classes inherit from CubecTest and use the @c allocator (and
 * optional @c vm) members created here, instead of allocating their own
 * per-class allocator. The allocator is created in SetUp() and disposed in
 * TearDown() with a leak check, so individual tests must NOT call
 * create_allocator()/delete_allocator() for the shared allocator.
 */
class CubecTest : public ::testing::Test {
protected:
  allocator_t allocator = NULL;
  vm_t vm = NULL;

  void SetUp() override {
    allocator = create_allocator_with_limit(NULL, NULL, 256 * 1024 * 1024);
    vm = vm_create(allocator);
  }

  void TearDown() override {
    if (vm) {
      vm_dispose(vm, allocator);
      vm = NULL;
    }
    if (allocator) {
      size_t peak = allocator_get_peak(allocator);
      size_t total = allocator_get_total(allocator);
      size_t allocs = allocator_get_alloc_count(allocator);
      size_t frees = allocator_get_free_count(allocator);
      bool leaked = (total > 0 || allocs != frees);
      delete_allocator(allocator);
      allocator = NULL;
      if (leaked) {
        ADD_FAILURE() << "[CubecTest] memory leak detected: peak="
                      << (peak / (1024.0 * 1024.0)) << "MB, still_allocated="
                      << total << " bytes, allocs=" << allocs
                      << ", frees=" << frees;
      }
    }
  }
};

#endif
