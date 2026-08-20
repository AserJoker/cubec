#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/str_type.h"
#include "engine/callable_type.h"
#include "engine/generic_fn_type.h"
#include "engine/generic_param.h"
#include "engine/ast_func.h"
#include "engine/func.h"
#include "core/string.h"
#include "core/vec.h"
#include "core/location.h"
#include "cubec/declaration_function.h"
#include "cubec/function_argument.h"
#include "cubec/literal_identifier.h"
#include "cubec/expression_binary.h"
#include "cubec/statement_return.h"
#include "cubec/statement_block.h"
#include "cubec/generic_param.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_generic_fn_type : public CubecTest {
protected:
  type_t _get_i32_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
  type_t _get_bool_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_bool_type(vm));
  }
  type_t _get_void_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_void_type(vm));
  }
  type_t _get_str_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_str_type(vm));
  }

  location_t _dummy_loc() {
    return (location_t){.filename = "<test>", .begin = {1, 1, NULL},
                        .end = {1, 1, NULL}};
  }

  /* Build AST for: func <name>(<params>): <return_type> { return <body_expr>; } */
  node_t _build_func_ast(vm_t vm, const char *name, vec_t params,
                          node_t return_type, node_t body_expr) {
    location_t loc = _dummy_loc();
    node_t ret_stmt = create_statement_return(vm, loc, body_expr);
    vec_init_t bvi = {.auto_dispose = true};
    vec_t stmts = (vec_t)allocator_create(vm_get_allocator(vm), &g_vec_class,
                                           &bvi);
    vec_push(stmts, ret_stmt);
    node_t body = create_statement_block(vm, loc, stmts);
    node_t name_node = create_literal_identifier(vm, loc, name);
    return create_declaration_function(vm, loc, name_node, NULL, NULL, params,
                                       return_type, body, false, false, false,
                                       false, false);
  }

  /* Create a generic_param_t (type param, no extends) */
  generic_param_t _make_type_param(vm_t vm, const char *name) {
    allocator_t alloc = vm_get_allocator(vm);
    type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
    vec_init_t vi = {.auto_dispose = true};
    vec_t extends = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    generic_param_t gp = generic_param_create(alloc, name, type_type, extends);
    allocator_free(alloc, &extends);
    return gp;
  }

  /* Create a generic_param_t with extends constraint */
  generic_param_t _make_constrained_param(vm_t vm, const char *name,
                                           type_t constraint) {
    allocator_t alloc = vm_get_allocator(vm);
    type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
    vec_init_t vi = {.auto_dispose = false};
    vec_t extends = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    vec_push(extends, constraint); /* borrowed: types managed by vm->types */
    generic_param_t gp = generic_param_create(alloc, name, type_type, extends);
    allocator_free(alloc, &extends);
    return gp;
  }
};

/* ---- Generic function instantiation ---- */

