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
#include "engine/func.h"
#include "engine/ast_func.h"
#include "core/string.h"
#include "core/vec.h"
#include "core/location.h"
#include "cubec/declaration_function.h"
#include "cubec/function_argument.h"
#include "cubec/literal_identifier.h"
#include "cubec/expression_binary.h"
#include "cubec/statement_return.h"
#include "cubec/statement_block.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_ast_func : public CubecTest {
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

  /* Dummy location for hand-built AST nodes */
  location_t _dummy_loc() {
    return (location_t){.filename = "<test>", .begin = {1, 1, NULL},
                        .end = {1, 1, NULL}};
  }

  /* Build an AST for: func <name>(<params>): <return_type> { return <body_expr>; }
   * params is a vec of cubec_function_argument_t (auto_dispose=true).
   * return_type is a literal_identifier node.
   * body_expr is an expression node (e.g. binary +). */
  node_t _build_func_ast(vm_t vm, const char *name, vec_t params,
                          node_t return_type, node_t body_expr) {
    location_t loc = _dummy_loc();

    /* return <body_expr>; */
    node_t ret_stmt = create_statement_return(vm, loc, body_expr);

    /* { return <body_expr>; } */
    vec_init_t bvi = {.auto_dispose = true};
    vec_t stmts = (vec_t)allocator_create(vm_get_allocator(vm), &g_vec_class,
                                           &bvi);
    vec_push(stmts, ret_stmt);
    node_t body = create_statement_block(vm, loc, stmts);

    /* func <name>(<params>): <return_type> { ... } */
    node_t name_node = create_literal_identifier(vm, loc, name);
    return create_declaration_function(vm, loc, name_node, NULL, NULL, params,
                                       return_type, body, false, false, false,
                                       false, false);
  }
};

/* ---- Basic creation ---- */

TEST_F(it_ast_func, create_ast_func_value) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  /* Create a callable type: () -> void */
  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_callable_type_value(vm, params, _get_void_type(vm),
                                             false, true, "<test>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  /* Create an ast_func value with NULL node (placeholder) */
  value_t afv = create_ast_func_value(vm, ct, "test_func", NULL, NULL);
  EXPECT_NE(afv, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(afv)), TYPE_KIND_CALLABLE);
  EXPECT_FALSE(value_is_own(afv)); /* borrowed ref */
  EXPECT_TRUE(value_is_initialized(afv));

  /* ast_func_t registered in current scope's cfuncs */
  scope_t scope = vm_get_current_scope(vm);
  EXPECT_GE(vec_get_size(scope->cfuncs), 1u);

  /* Verify ast_func internals */
  ast_func_t af = (ast_func_t)value_get_data(afv);
  EXPECT_EQ(af->base.func, _ast_func_call);
  EXPECT_STREQ(af->base.name, "test_func");
  EXPECT_EQ(af->base.closure_scope, nullptr);
  EXPECT_NE(af->base.root_scope, nullptr);
  EXPECT_EQ(af->node, nullptr);
  EXPECT_EQ(af->template_scope, nullptr);

  vm_dispose(vm, allocator);
}

TEST_F(it_ast_func, create_with_template_scope) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_callable_type_value(vm, params, _get_void_type(vm),
                                             false, true, "<test>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  /* Create a template scope */
  scope_t tpl_scope = scope_create(alloc, SCOPE_TYPE, NULL, NULL);

  value_t afv = create_ast_func_value(vm, ct, "generic_fn", NULL, tpl_scope);
  ast_func_t af = (ast_func_t)value_get_data(afv);
  EXPECT_EQ(ast_func_get_template_scope(af), tpl_scope);

  vm_dispose(vm, allocator);
}

