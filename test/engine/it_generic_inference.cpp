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
#include "engine/generic_inference.h"
#include "engine/ast_func.h"
#include "engine/func.h"
#include "engine/struct_type.h"
#include "engine/union_type.h"
#include "engine/enum_type.h"
#include "engine/pointer_type.h"
#include "engine/array_type.h"
#include "engine/slice_type.h"
#include "core/string.h"
#include "core/vec.h"
#include "core/location.h"
#include "cubec/declaration_function.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_slice.h"
#include "cubec/declaration_callable.h"
#include "cubec/function_argument.h"
#include "cubec/literal_identifier.h"
#include "cubec/literal_nil.h"
#include "cubec/expression_binary.h"
#include "cubec/statement_return.h"
#include "cubec/statement_block.h"
#include "cubec/expression_deref.h"
#include "cubec/generic_param.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_generic_inference : public CubecTest {
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
  type_t _get_u64_type(vm_t vm) {
    return (type_t)value_get_data(vm_get_u64_type(vm));
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
    generic_param_t gp = generic_param_create(alloc, name, type_type, extends, false);
    allocator_free(alloc, &extends);
    return gp;
  }

  /* Create a generic_param_t (value param, e.g. N:u64) */
  generic_param_t _make_value_param(vm_t vm, const char *name, type_t value_type) {
    allocator_t alloc = vm_get_allocator(vm);
    vec_init_t vi = {.auto_dispose = true};
    vec_t extends = (vec_t)allocator_create(alloc, &g_vec_class, &vi);
    generic_param_t gp = generic_param_create(alloc, name, value_type, extends, false);
    allocator_free(alloc, &extends);
    return gp;
  }

  /* Build a generic_fn_type, register in vm->types, create self-reference,
   * and return the generic value. Reduces boilerplate in each test. */
  value_t _build_generic_fn(vm_t vm, const char *name, vec_t generic_params,
                            node_t func_node) {
    allocator_t alloc = vm_get_allocator(vm);
    generic_fn_type_t gt = generic_fn_type_create(alloc, name,
                                                    generic_params, func_node);
    allocator_free(alloc, &generic_params);
    vec_push(vm_get_types(vm), gt);

    /* Register self-reference in gt->scope */
    {
      scope_t gt_scope = generic_fn_type_get_scope(gt);
      value_t self_ref = value_create(alloc, (type_t)gt,
                                      (void *)create_fn_instance, false);
      vec_push(gt_scope->values, self_ref);
      name_t self_name = name_create(gt_scope->allocator, self_ref);
      char *owned = cstring_clone(gt_scope->allocator, name);
      strmap_insert(gt_scope->names, owned, self_name);
      allocator_free(gt_scope->allocator, &owned);
    }

    return vm_create_value_ref(vm, (type_t)gt,
                               (const void *)create_fn_instance, name);
  }

  /* Helper: wrap a type_t as a type value */
  value_t _type_val(vm_t vm, type_t t) {
    type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));
    return vm_create_value_ref(vm, type_type, t, NULL);
  }
};

/* ---- Placeholder type tests ---- */

TEST_F(it_generic_inference, placeholder_type_is_generic_param) {
  vm_t vm = vm_create(allocator);
  type_t ph = generic_param_placeholder_create(allocator, "T");
  vec_push(vm_get_types(vm), ph); /* register for cleanup */
  EXPECT_TRUE(type_is_generic_param_placeholder(ph));
  EXPECT_FALSE(type_is_generic_pack_placeholder(ph));
  EXPECT_STREQ(generic_placeholder_get_name(ph), "T");
  vm_dispose(vm, allocator);
}

TEST_F(it_generic_inference, placeholder_type_is_generic_pack) {
  vm_t vm = vm_create(allocator);
  type_t ph = generic_pack_placeholder_create(allocator, "T");
  vec_push(vm_get_types(vm), ph); /* register for cleanup */
  EXPECT_FALSE(type_is_generic_param_placeholder(ph));
  EXPECT_TRUE(type_is_generic_pack_placeholder(ph));
  EXPECT_STREQ(generic_placeholder_get_name(ph), "T");
  vm_dispose(vm, allocator);
}

/* ---- Inference: identity function ---- */

