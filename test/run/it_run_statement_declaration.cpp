#include "run/run.h"
#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/str_type.h"
#include "engine/pointer_type.h"
#include "engine/array_type.h"
#include "engine/generic_type.h"
#include "cubec/token.h"
#include "cubec/program.h"
#include "cubec/node.h"
#include "core/location.h"
#include "core/vec.h"
#include "core/class.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

/* ------------------------------------------------------------------ *
 *  it_run_statement_declaration — end-to-end tests for variable
 *  declaration and type alias runners via lexer→parser→run_program.
 * ------------------------------------------------------------------ */

class it_run_statement_declaration : public CubecTest {
protected:
  void free_node(node_t &node) {
    if (node) allocator_free(vm_get_allocator(vm), &node);
  }

  void free_tokens(vec_t &tokens) {
    if (tokens) allocator_free(vm_get_allocator(vm), &tokens);
  }

  /* Parse source code into a program node via lexer→parser */
  node_t _parse(const char *source) {
    vec_t tokens = resolve_token_list(vm, "test.cubec", source);
    if (!tokens) return NULL;
    size_t position = 0;
    node_t node = read_program_node(vm, tokens, &position, "test.cubec");
    free_tokens(tokens);
    return node;
  }

  /* Run a program node */
  value_t _run(node_t node, bool shadow = false) {
    return run_program(vm, node, shadow);
  }

  /* Parse + run a source string in one step */
  value_t _run_source(const char *source, bool shadow = false) {
    node_t node = _parse(source);
    value_t v = _run(node, shadow);
    free_node(node);
    return v;
  }

  size_t _error_count() {
    return diagnostic_list_get_error_count(vm_get_diagnostics(vm));
  }

  void _clear_diagnostics() {
    diagnostic_list_clear(vm_get_diagnostics(vm));
  }
};

/* ==================================================================
 *  Variable declaration: normal
 * ================================================================== */

