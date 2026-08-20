#include "run/run.h"
#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/integer_type.h"
#include "engine/exception_type.h"
#include "engine/generic_type.h"
#include "engine/generic_param.h"
#include "engine/array_type.h"
#include "engine/struct_type.h"
#include "engine/void_type.h"
#include "core/string.h"
#include "core/vec.h"
#include "core/class.h"
#include "cubec/literal_identifier.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_struct.h"
#include "cubec/struct_field.h"
#include "cubec/expression.h"
#include "cubec/token.h"
#include "cubec/node.h"
#include "core/location.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ------------------------------------------------------------------ *
 *  it_generic_type — end-to-end generic instantiation tests.
 *
 *  Tests the full flow:
 *    1. Build a generic type with params + AST node
 *    2. Create a generic value (value.data = create_instance_fn_t)
 *    3. Call value_instantiate with concrete args
 *    4. Verify the result type
 *    5. Verify cache hit returns same instance
 * ------------------------------------------------------------------ */

class it_generic_type : public CubecTest {
protected:
  /* Owned test artifacts that must be freed after vm_dispose (which
   * frees the generic_type_t that borrows them). */
  node_t rhs_node = NULL;
  vec_t params_vec = NULL;

  void TearDown() override {
    /* vm_dispose frees the generic_type_t (which borrows rhs_node),
     * so free rhs_node + params_vec AFTER vm_dispose. */
    if (vm) {
      vm_dispose(vm, allocator);
      vm = NULL;
    }
    if (params_vec) allocator_free(allocator, &params_vec);
    if (rhs_node)   allocator_free(allocator, &rhs_node);
    /* Now run the base leak check (but skip vm_dispose, already done). */
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

  location_t _loc() {
    location_t loc;
    memset(&loc, 0, sizeof(loc));
    loc.filename = "test";
    return loc;
  }

  allocator_t alloc() { return vm_get_allocator(vm); }

  /* Create a u64 value with the given count (registered in current scope) */
  value_t _u64_val(uint64_t n) {
    return vm_create_value(vm,
        (type_t)value_get_data(vm_get_u64_type(vm)), &n, NULL);
  }

  /* Create a type value wrapping the given type_t (registered in current
   * scope via vm_create_value_ref so it is freed on vm_dispose). */
  value_t _type_val(type_t t) {
    type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
    return vm_create_value_ref(vm, type_type, t, NULL);
  }

  void free_tokens(vec_t &tokens) {
    if (tokens) allocator_free(vm_get_allocator(vm), &tokens);
  }

  /* Parse a source string into an expression node */
  node_t _parse_expr(const char *source) {
    vec_t tokens = resolve_token_list(vm, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_expression(vm, tokens, &position, "test.cubec");
    free_tokens(tokens);
    return node;
  }

  /* Create a generic_param_t with name + type + empty extends.
   * generic_param_create deep-copies extends, so the caller's extends vec
   * must be freed afterwards. */
  generic_param_t _make_param(const char *name, type_t type) {
    vec_init_t vi = {.auto_dispose = true};
    vec_t extends = (vec_t)allocator_create(alloc(), &g_vec_class, &vi);
    generic_param_t gp = generic_param_create(alloc(), name, type, extends);
    allocator_free(alloc(), &extends);
    return gp;
  }

  /* Build a generic_type_t for `type Array[T, N:u64] = [N]T`
   * with create_type_instance as callback.
   * The AST node is the RHS type expression `[N]T` parsed from source.
   * Stores rhs_node + params_vec as members so TearDown can free them
   * after vm_dispose. */
  value_t _make_array_generic() {
    /* parse [N]T as type expression — N and T will be resolved at
     * instantiation time from the temp scope */
    rhs_node = _parse_expr("[N]T");
    /* rhs should be a CUBEC_NODE_DECLARATION_ARRAY with size=N, type=T */

    /* build params: T (type), N (u64 value) */
    vec_init_t vi = {.auto_dispose = true};
    params_vec = (vec_t)allocator_create(alloc(), &g_vec_class, &vi);

    /* T: type param — type is the "type" type */
    type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
    generic_param_t p_T = _make_param("T", type_type);
    vec_push(params_vec, p_T);

    /* N: u64 value param — type is u64 */
    type_t u64_type = (type_t)value_get_data(vm_get_u64_type(vm));
    generic_param_t p_N = _make_param("N", u64_type);
    vec_push(params_vec, p_N);

    /* create generic type — node is the RHS expression (borrowed) */
    generic_type_t gt = generic_type_create(alloc(), "Array", params_vec, rhs_node);

    /* register in vm->types */
    vec_push(vm_get_types(vm), gt);

    /* create generic value: value.type = gt, value.data = callback */
    value_t gv = value_create(alloc(), (type_t)gt,
                              (void *)create_type_instance, false);
    vec_push(vm_get_current_scope(vm)->values, gv);

    /* bind name "Array" -> generic value */
    scope_t scope = vm_get_current_scope(vm);
    name_t n = name_create(scope->allocator, gv);
    strmap_insert(scope->names, "Array", n);

    return gv;
  }
};

/* ---- create_type_instance: Array[i32, 4] ---- */

TEST_F(it_generic_type, type_alias_instantiate_array) {
  value_t gv = _make_array_generic();
  ASSERT_FALSE(value_is_abnormal(gv));

  /* build args: i32 type value, u64 value 4 */
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));