TEST_F(it_generic_inference, identity_i32_inferred) {
  /* func identity[T](x: T) -> T { return x; }
   * identity(42) — T inferred as i32 */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  /* Build AST */
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

  vec_push(vm_get_types(vm), gt);

  /* Register self-reference in gt->scope for recursion */
  {
    scope_t gt_scope = generic_fn_type_get_scope(gt);
    value_t self_ref = value_create(alloc, (type_t)gt,
                                    (void *)create_fn_instance, false);
    vec_push(gt_scope->values, self_ref);
    name_t self_name = name_create(gt_scope->allocator, self_ref);
    char *owned = cstring_clone(gt_scope->allocator, "identity");
    strmap_insert(gt_scope->names, owned, self_name);
    allocator_free(gt_scope->allocator, &owned);
  }

  /* Create generic value */
  value_t gen_val = vm_create_value_ref(vm, (type_t)gt,
                                         (const void *)create_fn_instance,
                                         "identity");

  /* Call identity(42) — should infer T=i32 */
  int32_t val = 42;
  value_t arg = vm_create_value(vm, _get_i32_type(vm), &val, NULL);
  value_t result = value_call(vm, gen_val, 1, &arg);

  EXPECT_FALSE(value_is_abnormal(result));
  if (!value_is_abnormal(result)) {
    EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
    EXPECT_EQ(*(int32_t *)value_get_data(result), 42);
  }

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

/* ---- Inference: add function with same type param ---- */

TEST_F(it_generic_inference, add_i32_inferred) {
  /* func add[T](a: T, b: T) -> T { return a + b; }
   * add(3, 4) — T inferred as i32 from first arg, second must match */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  /* Build AST */
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

  generic_fn_type_t gt = generic_fn_type_create(alloc, "add",
                                                  params_vec, func_node);
  allocator_free(alloc, &params_vec);

  vec_push(vm_get_types(vm), gt);

  /* Register self-reference */
  {
    scope_t gt_scope = generic_fn_type_get_scope(gt);
    value_t self_ref = value_create(alloc, (type_t)gt,
                                    (void *)create_fn_instance, false);
    vec_push(gt_scope->values, self_ref);
    name_t self_name = name_create(gt_scope->allocator, self_ref);
    char *owned = cstring_clone(gt_scope->allocator, "add");
    strmap_insert(gt_scope->names, owned, self_name);
    allocator_free(gt_scope->allocator, &owned);
  }

  value_t gen_val = vm_create_value_ref(vm, (type_t)gt,
                                         (const void *)create_fn_instance,
                                         "add");

  /* Call add(3, 4) */
  int32_t a = 3, b = 4;
  value_t argv[2] = {
      vm_create_value(vm, _get_i32_type(vm), &a, NULL),
      vm_create_value(vm, _get_i32_type(vm), &b, NULL),
  };
  value_t result = value_call(vm, gen_val, 2, argv);

  EXPECT_FALSE(value_is_abnormal(result));
  if (!value_is_abnormal(result)) {
    EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
    EXPECT_EQ(*(int32_t *)value_get_data(result), 7);
  }

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

/* ---- Inference: type mismatch (T appears twice with different types) ---- */

TEST_F(it_generic_inference, type_mismatch_error) {
  /* func add[T](a: T, b: T) -> T { return a; }
   * add(1, "hi") — T inferred as i32 from first arg, second is str → error */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
  vec_push(args, create_function_argument(vm, loc, "a",
      create_literal_identifier(vm, loc, "T")));
  vec_push(args, create_function_argument(vm, loc, "b",
      create_literal_identifier(vm, loc, "T")));

  node_t ret_type = create_literal_identifier(vm, loc, "T");
  node_t body_expr = create_literal_identifier(vm, loc, "a");
  node_t func_node = _build_func_ast(vm, "add", args, ret_type, body_expr);

  vec_init_t pvi = {.auto_dispose = true};
  vec_t params_vec = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  vec_push(params_vec, _make_type_param(vm, "T"));

  generic_fn_type_t gt = generic_fn_type_create(alloc, "add",
                                                  params_vec, func_node);
  allocator_free(alloc, &params_vec);

  vec_push(vm_get_types(vm), gt);

  value_t gen_val = vm_create_value_ref(vm, (type_t)gt,
                                         (const void *)create_fn_instance,
                                         "add");

  /* Call add(1, "hi") — type mismatch */
  int32_t ival = 1;
  const char *sval = "hi";
  value_t argv[2] = {
      vm_create_value(vm, _get_i32_type(vm), &ival, NULL),
      vm_create_value(vm, _get_str_type(vm), sval, NULL),
  };
  value_t result = value_call(vm, gen_val, 2, argv);

  /* Should return an exception (type mismatch) */
  EXPECT_TRUE(value_is_abnormal(result));

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

/* ---- Inference: no call args → cannot infer ---- */

TEST_F(it_generic_inference, no_args_cannot_infer) {
  /* func foo[T]() -> T { ... }
   * foo() — T cannot be inferred → error */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  /* Empty args */
  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);

  node_t ret_type = create_literal_identifier(vm, loc, "T");
  node_t body_expr = create_literal_nil(vm, loc);
  node_t func_node = _build_func_ast(vm, "foo", args, ret_type, body_expr);

  vec_init_t pvi = {.auto_dispose = true};
  vec_t params_vec = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  vec_push(params_vec, _make_type_param(vm, "T"));

  generic_fn_type_t gt = generic_fn_type_create(alloc, "foo",
                                                  params_vec, func_node);
  allocator_free(alloc, &params_vec);

  vec_push(vm_get_types(vm), gt);

  value_t gen_val = vm_create_value_ref(vm, (type_t)gt,
                                         (const void *)create_fn_instance,
                                         "foo");

  /* Call foo() — no args to infer from */
  value_t result = value_call(vm, gen_val, 0, NULL);

  EXPECT_TRUE(value_is_abnormal(result));

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

/* ---- Inference: pointer type ---- */

TEST_F(it_generic_inference, pointer_inferred) {
  /* func deref[T](p: *T) -> T { return *p; }
   * deref(&42) — T inferred as i32 from *i32 */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  /* Build AST: func deref(p: *T) -> T { return *p; } */
  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
  /* p: *T */
  node_t t_ident = create_literal_identifier(vm, loc, "T");
  node_t ptr_type = create_declaration_pointer(vm, loc, t_ident, false);
  vec_push(args, create_function_argument(vm, loc, "p", ptr_type));

  node_t ret_type = create_literal_identifier(vm, loc, "T");
  node_t p_ident = create_literal_identifier(vm, loc, "p");
  node_t body_expr = create_expression_deref(vm, loc, p_ident);
  node_t func_node = _build_func_ast(vm, "deref", args, ret_type, body_expr);

  /* Build generic params: [T] */
  vec_init_t pvi = {.auto_dispose = true};
  vec_t params_vec = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  vec_push(params_vec, _make_type_param(vm, "T"));

  generic_fn_type_t gt = generic_fn_type_create(alloc, "deref",
                                                  params_vec, func_node);
  allocator_free(alloc, &params_vec);
  vec_push(vm_get_types(vm), gt);

  /* Register self-reference */
  {
    scope_t gt_scope = generic_fn_type_get_scope(gt);
    value_t self_ref = value_create(alloc, (type_t)gt,
                                    (void *)create_fn_instance, false);
    vec_push(gt_scope->values, self_ref);
    name_t self_name = name_create(gt_scope->allocator, self_ref);
    char *owned = cstring_clone(gt_scope->allocator, "deref");
    strmap_insert(gt_scope->names, owned, self_name);
    allocator_free(gt_scope->allocator, &owned);
  }

  value_t gen_val = vm_create_value_ref(vm, (type_t)gt,
                                         (const void *)create_fn_instance,
                                         "deref");

  /* Call deref(&42) — need a *i32 value */
  int32_t val = 42;
  value_t i32_val = vm_create_value(vm, _get_i32_type(vm), &val, NULL);
  type_t i32_type = _get_i32_type(vm);
  value_t ptr_type_val = vm_create_pointer_type_value(vm, i32_type, true, false);
  pointer_type_t pt = (pointer_type_t)value_get_data(ptr_type_val);
  value_t ptr_val = create_pointer_value(vm, pt, i32_val);

  value_t result = value_call(vm, gen_val, 1, &ptr_val);

  EXPECT_FALSE(value_is_abnormal(result));
  if (!value_is_abnormal(result)) {
    EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
    EXPECT_EQ(*(int32_t *)value_get_data(result), 42);
  }

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

/* ---- Inference: struct field type (vtable verification) ---- */

TEST_F(it_generic_inference, struct_field_infer_walk) {
  /* Verify struct infer_walk vtable is set.
   * Full integration test (struct{id:T} vs struct{id:i32} inferring T)
   * requires run_declaration_struct for AST construction — test that later.
   * For now, verify vtable is wired up and struct type dispatch works. */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));

  /* Build struct{id: i32} — no placeholders, just verify vtable */
  value_t struct_val = vm_create_struct_type_value(vm, "TestS", true, "<test>");
  value_t i32_type_val = vm_create_value_ref(vm, type_type, _get_i32_type(vm), NULL);
  vm_struct_add_field(vm, struct_val, "id", i32_type_val, false);
  vm_struct_seal(vm, struct_val);

  struct_type_t st = (struct_type_t)value_get_data(struct_val);
  EXPECT_TRUE(type_get_vtable((type_t)st).infer_walk != NULL);

  /* Verify it matches itself (no placeholders, no ctx needed for kind check) */
  bool match = _struct_infer_walk(vm, (type_t)st, (type_t)st, NULL);
  EXPECT_TRUE(match);

  /* Verify it rejects different kind */
  value_t ptr_tv = vm_create_pointer_type_value(vm, _get_i32_type(vm), true, false);
  pointer_type_t pt = (pointer_type_t)value_get_data(ptr_tv);
  bool mismatch = _struct_infer_walk(vm, (type_t)st, (type_t)pt, NULL);
  EXPECT_FALSE(mismatch);

  vm_dispose(vm, allocator);
}

/* ---- Inference: verify vtable infer_walk is set for all composite types ---- */

TEST_F(it_generic_inference, vtable_infer_walk_set_for_composite_types) {
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));

  /* Pointer type */
  value_t ptr_tv = vm_create_pointer_type_value(vm, _get_i32_type(vm), true, false);
  pointer_type_t pt = (pointer_type_t)value_get_data(ptr_tv);
  EXPECT_TRUE(type_get_vtable((type_t)pt).infer_walk != NULL);

  /* Array type */
  value_t count_val = create_i32_value(vm, 3);
  value_t arr_tv = vm_create_array_type_value(vm, _get_i32_type(vm), count_val, true);
  array_type_t at = (array_type_t)value_get_data(arr_tv);
  EXPECT_TRUE(type_get_vtable((type_t)at).infer_walk != NULL);

  /* Slice type */
  value_t slc_tv = vm_create_slice_type_value(vm, _get_i32_type(vm), true);
  slice_type_t st = (slice_type_t)value_get_data(slc_tv);
  EXPECT_TRUE(type_get_vtable((type_t)st).infer_walk != NULL);

  /* Callable type — vm_create_callable_type_value takes vec_t param_types */
  vec_init_t cvi = {.auto_dispose = false};
  vec_t callable_params = (vec_t)allocator_create(alloc, &g_vec_class, &cvi);
  vec_push(callable_params, _get_i32_type(vm));
  value_t callable_tv = vm_create_callable_type_value(vm, callable_params,
                                                       _get_i32_type(vm), false,
                                                       true, "<test>");
  allocator_free(alloc, &callable_params);
  callable_type_t ct = (callable_type_t)value_get_data(callable_tv);
  EXPECT_TRUE(type_get_vtable((type_t)ct).infer_walk != NULL);

  /* Struct type */
  value_t struct_tv = vm_create_struct_type_value(vm, "TestStruct", true, "<test>");
  vm_struct_seal(vm, struct_tv);
  struct_type_t sst = (struct_type_t)value_get_data(struct_tv);
  EXPECT_TRUE(type_get_vtable((type_t)sst).infer_walk != NULL);

  /* Union type */
  value_t union_tv = vm_create_union_type_value(vm, "TestUnion", true, "<test>");
  vm_union_seal(vm, union_tv);
  union_type_t ut = (union_type_t)value_get_data(union_tv);
  EXPECT_TRUE(type_get_vtable((type_t)ut).infer_walk != NULL);

  /* Enum type */
  value_t i32_type_val = vm_create_value_ref(vm, type_type, _get_i32_type(vm), NULL);
  value_t enum_tv = vm_create_enum_type_value(vm, "TestEnum", i32_type_val, true, "<test>");
  enum_type_t et = (enum_type_t)value_get_data(enum_tv);
  EXPECT_TRUE(type_get_vtable((type_t)et).infer_walk != NULL);

  vm_dispose(vm, allocator);
}