TEST_F(it_ast_func, ast_func_class_init) {
  /* Test that ast_func_t class initializes all fields correctly */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_callable_type_value(vm, params, _get_i32_type(vm),
                                             false, true, "<test>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  value_t afv = create_ast_func_value(vm, ct, "my_func", NULL, NULL);
  ast_func_t af = (ast_func_t)value_get_data(afv);

  /* base fields */
  EXPECT_EQ(af->base.func, _ast_func_call);
  EXPECT_STREQ(af->base.name, "my_func");
  EXPECT_EQ(af->base.root_scope, vm_get_root_scope(vm));
  EXPECT_EQ(af->base.closure_scope, nullptr);

  /* ast_func fields */
  EXPECT_EQ(ast_func_get_node(af), nullptr);
  EXPECT_EQ(ast_func_get_template_scope(af), nullptr);

  vm_dispose(vm, allocator);
}

TEST_F(it_ast_func, call_without_node_returns_error) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_callable_type_value(vm, params, _get_void_type(vm),
                                             false, true, "<test>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  /* Create with NULL node — calling should return exception */
  value_t afv = create_ast_func_value(vm, ct, "stub", NULL, NULL);

  value_t result = value_call(vm, afv, 0, NULL);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_EXCEPTION);

  vm_dispose(vm, allocator);
}

TEST_F(it_ast_func, ast_func_check_shadow_without_node) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_callable_type_value(vm, params, _get_void_type(vm),
                                             false, true, "<test>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  value_t afv = create_ast_func_value(vm, ct, "stub", NULL, NULL);
  /* check on a value without a node should return void (skip check) */
  value_t result = ast_func_check(vm, afv);
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_VOID);

  vm_dispose(vm, allocator);
}

TEST_F(it_ast_func, dispose_cleans_template_scope) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_callable_type_value(vm, params, _get_void_type(vm),
                                             false, true, "<test>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  scope_t tpl_scope = scope_create(alloc, SCOPE_TYPE, NULL, NULL);
  value_t afv = create_ast_func_value(vm, ct, "gen_fn", NULL, tpl_scope);

  /* vm_dispose should clean up the template_scope via ast_func_dispose */
  vm_dispose(vm, allocator);
}

TEST_F(it_ast_func, dispose_cleans_closure_scope) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_callable_type_value(vm, params, _get_void_type(vm),
                                             false, true, "<test>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  value_t afv = create_ast_func_value(vm, ct, "closure_fn", NULL, NULL);

  /* Create a closure scope and set it */
  int32_t val = 42;
  vm_create_value(vm, _get_i32_type(vm), &val, "captured");
  callable_capture(vm, afv, "captured");

  ast_func_t af = (ast_func_t)value_get_data(afv);
  EXPECT_NE(af->base.closure_scope, nullptr);

  /* vm_dispose should clean up the closure scope */
  vm_dispose(vm, allocator);
}

/* ---- Clone behavior ---- */

TEST_F(it_ast_func, clone_has_no_closure_nor_template) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_callable_type_value(vm, params, _get_void_type(vm),
                                             false, true, "<test>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  scope_t tpl_scope = scope_create(alloc, SCOPE_TYPE, NULL, NULL);
  value_t afv = create_ast_func_value(vm, ct, "original", NULL, tpl_scope);

  /* Capture a variable */
  int32_t val = 99;
  vm_create_value(vm, _get_i32_type(vm), &val, "x");
  callable_capture(vm, afv, "x");

  /* Clone */
  value_t cloned = value_clone(vm, afv);
  ast_func_t orig_af = (ast_func_t)value_get_data(afv);
  ast_func_t clone_af = (ast_func_t)value_get_data(cloned);

  /* Same func pointer and name */
  EXPECT_EQ(orig_af->base.func, clone_af->base.func);
  EXPECT_STREQ(orig_af->base.name, clone_af->base.name);

  /* No closure, no template scope in clone */
  EXPECT_EQ(func_get_closure_scope((func_t)clone_af), nullptr);
  EXPECT_EQ(ast_func_get_template_scope(clone_af), nullptr);

  vm_dispose(vm, allocator);
}

/* ---- Root scope ---- */

TEST_F(it_ast_func, root_scope_is_vm_root) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);

  vec_init_t vi = {.auto_dispose = false};
  vec_t params = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
  value_t tv = vm_create_callable_type_value(vm, params, _get_void_type(vm),
                                             false, true, "<test>");
  allocator_free(alloc, &params);
  callable_type_t ct = (callable_type_t)value_get_data(tv);

  value_t afv = create_ast_func_value(vm, ct, "fn", NULL, NULL);
  ast_func_t af = (ast_func_t)value_get_data(afv);

  EXPECT_EQ(func_get_root_scope((func_t)af), vm_get_root_scope(vm));

  vm_dispose(vm, allocator);
}

