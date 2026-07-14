#include "engine/semantic_type.h"
#include "engine/type_hash.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class dt_type_hash : public CubecTest {
protected:
  TEST_ALLOCATOR;
};

TEST_F(dt_type_hash, hash_primitive) {
  semantic_type_t t =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  type_hash_ensure(t);
  EXPECT_NE(type_hash_compute(t), (size_t)0);
  EXPECT_NE(t->impl->hash, (size_t)0);

  allocator_free(allocator, &t);
}

TEST_F(dt_type_hash, hash_pointer) {
  semantic_type_t base =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t ptr = semantic_type_create_pointer(allocator, base);
  type_hash_ensure(ptr);
  EXPECT_NE(ptr->impl->hash, (size_t)0);

  allocator_free(allocator, &ptr);
  allocator_free(allocator, &base);
}

TEST_F(dt_type_hash, hash_deterministic) {
  semantic_type_t a1 =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t b1 =
      semantic_type_create_named(allocator, "i32", TYPE_I32);
  semantic_type_t pa = semantic_type_create_pointer(allocator, a1);
  semantic_type_t pb = semantic_type_create_pointer(allocator, b1);

  type_hash_ensure(pa);
  type_hash_ensure(pb);

  /* Same structure => same hash */
  EXPECT_EQ(pa->impl->hash, pb->impl->hash);

  allocator_free(allocator, &pb);
  allocator_free(allocator, &pa);
  allocator_free(allocator, &b1);
  allocator_free(allocator, &a1);
}

TEST_F(dt_type_hash, hash_different_kinds) {
  semantic_type_t elem =
      semantic_type_create_named(allocator, "u8", TYPE_U8);
  semantic_type_t ptr = semantic_type_create_pointer(allocator, elem);
  semantic_type_t sl = semantic_type_create_slice(allocator, elem);

  type_hash_ensure(ptr);
  type_hash_ensure(sl);
  EXPECT_NE(ptr->impl->hash, sl->impl->hash);

  allocator_free(allocator, &sl);
  allocator_free(allocator, &ptr);
  allocator_free(allocator, &elem);
}

TEST_F(dt_type_hash, ensure_idempotent) {
  semantic_type_t t =
      semantic_type_create_named(allocator, "f64", TYPE_F64);
  type_hash_ensure(t);
  size_t h1 = t->impl->hash;
  type_hash_ensure(t);
  EXPECT_EQ(t->impl->hash, h1);

  allocator_free(allocator, &t);
}