/* ---- Inference: slice type ---- */

TEST_F(it_generic_inference, slice_inferred) {
  /* func first[T](s: []T) -> T { return s[0]; }
   * first([1,2,3]) — T inferred as i32 from []i32 */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  /* Build AST: func first(s: []T) -> T { return s; }
   * Body is just `s` for simplicity — inference is what we're testing. */
  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
  /* s: []T */
  node_t t_ident = create_literal_identifier(vm, loc, "T");
  node_t slice_type_node = create_declaration_slice(vm, loc, t_ident, false, false);
  vec_push(args, create_function_argument(vm, loc, "s", slice_type_node));

  node_t ret_type = create_literal_identifier(vm, loc, "T");
  node_t body_expr = create_literal_identifier(vm, loc, "s");
  node_t func_node = _build_func_ast(vm, "first", args, ret_type, body_expr);

  /* Build generic params: [T] */
  vec_init_t pvi = {.auto_dispose = true};
  vec_t params_vec = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  vec_push(params_vec, _make_type_param(vm, "T"));

  value_t gen_val = _build_generic_fn(vm, "first", params_vec, func_node);

  /* Create a []i32 slice value via [3]i32 array */
  int32_t vals[] = {10, 20, 30};
  value_t i32_arr[3];
  for (int i = 0; i < 3; i++)
    i32_arr[i] = vm_create_value(vm, _get_i32_type(vm), &vals[i], NULL);

  value_t count_val = create_i32_value(vm, 3);
  value_t arr_tv = vm_create_array_type_value(vm, _get_i32_type(vm), count_val, true);
  array_type_t at = (array_type_t)value_get_data(arr_tv);
  value_t arr_val = create_array_value(vm, at, i32_arr);

  value_t slc_tv = vm_create_slice_type_value(vm, _get_i32_type(vm), true);
  slice_type_t st = (slice_type_t)value_get_data(slc_tv);
  value_t slice_val = create_slice_value(vm, st, arr_val, 0, 3);

  /* Call first(slice_of_i32) — T should be inferred as i32 */
  value_t result = value_call(vm, gen_val, 1, &slice_val);

  /* Body returns `s` ([]i32), return type is T=i32 — safe_cast will fail,
   * but inference itself should succeed. Check for "could not infer" error. */
  if (value_is_abnormal(result)) {
    type_t rtype = value_get_type(result);
    if (type_get_kind(rtype) == TYPE_KIND_EXCEPTION) {
      const char *msg = (const char *)value_get_data(result);
      EXPECT_TRUE(strstr(msg, "could not infer") == NULL)
          << "Inference should succeed: " << msg;
    }
  }

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

/* ---- Inference: array with value param ---- */

TEST_F(it_generic_inference, array_value_param_inferred) {
  /* func get[N:u64](arr: [N]i32) -> i32 { return arr[0]; }
   * get([10,20,30]) — N inferred as 3 from [3]i32 */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  /* Build AST: func get(arr: [N]i32) -> i32 */
  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
  /* arr: [N]i32 */
  node_t n_ident = create_literal_identifier(vm, loc, "N");
  node_t i32_ident = create_literal_identifier(vm, loc, "i32");
  node_t arr_type_node = create_declaration_array(vm, loc, n_ident, i32_ident);
  vec_push(args, create_function_argument(vm, loc, "arr", arr_type_node));

  node_t ret_type = create_literal_identifier(vm, loc, "i32");
  node_t body_expr = create_literal_identifier(vm, loc, "arr");
  node_t func_node = _build_func_ast(vm, "get", args, ret_type, body_expr);

  /* Build generic params: [N:u64] */
  vec_init_t pvi = {.auto_dispose = true};
  vec_t params_vec = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  vec_push(params_vec, _make_value_param(vm, "N", _get_u64_type(vm)));

  value_t gen_val = _build_generic_fn(vm, "get", params_vec, func_node);

  /* Create a [3]i32 array value */
  int32_t vals[] = {10, 20, 30};
  value_t i32_arr[3];
  for (int i = 0; i < 3; i++)
    i32_arr[i] = vm_create_value(vm, _get_i32_type(vm), &vals[i], NULL);

  value_t count_val = create_i32_value(vm, 3);
  value_t arr_tv = vm_create_array_type_value(vm, _get_i32_type(vm), count_val, true);
  array_type_t at = (array_type_t)value_get_data(arr_tv);
  value_t arr_val = create_array_value(vm, at, i32_arr);

  /* Call get([10,20,30]) — N should be inferred as 3 */
  value_t result = value_call(vm, gen_val, 1, &arr_val);

  /* Inference should succeed — check not a "could not infer" error.
   * The body returns `arr` ([N]i32) but return type is i32, so
   * safe_cast may fail — that's OK, we're only testing inference. */
  if (value_is_abnormal(result)) {
    type_t rtype = value_get_type(result);
    if (type_get_kind(rtype) == TYPE_KIND_EXCEPTION) {
      const char *msg = (const char *)value_get_data(result);
      EXPECT_TRUE(strstr(msg, "could not infer") == NULL)
          << "Inference should succeed: " << msg;
    }
  }

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

/* ---- Inference: callable type ---- */

TEST_F(it_generic_inference, callable_inferred) {
  /* func apply[T](f: func(T)->T, x: T) -> T { return x; }
   * apply(fn, 42) — T inferred as i32 from both args */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  /* Build AST: func apply(f: func(T)->T, x: T) -> T */
  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);

  /* f: func(T)->T */
  vec_init_t cpi = {.auto_dispose = true};
  vec_t callable_params = (vec_t)allocator_create(alloc, &g_vec_class, &cpi);
  vec_push(callable_params, create_literal_identifier(vm, loc, "T"));
  node_t callable_ret = create_literal_identifier(vm, loc, "T");
  node_t callable_type_node = create_declaration_callable(vm, loc, callable_params,
                                                           callable_ret, false);
  vec_push(args, create_function_argument(vm, loc, "f", callable_type_node));

  /* x: T */
  vec_push(args, create_function_argument(vm, loc, "x",
      create_literal_identifier(vm, loc, "T")));

  node_t ret_type = create_literal_identifier(vm, loc, "T");
  node_t body_expr = create_literal_identifier(vm, loc, "x");
  node_t func_node = _build_func_ast(vm, "apply", args, ret_type, body_expr);

  /* Build generic params: [T] */
  vec_init_t pvi = {.auto_dispose = true};
  vec_t params_vec = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  vec_push(params_vec, _make_type_param(vm, "T"));

  value_t gen_val = _build_generic_fn(vm, "apply", params_vec, func_node);

  /* Create a func(i32)->i32 callable value */
  vec_init_t cvi2 = {.auto_dispose = false};
  vec_t cparam_types = (vec_t)allocator_create(alloc, &g_vec_class, &cvi2);
  vec_push(cparam_types, _get_i32_type(vm));
  value_t callable_tv = vm_create_callable_type_value(vm, cparam_types,
                                                       _get_i32_type(vm), false,
                                                       true, "<test>");
  allocator_free(alloc, &cparam_types);
  callable_type_t ct = (callable_type_t)value_get_data(callable_tv);

  /* Create a simple identity function as the callable argument */
  vec_init_t id_args_vi = {.auto_dispose = true};
  vec_t id_args = (vec_t)allocator_create(alloc, &g_vec_class, &id_args_vi);
  vec_push(id_args, create_function_argument(vm, loc, "x",
      create_literal_identifier(vm, loc, "i32")));
  node_t id_ret = create_literal_identifier(vm, loc, "i32");
  node_t id_body = create_literal_identifier(vm, loc, "x");
  node_t id_func = _build_func_ast(vm, "identity", id_args, id_ret, id_body);

  value_t fn_val = create_ast_func_value(vm, ct, NULL, id_func, NULL);

  /* Create i32 arg */
  int32_t val = 42;
  value_t i32_val = vm_create_value(vm, _get_i32_type(vm), &val, NULL);

  /* Call apply(identity_fn, 42) — T should be inferred as i32 */
  value_t call_argv[2] = {fn_val, i32_val};
  value_t result = value_call(vm, gen_val, 2, call_argv);

  /* Body returns `x` which is i32, return type T=i32 — should work */
  EXPECT_FALSE(value_is_abnormal(result));
  if (!value_is_abnormal(result)) {
    EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_I32);
    EXPECT_EQ(*(int32_t *)value_get_data(result), 42);
  }

  allocator_free(alloc, &func_node);
  allocator_free(alloc, &id_func);
  vm_dispose(vm, allocator);
}