/* ---- End-to-end: AST function call ---- */

TEST_F(it_ast_func, e2e_add_i32_i32) {
  /* func add(a: i32, b: i32): i32 { return a + b; } */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  /* Build parameter list */
  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
  vec_push(args, create_function_argument(vm, loc, "a", NULL));
  vec_push(args, create_function_argument(vm, loc, "b", NULL));

  /* Return type: identifier "i32" */
  node_t ret_type = create_literal_identifier(vm, loc, "i32");

  /* Body expression: a + b */
  node_t body_expr = create_expression_binary(
      vm, loc, "+", create_literal_identifier(vm, loc, "a"),
      create_literal_identifier(vm, loc, "b"));

  /* Build full function AST */
  node_t func_node = _build_func_ast(vm, "add", args, ret_type, body_expr);

  /* Create callable type: (i32, i32) -> i32 */
  vec_init_t cvi = {.auto_dispose = false};
  vec_t ct_params = (vec_t)allocator_create(alloc, &g_vec_class, &cvi);
  vec_push(ct_params, _get_i32_type(vm));
  vec_push(ct_params, _get_i32_type(vm));
  value_t ctv = vm_create_callable_type_value(vm, ct_params,
                                               _get_i32_type(vm), false, true,
                                               "<test>");
  allocator_free(alloc, &ct_params);
  callable_type_t ct = (callable_type_t)value_get_data(ctv);

  /* Create AST function value */
  value_t add_fn = create_ast_func_value(vm, ct, "add", func_node, NULL);

  /* Call add(3, 4) */
  int32_t a = 3, b = 4;
  value_t argv[2] = {
      vm_create_value(vm, _get_i32_type(vm), &a, NULL),
      vm_create_value(vm, _get_i32_type(vm), &b, NULL),
  };
  value_t result = value_call(vm, add_fn, 2, argv);

  EXPECT_FALSE(value_is_abnormal(result));
  EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(result), 7);

  /* AST nodes are allocated via allocator_create — free the root node tree */
  allocator_free(alloc, &func_node);

  vm_dispose(vm, allocator);
}

TEST_F(it_ast_func, e2e_add_i32_str_throws) {
  /* func add(a: i32, b: str): i32 { return a + b; }
   * i32 + str is invalid — should produce an exception */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  /* Build parameter list */
  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
  vec_push(args, create_function_argument(vm, loc, "a", NULL));
  vec_push(args, create_function_argument(vm, loc, "b", NULL));

  /* Return type: identifier "i32" */
  node_t ret_type = create_literal_identifier(vm, loc, "i32");

  /* Body expression: a + b */
  node_t body_expr = create_expression_binary(
      vm, loc, "+", create_literal_identifier(vm, loc, "a"),
      create_literal_identifier(vm, loc, "b"));

  /* Build full function AST */
  node_t func_node = _build_func_ast(vm, "add", args, ret_type, body_expr);

  /* Create callable type: (i32, str) -> i32 */
  vec_init_t cvi = {.auto_dispose = false};
  vec_t ct_params = (vec_t)allocator_create(alloc, &g_vec_class, &cvi);
  vec_push(ct_params, _get_i32_type(vm));
  vec_push(ct_params, _get_str_type(vm));
  value_t ctv = vm_create_callable_type_value(vm, ct_params,
                                               _get_i32_type(vm), false, true,
                                               "<test>");
  allocator_free(alloc, &ct_params);
  callable_type_t ct = (callable_type_t)value_get_data(ctv);

  /* Create AST function value */
  value_t add_fn = create_ast_func_value(vm, ct, "add", func_node, NULL);

  /* Call add(3, "hello") */
  int32_t a = 3;
  value_t argv[2] = {
      vm_create_value(vm, _get_i32_type(vm), &a, NULL),
      create_str_value(vm, "hello"),
  };
  value_t result = value_call(vm, add_fn, 2, argv);

  /* i32 + str should fail — either at safe_cast (callable vtable casts str to
   * i32) or inside the body when value_add rejects the operand type */
  EXPECT_TRUE(value_is_abnormal(result));

  /* Clean up AST node */
  allocator_free(alloc, &func_node);

  vm_dispose(vm, allocator);
}