TEST_F(it_run_statement_declaration, var_inferred_type) {
  value_t v = _run_source("var x = 42;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "x");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 42);
}

TEST_F(it_run_statement_declaration, var_explicit_type) {
  value_t v = _run_source("var x: i32 = 42;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "x");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 42);
}

TEST_F(it_run_statement_declaration, var_safe_cast_widening) {
  /* i32 → i64: widening integer safe_cast succeeds */
  value_t v = _run_source("var x: i64 = 10;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "x");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_I64);
  EXPECT_EQ(*(int64_t *)value_get_data(n->ref), 10);
}

TEST_F(it_run_statement_declaration, var_safe_cast_mismatch_error) {
  /* i32 → bool: incompatible safe_cast → error */
  value_t v = _run_source("var x: bool = 42;");
  EXPECT_TRUE(value_is_error(v));
}

TEST_F(it_run_statement_declaration, var_no_initializer_error) {
  /* non-extern/non-builtin var without initializer → error */
  value_t v = _run_source("var x: i32;");
  EXPECT_TRUE(value_is_error(v));
}

TEST_F(it_run_statement_declaration, var_bool_init) {
  value_t v = _run_source("var b = true;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "b");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_BOOL);
  EXPECT_TRUE(*(bool *)value_get_data(n->ref));
}

TEST_F(it_run_statement_declaration, var_expression_init) {
  value_t v = _run_source("var x = 10 + 20;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "x");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 30);
}

/* ==================================================================
 *  Variable declaration: extern / builtin
 * ================================================================== */

TEST_F(it_run_statement_declaration, extern_var_creates_shadow) {
  value_t v = _run_source("extern var x: i32;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "x");
  ASSERT_NE(n, nullptr);
  EXPECT_TRUE(value_is_shadow(n->ref));
  EXPECT_TRUE(value_is_initialized(n->ref));
}

TEST_F(it_run_statement_declaration, builtin_var_creates_shadow) {
  value_t v = _run_source("builtin var x: i32;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "x");
  ASSERT_NE(n, nullptr);
  EXPECT_TRUE(value_is_shadow(n->ref));
  EXPECT_TRUE(value_is_initialized(n->ref));
}

TEST_F(it_run_statement_declaration, extern_var_no_type_error) {
  /* extern without type annotation → error */
  value_t v = _run_source("extern var x;");
  EXPECT_TRUE(value_is_error(v));
}

/* ==================================================================
 *  Variable declaration: comptime
 * ================================================================== */

TEST_F(it_run_statement_declaration, comptime_var_eval) {
  value_t v = _run_source("comptime var PI = 3;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "PI");
  ASSERT_NE(n, nullptr);
  /* comptime evaluates with shadow=true, so result is a shadow value */
  EXPECT_TRUE(value_is_shadow(n->ref));
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_I32);
}

TEST_F(it_run_statement_declaration, comptime_var_no_init_error) {
  /* comptime without initializer → error */
  value_t v = _run_source("comptime var PI: i32;");
  EXPECT_TRUE(value_is_error(v));
}

/* ==================================================================
 *  Variable declaration: undefined (TDZ)
 * ================================================================== */

TEST_F(it_run_statement_declaration, undefined_creates_tdz) {
  value_t v = _run_source("var x: i32 = undefined;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "x");
  ASSERT_NE(n, nullptr);
  EXPECT_TRUE(value_is_shadow(n->ref));
  EXPECT_FALSE(value_is_initialized(n->ref));
}

TEST_F(it_run_statement_declaration, undefined_no_type_error) {
  /* undefined without type annotation → error */
  value_t v = _run_source("var x = undefined;");
  EXPECT_TRUE(value_is_error(v));
}

/* ==================================================================
 *  Variable declaration: shadow mode
 * ================================================================== */

TEST_F(it_run_statement_declaration, shadow_mode_var_produces_shadow) {
  /* In shadow mode, var initializer produces shadow value */
  value_t v = _run_source("var x = 42;", true);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "x");
  ASSERT_NE(n, nullptr);
  EXPECT_TRUE(value_is_shadow(n->ref));
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_I32);
}

/* ==================================================================
 *  Type alias: normal
 * ================================================================== */

TEST_F(it_run_statement_declaration, type_alias_simple) {
  value_t v = _run_source("type MyInt = i32;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "MyInt");
  ASSERT_NE(n, nullptr);
  /* MyInt should be a type value */
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_TYPE);
}

TEST_F(it_run_statement_declaration, type_alias_complex) {
  value_t v = _run_source("type Arr = [3]i32;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Arr");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_TYPE);
}

TEST_F(it_run_statement_declaration, type_alias_usable_as_type) {
  /* Define type alias, then use it in a var declaration */
  value_t v = _run_source("type MyInt = i32; var x: MyInt = 10;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "x");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_I32);
  EXPECT_EQ(*(int32_t *)value_get_data(n->ref), 10);
}

TEST_F(it_run_statement_declaration, type_alias_pointer) {
  value_t v = _run_source("type Ptr = *i32;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Ptr");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_TYPE);
}

/* ==================================================================
 *  Type alias: builtin
 * ================================================================== */

TEST_F(it_run_statement_declaration, builtin_type_registers_name) {
  /* builtin type i32 should already be in global scope; runner
   * verifies and registers in current scope */
  value_t v = _run_source("builtin type i32;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ==================================================================
 *  Type alias: generic
 * ================================================================== */

TEST_F(it_run_statement_declaration, type_alias_generic_creates_generic_value) {
  /* type Array[T] = [3]T; should create a generic type value */
  value_t v = _run_source("type Array[T] = [3]T;");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Array");
  ASSERT_NE(n, nullptr);
  /* The bound value should be a generic type */
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_GENERIC);
}

TEST_F(it_run_statement_declaration, type_alias_generic_instantiation) {
  /* Define generic type alias, instantiate it, bind result to a new name.
   * Directly test the instantiation path: type Arr = Array[i32]; */
  value_t v = _run_source("type Array[T] = [3]T; type Arr = Array[i32];");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Arr");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_TYPE);
  type_t arr_type = (type_t)value_get_data(n->ref);
  EXPECT_EQ(type_get_kind(arr_type), TYPE_KIND_ARRAY);
}

TEST_F(it_run_statement_declaration, type_alias_generic_no_type_value_error) {
  /* generic type alias without type expression → error */
  value_t v = _run_source("type Foo[T];");
  EXPECT_TRUE(value_is_error(v));
}

TEST_F(it_run_statement_declaration, type_alias_generic_value_param) {
  /* type Array[T, N: u64] = [N]T; — value param N: u64 + type param T */
  value_t v = _run_source("type Array[T, N: u64] = [N]T; type Arr = Array[i32, 4];");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "Arr");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_TYPE);
  type_t arr_type = (type_t)value_get_data(n->ref);
  EXPECT_EQ(type_get_kind(arr_type), TYPE_KIND_ARRAY);
  /* verify the array is [4]i32 */
  array_type_t at = (array_type_t)arr_type;
  EXPECT_EQ(array_type_get_count(at), 4u);
  EXPECT_EQ(type_get_kind(array_type_get_element_type(at)), TYPE_KIND_I32);
}

TEST_F(it_run_statement_declaration, type_alias_generic_value_param_different_args) {
  /* different concrete args produce different instances */
  value_t v = _run_source("type Array[T, N: u64] = [N]T; type A1 = Array[i32, 3]; type A2 = Array[i64, 5];");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n1 = scope_lookup(scope, "A1");
  name_t n2 = scope_lookup(scope, "A2");
  ASSERT_NE(n1, nullptr);
  ASSERT_NE(n2, nullptr);

  array_type_t at1 = (array_type_t)value_get_data(n1->ref);
  array_type_t at2 = (array_type_t)value_get_data(n2->ref);

  EXPECT_EQ(array_type_get_count(at1), 3u);
  EXPECT_EQ(type_get_kind(array_type_get_element_type(at1)), TYPE_KIND_I32);
  EXPECT_EQ(array_type_get_count(at2), 5u);
  EXPECT_EQ(type_get_kind(array_type_get_element_type(at2)), TYPE_KIND_I64);
}

/* ==================================================================
 *  Multiple declarations in block
 * ================================================================== */

TEST_F(it_run_statement_declaration, multiple_vars_in_block) {
  value_t v = _run_source("{ var a = 1; var b = 2; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

TEST_F(it_run_statement_declaration, var_and_type_in_block) {
  value_t v = _run_source("{ type T = i32; var x: T = 5; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ==================================================================
 *  Builtin generic type: remove_const
 * ================================================================== */

TEST_F(it_run_statement_declaration, builtin_generic_remove_const_decl) {
  /* builtin generic type with extends const ? constraint */
  value_t v = _run_source("builtin type remove_const[T extends const ?];");
  if (value_is_error(v)) {
    const char *msg = (const char *)value_get_data(v);
    FAIL() << "unexpected error: " << (msg ? msg : "(null)");
  }
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "remove_const");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_GENERIC);
}

TEST_F(it_run_statement_declaration, builtin_generic_remove_const_instantiate) {
  /* remove_const[const i32] should produce mutable i32 */
  value_t v = _run_source(
      "builtin type remove_const[T extends const ?]; "
      "type T = remove_const[const i32];");
  if (value_is_error(v)) {
    const char *msg = (const char *)value_get_data(v);
    FAIL() << "unexpected error: " << (msg ? msg : "(null)");
  }
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  scope_t scope = vm_get_current_scope(vm);
  name_t n = scope_lookup(scope, "T");
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(type_get_kind(value_get_type(n->ref)), TYPE_KIND_TYPE);
  type_t result_type = (type_t)value_get_data(n->ref);
  EXPECT_EQ(type_get_kind(result_type), TYPE_KIND_I32);
  EXPECT_TRUE(type_is_mut(result_type));
}

TEST_F(it_run_statement_declaration, builtin_generic_remove_const_extends_violation) {
  /* remove_const[i32] should fail — i32 is mutable, not const,
   * violates T extends const ? constraint */
  value_t v = _run_source(
      "builtin type remove_const[T extends const ?]; "
      "type T = remove_const[i32];");
  EXPECT_TRUE(value_is_error(v));
}