/* ---- Inference: nested pointer → pointer ---- */

TEST_F(it_generic_inference, nested_pointer_inferred) {
  /* func get_ptr[T](pp: **T) -> *T { return *pp; }
   * get_ptr(&&42) — T inferred as i32 from **i32 */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  location_t loc = _dummy_loc();

  /* Build AST: func get_ptr(pp: **T) -> *T { return *pp; } */
  vec_init_t avi = {.auto_dispose = true};
  vec_t args = (vec_t)allocator_create(alloc, &g_vec_class, &avi);
  /* pp: **T */
  node_t t_ident = create_literal_identifier(vm, loc, "T");
  node_t inner_ptr = create_declaration_pointer(vm, loc, t_ident, false);
  node_t outer_ptr = create_declaration_pointer(vm, loc, inner_ptr, false);
  vec_push(args, create_function_argument(vm, loc, "pp", outer_ptr));

  /* return type: *T */
  node_t ret_t = create_literal_identifier(vm, loc, "T");
  node_t ret_ptr = create_declaration_pointer(vm, loc, ret_t, false);
  /* body: *pp */
  node_t pp_ident = create_literal_identifier(vm, loc, "pp");
  node_t body_expr = create_expression_deref(vm, loc, pp_ident);
  node_t func_node = _build_func_ast(vm, "get_ptr", args, ret_ptr, body_expr);

  /* Build generic params: [T] */
  vec_init_t pvi = {.auto_dispose = true};
  vec_t params_vec = (vec_t)allocator_create(alloc, &g_vec_class, &pvi);
  vec_push(params_vec, _make_type_param(vm, "T"));

  value_t gen_val = _build_generic_fn(vm, "get_ptr", params_vec, func_node);

  /* Create a **i32 value */
  int32_t val = 42;
  value_t i32_val = vm_create_value(vm, _get_i32_type(vm), &val, NULL);
  type_t i32_type = _get_i32_type(vm);

  /* *i32 type and value */
  value_t ptr_tv = vm_create_pointer_type_value(vm, i32_type, true, false);
  pointer_type_t pt = (pointer_type_t)value_get_data(ptr_tv);
  value_t ptr_val = create_pointer_value(vm, pt, i32_val);

  /* **i32 type and value */
  value_t pptr_tv = vm_create_pointer_type_value(vm, (type_t)pt, true, false);
  pointer_type_t ppt = (pointer_type_t)value_get_data(pptr_tv);
  value_t pptr_val = create_pointer_value(vm, ppt, ptr_val);

  /* Call get_ptr(&&42) — T inferred as i32 from **i32 */
  value_t result = value_call(vm, gen_val, 1, &pptr_val);

  EXPECT_FALSE(value_is_abnormal(result));
  if (!value_is_abnormal(result)) {
    /* Result should be *i32 (a pointer to 42) */
    EXPECT_EQ(type_get_kind(value_get_type(result)), TYPE_KIND_POINTER);
  }

  allocator_free(alloc, &func_node);
  vm_dispose(vm, allocator);
}