TEST_F(it_generic_fn_type, instantiate_identity_i32) {
  /* func identity[T](x: T) -> T { return x; }
   * Test instantiation only (no call yet) */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  /* Build AST for identity function */
  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
  vec_push(args, create_function_argument(vm, loc, "x",
      create_literal_identifier(vm, loc, "T")));

  node_t ret_type = create_literal_identifier(vm, loc, "T");
  node_t body_expr = create_literal_identifier(vm, loc, "x");
  node_t func_node = _build_func_ast(vm, "identity", args, ret_type, body_expr);

  /* Build generic params: [T] */
  vec_init_t pvi = {.auto_dispose = true};
  vec_t params_vec = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  vec_push(params_vec, _make_type_param(vm, "T"));

  /* Create generic_fn_type_t */
  generic_fn_type_t gt = generic_fn_type_create(alloc, "identity",
                                                  params_vec, func_node);
  allocator_free(alloc, &params_vec);

  /* Register in vm->types */
  scope_t scope = vm_get_current_scope(vm);
  vec_push(vm_get_types(vm), gt);

  /* Create generic value with create_fn_instance callback */
  value_t gen_val = vm_create_value_ref(vm, (type_t)gt,
                                         (const void *)create_fn_instance,
                                         "identity");

  /* Instantiate identity[i32] */
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  value_t i32_type_val = vm_create_value_ref(vm, type_type,
                                              _get_i32_type(vm), NULL);
  value_t inst = value_instantiate(vm, gen_val, 1, &i32_type_val);

  /* Check instantiation result */
  EXPECT_FALSE(value_is_abnormal(inst));
  if (!value_is_abnormal(inst)) {
    EXPECT_EQ(type_get_kind(value_get_type(inst)), TYPE_KIND_CALLABLE);
  }

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

TEST_F(it_generic_fn_type, instantiate_add_i32) {
  /* func add[T](a: T, b: T) -> T { return a + b; }
   * add[i32](3, 4) → 7 */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  /* Build AST for add function */
  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
  vec_push(args, create_function_argument(vm, loc, "a",
      create_literal_identifier(vm, loc, "T")));
  vec_push(args, create_function_argument(vm, loc, "b",
      create_literal_identifier(vm, loc, "T")));

  node_t ret_type = create_literal_identifier(vm, loc, "T");
  node_t body_expr = create_expression_binary(
      vm, loc, "+", create_literal_identifier(vm, loc, "a"),
      create_literal_identifier(vm, loc, "b"));
  node_t func_node = _build_func_ast(vm, "add", args, ret_type, body_expr);

  /* Build generic params: [T] */
  vec_init_t pvi = {.auto_dispose = true};
  vec_t params_vec = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  vec_push(params_vec, _make_type_param(vm, "T"));

  /* Create generic_fn_type_t */
  generic_fn_type_t gt = generic_fn_type_create(alloc, "add",
                                                  params_vec, func_node);
  allocator_free(alloc, &params_vec);

  scope_t scope = vm_get_current_scope(vm);
  vec_push(vm_get_types(vm), gt);

  value_t gen_val = vm_create_value_ref(vm, (type_t)gt,
                                         (const void *)create_fn_instance,
                                         "add");

  /* Instantiate add[i32] */
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  value_t i32_type_val = vm_create_value_ref(vm, type_type,
                                              _get_i32_type(vm), NULL);
  value_t inst = value_instantiate(vm, gen_val, 1, &i32_type_val);
  EXPECT_FALSE(value_is_abnormal(inst));

  /* Call add[i32](3, 4) */
  int32_t a = 3, b = 4;
  value_t argv[2] = {
      vm_create_value(vm, _get_i32_type(vm), &a, NULL),
      vm_create_value(vm, _get_i32_type(vm), &b, NULL),
  };
  value_t result = value_call(vm, inst, 2, argv);
  EXPECT_FALSE(value_is_abnormal(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(result), 7);

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

TEST_F(it_generic_fn_type, cache_hit_returns_same_instance) {
  /* identity[i32] called twice should return the same value */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
  vec_push(args, create_function_argument(vm, loc, "x",
      create_literal_identifier(vm, loc, "T")));

  node_t ret_type = create_literal_identifier(vm, loc, "T");
  node_t body_expr = create_literal_identifier(vm, loc, "x");
  node_t func_node = _build_func_ast(vm, "identity", args, ret_type, body_expr);

  vec_init_t pvi = {.auto_dispose = true};
  vec_t params_vec = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  vec_push(params_vec, _make_type_param(vm, "T"));

  generic_fn_type_t gt = generic_fn_type_create(alloc, "identity",
                                                  params_vec, func_node);
  allocator_free(alloc, &params_vec);

  scope_t scope = vm_get_current_scope(vm);
  vec_push(vm_get_types(vm), gt);

  value_t gen_val = vm_create_value_ref(vm, (type_t)gt,
                                         (const void *)create_fn_instance,
                                         "identity");

  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  value_t i32_type_val = vm_create_value_ref(vm, type_type,
                                              _get_i32_type(vm), NULL);

  value_t inst1 = value_instantiate(vm, gen_val, 1, &i32_type_val);
  value_t inst2 = value_instantiate(vm, gen_val, 1, &i32_type_val);
  EXPECT_EQ(inst1, inst2);

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

TEST_F(it_generic_fn_type, extends_constraint_violation) {
  /* func foo[T extends str](x: T) -> T { return x; }
   * foo[i32](42) → exception (i32 does not extend str) */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
  vec_push(args, create_function_argument(vm, loc, "x",
      create_literal_identifier(vm, loc, "T")));

  node_t ret_type = create_literal_identifier(vm, loc, "T");
  node_t body_expr = create_literal_identifier(vm, loc, "x");
  node_t func_node = _build_func_ast(vm, "foo", args, ret_type, body_expr);

  /* Build generic params: [T extends str] */
  vec_init_t pvi = {.auto_dispose = true};
  vec_t params_vec = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  vec_push(params_vec,
           _make_constrained_param(vm, "T", _get_str_type(vm)));

  generic_fn_type_t gt = generic_fn_type_create(alloc, "foo",
                                                  params_vec, func_node);
  allocator_free(alloc, &params_vec);

  scope_t scope = vm_get_current_scope(vm);
  vec_push(vm_get_types(vm), gt);

  value_t gen_val = vm_create_value_ref(vm, (type_t)gt,
                                         (const void *)create_fn_instance,
                                         "foo");

  /* Instantiate foo[i32] — should fail (i32 does not extend str) */
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
  value_t i32_type_val = vm_create_value_ref(vm, type_type,
                                              _get_i32_type(vm), NULL);
  value_t inst = value_instantiate(vm, gen_val, 1, &i32_type_val);
  EXPECT_TRUE(value_is_abnormal(inst));
  EXPECT_EQ(type_get_kind(value_get_type(inst)), TYPE_KIND_EXCEPTION);

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}