  /* arg 0: i32 type value */
  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t arg0 = _type_val(i32_type);

  /* arg 1: u64 value 4 */
  value_t arg1 = _u64_val(4);

  value_t argv[2] = {arg0, arg1};

  /* instantiate */
  value_t result = value_instantiate(vm, gv, 2, argv);
  ASSERT_FALSE(value_is_abnormal(result))
      << "instantiation failed: "
      << (value_is_abnormal(result)
              ? (const char *)value_get_data(result)
              : "");

  /* result should be a type value (TYPE_KIND_TYPE) wrapping an array type */
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_TYPE);
  type_t result_type = (type_t)value_get_data(result);
  EXPECT_EQ(type_get_kind(result_type), TYPE_KIND_ARRAY);
  array_type_t at = (array_type_t)result_type;
  EXPECT_EQ(array_type_get_count(at), 4u);
  EXPECT_EQ(type_get_kind(array_type_get_element_type(at)), TYPE_KIND_I32);
}

/* ---- cache hit: same args return same instance ---- */

TEST_F(it_generic_type, type_alias_cache_hit) {
  value_t gv = _make_array_generic();

  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t arg0a = _type_val(i32_type);
  value_t arg1a = _u64_val(4);
  value_t argv_a[2] = {arg0a, arg1a};

  value_t result1 = value_instantiate(vm, gv, 2, argv_a);
  ASSERT_FALSE(value_is_abnormal(result1));

  /* second instantiation with same args */
  value_t arg0b = _type_val(i32_type);
  value_t arg1b = _u64_val(4);
  value_t argv_b[2] = {arg0b, arg1b};

  value_t result2 = value_instantiate(vm, gv, 2, argv_b);
  ASSERT_FALSE(value_is_abnormal(result2));

  /* cache hit: same pointer */
  EXPECT_EQ(result1, result2);
}

/* ---- cache miss: different args return different instance ---- */

TEST_F(it_generic_type, type_alias_cache_miss) {
  value_t gv = _make_array_generic();

  /* first: Array[i32, 4] */
  type_t i32_type = (type_t)value_get_data(vm_get_i32_type(vm));
  value_t arg0a = _type_val(i32_type);
  value_t arg1a = _u64_val(4);
  value_t argv_a[2] = {arg0a, arg1a};
  value_t result1 = value_instantiate(vm, gv, 2, argv_a);
  ASSERT_FALSE(value_is_abnormal(result1));

  /* second: Array[i64, 8] */
  type_t i64_type = (type_t)value_get_data(vm_get_i64_type(vm));
  value_t arg0b = _type_val(i64_type);
  value_t arg1b = _u64_val(8);
  value_t argv_b[2] = {arg0b, arg1b};
  value_t result2 = value_instantiate(vm, gv, 2, argv_b);
  ASSERT_FALSE(value_is_abnormal(result2))
      << "second instantiation failed: "
      << (value_is_abnormal(result2)
              ? (const char *)value_get_data(result2)
              : "");

  /* different instances */
  EXPECT_NE(result1, result2);

  /* verify result1 is [4]i32 */
  type_t t1 = (type_t)value_get_data(result1);
  EXPECT_EQ(array_type_get_count((array_type_t)t1), 4u);
  EXPECT_EQ(type_get_kind(array_type_get_element_type((array_type_t)t1)), TYPE_KIND_I32);

  /* verify result2 is [8]i64 */
  type_t t2 = (type_t)value_get_data(result2);
  EXPECT_EQ(array_type_get_count((array_type_t)t2), 8u);
  EXPECT_EQ(type_get_kind(array_type_get_element_type((array_type_t)t2)), TYPE_KIND_I64);
}