/* ---- Inference: struct field type via manual infer_walk ---- */

TEST_F(it_generic_inference, struct_field_type_inferred) {
  /* Verify that struct{id: T} vs struct{id: i32} infers T=i32
   * via _struct_infer_walk with manual context construction.
   * This tests the full infer_walk dispatch without needing run_declaration_struct. */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));

  /* Build formal struct type: struct{id: T} where T is a placeholder */
  type_t t_placeholder = generic_param_placeholder_create(alloc, "T");
  vec_push(vm_get_types(vm), t_placeholder);

  value_t formal_struct_val = vm_create_struct_type_value(vm, "FormalS", true, "<test>");
  value_t placeholder_type_val = vm_create_value_ref(vm, type_type, t_placeholder, NULL);
  vm_struct_add_field(vm, formal_struct_val, "id", placeholder_type_val, false);
  vm_struct_seal(vm, formal_struct_val);
  struct_type_t formal_st = (struct_type_t)value_get_data(formal_struct_val);

  /* Build actual struct type: struct{id: i32} */
  value_t actual_struct_val = vm_create_struct_type_value(vm, "ActualS", true, "<test>");
  value_t i32_type_val = vm_create_value_ref(vm, type_type, _get_i32_type(vm), NULL);
  vm_struct_add_field(vm, actual_struct_val, "id", i32_type_val, false);
  vm_struct_seal(vm, actual_struct_val);
  struct_type_t actual_st = (struct_type_t)value_get_data(actual_struct_val);

  /* Construct infer context with one entry: T (type param) */
  infer_entry_t entries[1];
  memset(entries, 0, sizeof(entries));
  entries[0].name = "T";
  entries[0].placeholder = t_placeholder;
  entries[0].inferred_value = NULL;
  entries[0].is_pack = false;
  entries[0].is_value_param = false;
  entries[0].param_type = NULL;
  entries[0].inferred_pack_values = NULL;

  infer_ctx_t ctx = infer_ctx_create(alloc, entries, 1);

  /* Run infer_walk */
  bool ok = _struct_infer_walk(vm, (type_t)formal_st, (type_t)actual_st, ctx);
  EXPECT_TRUE(ok);

  /* Verify T was inferred as i32 */
  value_t inferred = infer_ctx_get_inferred(ctx, 0);
  EXPECT_TRUE(inferred != NULL);
  if (inferred) {
    EXPECT_EQ(type_get_kind(value_get_type(inferred)), TYPE_KIND_TYPE);
    type_t inferred_type = (type_t)value_get_data(inferred);
    EXPECT_EQ(inferred_type, _get_i32_type(vm));
  }

  allocator_free(alloc, &ctx);
  vm_dispose(vm, allocator);
}

