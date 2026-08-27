#include "run/run.h"
#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "engine/integer_type.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/diagnostic.h"
#include "engine/defer.h"
#include "engine/ast_defer.h"
#include "engine/str_type.h"
#include "engine/callable_type.h"
#include "cubec/literal_identifier.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include "cubec/node.h"
#include "core/location.h"
#include "core/vec.h"
#include "core/class.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_defer : public CubecTest {
protected:
  void free_node(node_t &node) {
    if (node) allocator_free(vm_get_allocator(vm), &node);
  }

  void free_tokens(vec_t &tokens) {
    if (tokens) allocator_free(vm_get_allocator(vm), &tokens);
  }

  /* Register a value in current scope under the given name */
  void _bind(const char *name, value_t val) {
    scope_t scope = vm_get_current_scope(vm);
    name_t n = name_create(scope->allocator, val);
    strmap_insert(scope->names, name, n);
  }

  /* Parse source code into a program node via lexer->parser */
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

  /* Get the integer value of a name in current scope */
  int32_t _get_int(const char *name) {
    name_t n = scope_lookup(vm_get_current_scope(vm), name);
    if (!n || !n->ref) return -999;
    return *(int32_t *)value_get_data(n->ref);
  }

  type_t _i32() { return (type_t)value_get_data(vm_get_i32_type(vm)); }
  type_t _void() { return (type_t)value_get_data(vm_get_void_type(vm)); }

  size_t _error_count() {
    return diagnostic_list_get_error_count(vm_get_diagnostics(vm));
  }

  void _clear_diagnostics() {
    diagnostic_list_clear(vm_get_diagnostics(vm));
  }

  location_t _loc() {
    location_t loc;
    memset(&loc, 0, sizeof(loc));
    loc.filename = "test";
    return loc;
  }
};

/* ==== Basic defer class ==== */

TEST_F(it_defer, defer_class_init_dispose) {
  vm_t vm2 = vm_create(allocator);

  defer_init_t init = {.func = NULL, .closure_scope = NULL,
                       .root_scope = vm_get_root_scope(vm2)};
  defer_t d = (defer_t)allocator_create(allocator, &g_defer_class, &init);
  EXPECT_NE(d, nullptr);
  EXPECT_EQ(d->func, nullptr);
  EXPECT_EQ(d->closure_scope, nullptr);
  EXPECT_EQ(d->root_scope, vm_get_root_scope(vm2));

  allocator_free(allocator, &d);
  vm_dispose(vm2, allocator);
}

TEST_F(it_defer, ast_defer_class_init_dispose) {
  vm_t vm2 = vm_create(allocator);
  allocator_t alloc = vm_get_allocator(vm2);

  scope_t closure = scope_create(alloc, SCOPE_DEFER, NULL, NULL);
  scope_t template_scope = scope_create(alloc, SCOPE_TYPE, closure, NULL);

  ast_defer_init_t init = {
      .closure_scope = closure,
      .root_scope = vm_get_root_scope(vm2),
      .node = NULL,
      .template_scope = template_scope,
  };
  ast_defer_t ad =
      (ast_defer_t)allocator_create(alloc, &g_ast_defer_class, &init);
  EXPECT_NE(ad, nullptr);
  EXPECT_EQ(ad->base.func, nullptr);
  EXPECT_NE(ad->base.closure_scope, nullptr);
  EXPECT_EQ(ad->base.root_scope, vm_get_root_scope(vm2));
  EXPECT_EQ(ad->node, nullptr);
  EXPECT_NE(ad->template_scope, nullptr);
  EXPECT_EQ(ad->template_scope->parent, ad->base.closure_scope);

  allocator_free(alloc, &ad);
  vm_dispose(vm2, allocator);
}

/* ==== Basic defer execution (inside blocks) ==== */