/* ---- Inference: struct with multiple fields, partial inference ---- */

TEST_F(it_generic_inference, struct_multi_field_inferred) {
  /* Verify struct{x: T, y: T} vs struct{x: i32, y: i32} infers T=i32
   * and struct{x: T, y: U} vs struct{x: i32, y: bool} infers T=i32, U=bool */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));

  /* --- Case 1: same type param T in two fields --- */
  type_t t_placeholder = generic_param_placeholder_create(alloc, "T");
  vec_push(vm_get_types(vm), t_placeholder);

  value_t fs1 = vm_create_struct_type_value(vm, "SameT", true, "<test>");
  value_t t_val = vm_create_value_ref(vm, type_type, t_placeholder, NULL);
  vm_struct_add_field(vm, fs1, "x", t_val, false);
  vm_struct_add_field(vm, fs1, "y", t_val, false);
  vm_struct_seal(vm, fs1);
  struct_type_t fst1 = (struct_type_t)value_get_data(fs1);

  value_t as1 = vm_create_struct_type_value(vm, "ActualSameT", true, "<test>");
  value_t i32_tv = vm_create_value_ref(vm, type_type, _get_i32_type(vm), NULL);
  vm_struct_add_field(vm, as1, "x", i32_tv, false);
  vm_struct_add_field(vm, as1, "y", i32_tv, false);
  vm_struct_seal(vm, as1);
  struct_type_t ast1 = (struct_type_t)value_get_data(as1);

  infer_entry_t entries1[1];
  memset(entries1, 0, sizeof(entries1));
  entries1[0].name = "T";
  entries1[0].placeholder = t_placeholder;
  entries1[0].is_value_param = false;

  infer_ctx_t ctx1 = infer_ctx_create(alloc, entries1, 1);
  bool ok1 = _struct_infer_walk(vm, (type_t)fst1, (type_t)ast1, ctx1);
  EXPECT_TRUE(ok1);
  value_t inf1 = infer_ctx_get_inferred(ctx1, 0);
  EXPECT_TRUE(inf1 != NULL);
  if (inf1) {
    type_t inferred_type = (type_t)value_get_data(inf1);
    EXPECT_EQ(inferred_type, _get_i32_type(vm));
  }
  allocator_free(alloc, &ctx1);

  /* --- Case 2: two different type params T, U --- */
  type_t u_placeholder = generic_param_placeholder_create(alloc, "U");
  vec_push(vm_get_types(vm), u_placeholder);

  value_t fs2 = vm_create_struct_type_value(vm, "TwoParams", true, "<test>");
  value_t t_val2 = vm_create_value_ref(vm, type_type, t_placeholder, NULL);
  value_t u_val2 = vm_create_value_ref(vm, type_type, u_placeholder, NULL);
  vm_struct_add_field(vm, fs2, "x", t_val2, false);
  vm_struct_add_field(vm, fs2, "y", u_val2, false);
  vm_struct_seal(vm, fs2);
  struct_type_t fst2 = (struct_type_t)value_get_data(fs2);

  value_t as2 = vm_create_struct_type_value(vm, "ActualTwoParams", true, "<test>");
  value_t i32_tv2 = vm_create_value_ref(vm, type_type, _get_i32_type(vm), NULL);
  value_t bool_tv = vm_create_value_ref(vm, type_type, _get_bool_type(vm), NULL);
  vm_struct_add_field(vm, as2, "x", i32_tv2, false);
  vm_struct_add_field(vm, as2, "y", bool_tv, false);
  vm_struct_seal(vm, as2);
  struct_type_t ast2 = (struct_type_t)value_get_data(as2);

  infer_entry_t entries2[2];
  memset(entries2, 0, sizeof(entries2));
  entries2[0].name = "T";
  entries2[0].placeholder = t_placeholder;
  entries2[0].is_value_param = false;
  entries2[1].name = "U";
  entries2[1].placeholder = u_placeholder;
  entries2[1].is_value_param = false;

  infer_ctx_t ctx2 = infer_ctx_create(alloc, entries2, 2);
  bool ok2 = _struct_infer_walk(vm, (type_t)fst2, (type_t)ast2, ctx2);
  EXPECT_TRUE(ok2);
  value_t inf_t = infer_ctx_get_inferred(ctx2, 0);
  value_t inf_u = infer_ctx_get_inferred(ctx2, 1);
  EXPECT_TRUE(inf_t != NULL);
  EXPECT_TRUE(inf_u != NULL);
  if (inf_t && inf_u) {
    EXPECT_EQ((type_t)value_get_data(inf_t), _get_i32_type(vm));
    EXPECT_EQ((type_t)value_get_data(inf_u), _get_bool_type(vm));
  }
  allocator_free(alloc, &ctx2);

  vm_dispose(vm, allocator);
}

/* ---- Inference: union field type via manual infer_walk ---- */

TEST_F(it_generic_inference, union_field_type_inferred) {
  /* Verify union{val: T, err: E} vs union{val: i32, err: str} infers T=i32, E=str */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));

  /* Build formal union type: union{val: T, err: E} */
  type_t t_placeholder = generic_param_placeholder_create(alloc, "T");
  type_t e_placeholder = generic_param_placeholder_create(alloc, "E");
  vec_push(vm_get_types(vm), t_placeholder);
  vec_push(vm_get_types(vm), e_placeholder);

  value_t fu = vm_create_union_type_value(vm, "Result", true, "<test>");
  value_t t_val = vm_create_value_ref(vm, type_type, t_placeholder, NULL);
  value_t e_val = vm_create_value_ref(vm, type_type, e_placeholder, NULL);
  vm_union_add_field(vm, fu, "val", t_val, false);
  vm_union_add_field(vm, fu, "err", e_val, false);
  vm_union_seal(vm, fu);
  union_type_t fut = (union_type_t)value_get_data(fu);

  /* Build actual union type: union{val: i32, err: str} */
  value_t au = vm_create_union_type_value(vm, "ActualResult", true, "<test>");
  value_t i32_tv = vm_create_value_ref(vm, type_type, _get_i32_type(vm), NULL);
  value_t str_tv = vm_create_value_ref(vm, type_type, _get_str_type(vm), NULL);
  vm_union_add_field(vm, au, "val", i32_tv, false);
  vm_union_add_field(vm, au, "err", str_tv, false);
  vm_union_seal(vm, au);
  union_type_t aut = (union_type_t)value_get_data(au);

  /* Construct infer context */
  infer_entry_t entries[2];
  memset(entries, 0, sizeof(entries));
  entries[0].name = "T";
  entries[0].placeholder = t_placeholder;
  entries[0].is_value_param = false;
  entries[1].name = "E";
  entries[1].placeholder = e_placeholder;
  entries[1].is_value_param = false;

  infer_ctx_t ctx = infer_ctx_create(alloc, entries, 2);
  bool ok = _union_infer_walk(vm, (type_t)fut, (type_t)aut, ctx);
  EXPECT_TRUE(ok);
  value_t inf_t = infer_ctx_get_inferred(ctx, 0);
  value_t inf_e = infer_ctx_get_inferred(ctx, 1);
  EXPECT_TRUE(inf_t != NULL);
  EXPECT_TRUE(inf_e != NULL);
  if (inf_t && inf_e) {
    EXPECT_EQ((type_t)value_get_data(inf_t), _get_i32_type(vm));
    EXPECT_EQ((type_t)value_get_data(inf_e), _get_str_type(vm));
  }
  allocator_free(alloc, &ctx);

  vm_dispose(vm, allocator);
}