TEST_F(it_defer, defer_executes_on_block_exit) {
  int32_t x_val = 0;
  (void)vm_create_value(vm, _i32(), &x_val, "x");

  value_t v = _run_source("{ defer { x = 1; }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("x"), 1);
}

TEST_F(it_defer, defer_lifo_order) {
  int32_t x_val = 0;
  (void)vm_create_value(vm, _i32(), &x_val, "x");

  value_t v = _run_source("{ defer { x = 1; }; defer { x = 2; }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("x"), 1);
}

TEST_F(it_defer, defer_three_in_lifo_order) {
  int32_t x_val = 0;
  (void)vm_create_value(vm, _i32(), &x_val, "x");

  value_t v = _run_source(
      "{ defer { x = 1; }; defer { x = 2; }; defer { x = 3; }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("x"), 1);
}

/* ==== Defer with captures ==== */

TEST_F(it_defer, defer_with_explicit_captures) {
  int32_t x_val = 10;
  (void)vm_create_value(vm, _i32(), &x_val, "x");

  value_t v = _run_source("{ defer |x| { x = x + 1; }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  /* Captured x = 10, defer sets closure copy to 11.
   * The outer x is unaffected — still 10. */
  EXPECT_EQ(_get_int("x"), 10);
}

TEST_F(it_defer, defer_capture_preserves_value_at_registration) {
  int32_t x_val = 5;
  (void)vm_create_value(vm, _i32(), &x_val, "x");
  int32_t y_val = 0;
  (void)vm_create_value(vm, _i32(), &y_val, "y");

  value_t v = _run_source("{ defer |x| { y = x; }; x = 99; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  /* defer captured x=5, then x was changed to 99. defer sets y=5 */
  EXPECT_EQ(_get_int("y"), 5);
}

/* ==== Shadow mode ==== */

TEST_F(it_defer, shadow_mode_does_not_register_defer) {
  int32_t x_val = 0;
  (void)vm_create_value(vm, _i32(), &x_val, "x");

  value_t v = _run_source("{ defer { x = 1; }; }", true);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("x"), 0);
}

TEST_F(it_defer, shadow_mode_return_in_defer_is_error) {
  (void)_run_source("func foo(): void { defer { return; }; }", true);
  EXPECT_GE(_error_count(), 1u);
  _clear_diagnostics();
}

/* ==== Defer in nested blocks ==== */

TEST_F(it_defer, defer_in_nested_block) {
  int32_t x_val = 0;
  (void)vm_create_value(vm, _i32(), &x_val, "x");

  value_t v = _run_source("{ { defer { x = 1; }; } }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("x"), 1);
}

/* ==== Defer without captures accesses scope chain ==== */

TEST_F(it_defer, defer_without_captures_accesses_scope) {
  int32_t x_val = 5;
  (void)vm_create_value(vm, _i32(), &x_val, "x");

  value_t v = _run_source("{ defer { x = 42; }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("x"), 42);
}

/* ==== Empty defer body ==== */

TEST_F(it_defer, defer_empty_body) {
  int32_t x_val = 10;
  (void)vm_create_value(vm, _i32(), &x_val, "x");

  value_t v = _run_source("{ defer { }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("x"), 10);
}

/* ==== Defer with multiple statements ==== */

TEST_F(it_defer, defer_with_multiple_statements) {
  int32_t x_val = 0;
  (void)vm_create_value(vm, _i32(), &x_val, "x");
  int32_t y_val = 0;
  (void)vm_create_value(vm, _i32(), &y_val, "y");

  value_t v = _run_source("{ defer { x = 1; y = 2; }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("x"), 1);
  EXPECT_EQ(_get_int("y"), 2);
}

/* ==== Defer in function with return ==== */

TEST_F(it_defer, defer_executes_on_function_return) {
  /* Pre-bind a mutable counter in current scope */
  int32_t counter_val = 0;
  (void)vm_create_value(vm, _i32(), &counter_val, "counter");

  /* Define and call a function with defer + return in the same source
   * (AST nodes are freed after _run_source, so function definition and call
   * must be in the same parse unit). */
  value_t v = _run_source(
      "func foo(): void { defer { counter = 42; }; return; }"
      "foo();");
  _clear_diagnostics();  /* clear shadow-mode assignment diagnostic */
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);

  /* The defer should have set counter = 42 during scope unwind */
  EXPECT_EQ(_get_int("counter"), 42);
}

TEST_F(it_defer, defer_executes_on_function_return_with_value) {
  /* Function with defer returns a value — defer still runs during unwind */
  int32_t counter_val = 0;
  (void)vm_create_value(vm, _i32(), &counter_val, "counter");
  int32_t result_val = 0;
  (void)vm_create_value(vm, _i32(), &result_val, "result");

  /* Define and call in the same source, storing result */
  value_t v = _run_source(
      "func foo(): i32 { defer { counter = 99; }; return 7; }"
      "result = foo();");
  _clear_diagnostics();
  /* The defer should have set counter = 99 */
  EXPECT_EQ(_get_int("counter"), 99);
  /* The function returned 7, stored in result */
  EXPECT_EQ(_get_int("result"), 7);
}

/* ==== Defer in while loop with break ==== */

TEST_F(it_defer, defer_executes_on_loop_break) {
  /* defer inside a while loop body — runs each iteration, including
   * when break exits the loop body scope. */
  int32_t counter_val = 0;
  (void)vm_create_value(vm, _i32(), &counter_val, "counter");
  int32_t i_val = 0;
  (void)vm_create_value(vm, _i32(), &i_val, "i");

  /* while (true) { defer { counter = counter + 1; }; i = i + 1; if (i >= 3) { break; } } */
  value_t v = _run_source(
      "{ while (true) { defer { counter = counter + 1; }; i = i + 1; if (i >= 3) { break; } }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  /* defer runs 3 times (iterations 1, 2, 3 before break) */
  EXPECT_EQ(_get_int("counter"), 3);
  EXPECT_EQ(_get_int("i"), 3);
}

/* ==== Defer in while loop with continue ==== */

TEST_F(it_defer, defer_executes_on_loop_continue) {
  int32_t counter_val = 0;
  (void)vm_create_value(vm, _i32(), &counter_val, "counter");
  int32_t i_val = 0;
  (void)vm_create_value(vm, _i32(), &i_val, "i");

  /* Loop that continues — defer runs each iteration including continue path */
  value_t v = _run_source(
      "{ while (i < 3) { defer { counter = counter + 1; }; i = i + 1; continue; counter = 999; }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  /* defer runs 3 times (each iteration, continue skips counter=999 but defer
   * still runs when block scope exits via the interrupt unwind) */
  EXPECT_EQ(_get_int("counter"), 3);
  EXPECT_EQ(_get_int("i"), 3);
}

/* ==== Scope validation: defer not allowed in struct ==== */

TEST_F(it_defer, defer_not_allowed_in_struct) {
  value_t v = _run_source(
      "struct Foo { defer { x = 1; }; }");
  /* Should return an exception */
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

TEST_F(it_defer, defer_not_allowed_in_struct_shadow) {
  (void)_run_source(
      "struct Foo { defer { x = 1; }; }", true);
  EXPECT_GE(_error_count(), 1u);
  _clear_diagnostics();
}

/* ==== Scope validation: defer not allowed in union ==== */

TEST_F(it_defer, defer_not_allowed_in_union) {
  value_t v = _run_source(
      "union Bar { defer { x = 1; }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ==== Scope validation: defer not allowed in enum ==== */

TEST_F(it_defer, defer_not_allowed_in_enum) {
  value_t v = _run_source(
      "enum Baz { defer { x = 1; }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ==== Scope validation: defer not allowed in interface ==== */

TEST_F(it_defer, defer_not_allowed_in_interface) {
  value_t v = _run_source(
      "interface Qux { defer { x = 1; }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_EXCEPTION);
}

/* ==== Defer captures multiple variables ==== */

TEST_F(it_defer, defer_captures_multiple_variables) {
  int32_t x_val = 10;
  (void)vm_create_value(vm, _i32(), &x_val, "x");
  int32_t y_val = 20;
  (void)vm_create_value(vm, _i32(), &y_val, "y");
  int32_t result_val = 0;
  (void)vm_create_value(vm, _i32(), &result_val, "result");

  /* defer |x, y| { result = x + y; } — captures both x and y by value */
  value_t v = _run_source("{ defer |x, y| { result = x + y; }; }");
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("result"), 30);
}

/* ================================================================
 * using statement tests
 * ================================================================ */

/* ==== using creates a defer (basic plumbing) ==== */

TEST_F(it_defer, using_creates_defer_with_target) {
  /* Verify that using creates a defer_t with the target field set.
   * We can't easily test __dispose__ with struct methods (self-referential
   * type annotations not yet supported), so test the mechanism directly. */
  int32_t val = 42;
  value_t i32_val = vm_create_value(vm, _i32(), &val, "tmp");

  /* Create a defer_t with target = cloned value (same pattern as using) */
  scope_t closure = scope_create(vm_get_allocator(vm), SCOPE_DEFER, NULL, NULL);
  scope_t prev = vm_set_scope(vm, closure);
  value_t clone = value_clone(vm, i32_val);
  vm_set_scope(vm, prev);

  ASSERT_FALSE(value_is_abnormal(clone));

  defer_init_t dinit = {
      .func = NULL,
      .closure_scope = closure,
      .root_scope = vm_get_root_scope(vm),
      .target = clone,
  };
  defer_t d =
      (defer_t)allocator_create(vm_get_allocator(vm), &g_defer_class, &dinit);

  /* Verify the defer_t has target set */
  EXPECT_EQ(d->target, clone);
  EXPECT_EQ(d->func, nullptr);

  /* Pop the scope — this triggers vm_run_defers, which dispatches
   * to the target branch and calls value_member_call. i32 doesn't
   * support member_call, so the defer returns an exception, but
   * vm_exec_defer only stops on interrupts (exceptions are swallowed). */
  vec_push(vm_get_current_scope(vm)->defers, d);
  vm_pop_scope(vm);

  /* If we reach here without crashing, the using defer dispatch works */
  SUCCEED();
}

/* ==== using with i32 (no __dispose__) — defer exception is swallowed ==== */

TEST_F(it_defer, using_without_dispose_swallows_exception) {
  /* using x = 0; creates a defer. At scope exit, value_member_call on i32
   * returns an exception ("type 'i32' does not support member call"), but
   * vm_exec_defer only checks for interrupts, not exceptions.
   * The block itself returns void (the using statement succeeded). */
  int32_t x_val = 0;
  (void)vm_create_value(vm, _i32(), &x_val, "x");

  value_t v = _run_source("{ using x = 0; }");
  /* The block returns void — the defer's exception is swallowed */
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ==== using shadow mode does not register defer ==== */

TEST_F(it_defer, using_shadow_no_defer) {
  int32_t x_val = 0;
  (void)vm_create_value(vm, _i32(), &x_val, "x");

  /* Shadow mode: using behaves like var, no defer registered */
  value_t v = _run_source("{ using x = 0; }", true);
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  /* x should be 0 (shadow mode — no defer fires) */
  EXPECT_EQ(_get_int("x"), 0);
}

/* ==== using in block scope ==== */

TEST_F(it_defer, using_in_block_scope) {
  /* using inside a block scope — the defer should fire when the block exits */
  int32_t x_val = 0;
  (void)vm_create_value(vm, _i32(), &x_val, "x");

  value_t v = _run_source("{ using x = 0; }");
  /* Block returns void, defer fires at block exit (exception swallowed for i32) */
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ==== using in function with return ==== */

TEST_F(it_defer, using_in_function_return) {
  /* using inside a function — the defer should fire when the function returns.
   * The defer's exception (i32 has no __dispose__) is swallowed. */
  value_t v = _run_source(
      "func foo(): void { using x = 0; return; }"
      "foo();");
  _clear_diagnostics();
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
}

/* ==== using with defer — both fire in LIFO ==== */

TEST_F(it_defer, using_and_defer_both_fire) {
  /* using creates a defer; an explicit defer also exists.
   * Both should fire in LIFO order (explicit defer registered after using,
   * so explicit defer runs first). */
  int32_t counter_val = 0;
  (void)vm_create_value(vm, _i32(), &counter_val, "counter");

  value_t v = _run_source(
      "{ using x = 0; defer { counter = 42; }; }");
  _clear_diagnostics();
  /* The explicit defer sets counter = 42, then the using defer fires
   * (exception swallowed for i32). Counter should be 42. */
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("counter"), 42);
}

/* ==== using clone is independent from original ==== */

TEST_F(it_defer, using_clone_is_independent) {
  /* using captures the value by clone into the defer's closure_scope.
   * The defer |x| also captures x by clone (the block-scope x created by using).
   * After x is reassigned to 99, the defer's captured clone still has 5. */
  int32_t captured_val = 0;
  (void)vm_create_value(vm, _i32(), &captured_val, "captured");

  /* using x = 5 creates a new binding x=5 in the block scope.
   * defer |x| captures a clone of that x (=5) into its closure.
   * Then x = 99 reassigns the block-scope x.
   * At scope exit, defer sets captured = 5 (from its clone). */
  value_t v = _run_source(
      "{ using x = 5; defer |x| { captured = x; }; x = 99; }");
  _clear_diagnostics();
  EXPECT_EQ(_get_int("captured"), 5);
}

/* ==== using with struct __dispose__ ==== */

TEST_F(it_defer, using_struct_dispose_called) {
  /* struct with only methods (no fields) + empty initialization.
   * Empty struct has size 1 (like C). __dispose__ called on scope exit. */
  int32_t counter_val = 0;
  (void)vm_create_value(vm, _i32(), &counter_val, "counter");

  value_t v = _run_source(
      "struct Tracker { "
      "  func __dispose__(self: *Tracker): void { counter = counter + 1; }; "
      "}; "
      "{ using t = .Tracker{}; }");
  _clear_diagnostics();
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("counter"), 1);
}

/* ==== using struct __dispose__ in function ==== */

TEST_F(it_defer, using_struct_dispose_in_function) {
  int32_t counter_val = 0;
  (void)vm_create_value(vm, _i32(), &counter_val, "counter");

  value_t v = _run_source(
      "struct Tracker { "
      "  func __dispose__(self: *Tracker): void { counter = counter + 1; }; "
      "}; "
      "func foo(): void { using t = .Tracker{}; return; }"
      "foo();");
  _clear_diagnostics();
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("counter"), 1);
}

/* ==== multiple using structs — LIFO ==== */

TEST_F(it_defer, using_struct_multiple_lifo) {
  int32_t counter_val = 0;
  (void)vm_create_value(vm, _i32(), &counter_val, "counter");

  value_t v = _run_source(
      "struct Tracker { "
      "  func __dispose__(self: *Tracker): void { counter = counter + 1; }; "
      "}; "
      "{ using t1 = .Tracker{}; using t2 = .Tracker{}; }");
  _clear_diagnostics();
  EXPECT_EQ(type_get_kind(value_get_type(v)), TYPE_KIND_VOID);
  EXPECT_EQ(_get_int("counter"), 2);
}