/* ---- Inference: enum underlying type via manual infer_walk ---- */

TEST_F(it_generic_inference, enum_underlying_type_inferred) {
  /* Verify enum(T) vs enum(i32) infers T=i32 */
  vm_t vm = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm);
  type_t type_type = (type_t)value_get_data(vm_get_type_type(vm));

  /* Build formal enum type: enum(T) with items A, B */
  type_t t_placeholder = generic_param_placeholder_create(alloc, "T");
  vec_push(vm_get_types(vm), t_placeholder);

  value_t t_val = vm_create_value_ref(vm, type_type, t_placeholder, NULL);
  value_t fe = vm_create_enum_type_value(vm, "FormalE", t_val, true, "<test>");
  enum_type_t fet = (enum_type_t)value_get_data(fe);
  enum_type_add_item(vm, fet, "A", NULL);
  enum_type_add_item(vm, fet, "B", NULL);

  /* Build actual enum type: enum(i32) with items A, B */
  value_t i32_tv = vm_create_value_ref(vm, type_type, _get_i32_type(vm), NULL);
  value_t ae = vm_create_enum_type_value(vm, "ActualE", i32_tv, true, "<test>");
  enum_type_t aet = (enum_type_t)value_get_data(ae);
  enum_type_add_item(vm, aet, "A", NULL);
  enum_type_add_item(vm, aet, "B", NULL);

  /* Construct infer context */
  infer_entry_t entries[1];
  memset(entries, 0, sizeof(entries));
  entries[0].name = "T";
  entries[0].placeholder = t_placeholder;
  entries[0].is_value_param = false;

  infer_ctx_t ctx = infer_ctx_create(alloc, entries, 1);
  bool ok = _enum_infer_walk(vm, (type_t)fet, (type_t)aet, ctx);
  EXPECT_TRUE(ok);
  value_t inferred = infer_ctx_get_inferred(ctx, 0);
  EXPECT_TRUE(inferred != NULL);
  if (inferred) {
    EXPECT_EQ((type_t)value_get_data(inferred), _get_i32_type(vm));
  }
  allocator_free(alloc, &ctx);

  vm_dispose(vm, allocator);
}
