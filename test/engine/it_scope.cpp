#include "engine/vm.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/bool_type.h"
#include "core/string.h"
#include "common/test_common.h"
#include <gtest/gtest.h>

using ::testing::Test;

class it_scope : public CubecTest {
protected:
  allocator_t allocator = create_allocator(NULL, NULL);
};

/* ---- Basic scope creation ---- */

TEST_F(it_scope, create_standalone) {
  scope_t s = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  EXPECT_NE(s, nullptr);
  EXPECT_EQ(s->kind, SCOPE_GLOBAL);
  EXPECT_EQ(s->parent, nullptr);
  EXPECT_EQ(vec_get_size(s->children), 0u);
  EXPECT_EQ(strmap_get_size(s->names), 0u);
  EXPECT_EQ(vec_get_size(s->values), 0u);
  scope_dispose(s);
  delete_allocator(allocator);
}

TEST_F(it_scope, create_child) {
  scope_t parent = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  scope_t child = scope_create(allocator, SCOPE_BLOCK, parent, NULL);
  EXPECT_EQ(child->parent, parent);
  EXPECT_EQ(vec_get_size(parent->children), 1u);
  EXPECT_EQ(vec_get(parent->children, 0), child);
  scope_dispose(parent);
  delete_allocator(allocator);
}

TEST_F(it_scope, create_nested_children) {
  scope_t root = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  scope_t func = scope_create(allocator, SCOPE_FUNCTION, root, NULL);
  scope_t block = scope_create(allocator, SCOPE_BLOCK, func, NULL);
  EXPECT_EQ(block->parent, func);
  EXPECT_EQ(func->parent, root);
  EXPECT_EQ(vec_get_size(root->children), 1u);
  EXPECT_EQ(vec_get_size(func->children), 1u);
  scope_dispose(root);
  delete_allocator(allocator);
}

TEST_F(it_scope, dispose_parent_frees_children) {
  scope_t parent = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  scope_t child1 = scope_create(allocator, SCOPE_BLOCK, parent, NULL);
  scope_t child2 = scope_create(allocator, SCOPE_BLOCK, parent, NULL);
  (void)child1;
  (void)child2;
  EXPECT_EQ(vec_get_size(parent->children), 2u);
  /* disposing parent should recursively dispose children */
  scope_dispose(parent);
  delete_allocator(allocator);
}

TEST_F(it_scope, dispose_cascade_nested) {
  scope_t root = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  scope_t func = scope_create(allocator, SCOPE_FUNCTION, root, NULL);
  scope_t block = scope_create(allocator, SCOPE_BLOCK, func, NULL);
  (void)block;
  /* disposing root cascades to func, then to block */
  scope_dispose(root);
  delete_allocator(allocator);
}

/* ---- scope_add_child / scope_remove_child ---- */

TEST_F(it_scope, add_child_manual) {
  scope_t parent = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  scope_t child = scope_create(allocator, SCOPE_BLOCK, NULL, NULL);
  scope_add_child(parent, child);
  child->parent = parent;
  EXPECT_EQ(vec_get_size(parent->children), 1u);
  EXPECT_EQ(vec_get(parent->children, 0), child);
  scope_dispose(parent);
  delete_allocator(allocator);
}

TEST_F(it_scope, remove_child) {
  scope_t parent = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  scope_t child = scope_create(allocator, SCOPE_BLOCK, parent, NULL);
  EXPECT_EQ(vec_get_size(parent->children), 1u);
  scope_remove_child(parent, child);
  EXPECT_EQ(vec_get_size(parent->children), 0u);
  child->parent = NULL;
  scope_dispose(child);
  scope_dispose(parent);
  delete_allocator(allocator);
}

/* ---- scope_lookup ---- */

TEST_F(it_scope, lookup_in_same_scope) {
  vm_t vm = vm_create(allocator);
  scope_t global = vm_get_global_scope(vm);
  /* "bool" is already in global scope from vm_init */
  name_t found = scope_lookup(global, "bool");
  ASSERT_NE(found, nullptr);
  EXPECT_NE(found->ref, nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_scope, lookup_not_found) {
  scope_t s = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  name_t found = scope_lookup(s, "x");
  EXPECT_EQ(found, nullptr);
  scope_dispose(s);
  delete_allocator(allocator);
}

TEST_F(it_scope, lookup_walks_to_parent) {
  vm_t vm = vm_create(allocator);
  allocator_t vm_alloc = vm_get_allocator(vm);
  scope_t global = vm_get_global_scope(vm);
  scope_t child = scope_create(vm_alloc, SCOPE_BLOCK, global, NULL);

  /* "bool" is in global, child should find it by walking up */
  name_t found = scope_lookup(child, "bool");
  ASSERT_NE(found, nullptr);
  EXPECT_NE(found->ref, nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_scope, lookup_prefers_closer_scope) {
  vm_t vm = vm_create(allocator);
  allocator_t vm_alloc = vm_get_allocator(vm);
  scope_t global = vm_get_global_scope(vm);
  scope_t child = scope_create(vm_alloc, SCOPE_BLOCK, global, NULL);

  /* Add a "type" name in child scope (shadows global's "type") */
  value_t v_child = create_bool_value(vm, false);
  name_t n_child = name_create(vm_alloc, v_child);
  char *owned_c = cstring_clone(vm_alloc, "type");
  strmap_insert(child->names, owned_c, n_child);
  allocator_free(vm_alloc, &owned_c);

  /* child lookup finds child's own "type" */
  name_t found = scope_lookup(child, "type");
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->ref, v_child);

  /* global lookup finds global's "type" (the type_type value) */
  found = scope_lookup(global, "type");
  ASSERT_NE(found, nullptr);
  EXPECT_NE(found->ref, v_child);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_scope, lookup_walks_multiple_levels) {
  vm_t vm = vm_create(allocator);
  allocator_t vm_alloc = vm_get_allocator(vm);
  scope_t global = vm_get_global_scope(vm);
  scope_t func = scope_create(vm_alloc, SCOPE_FUNCTION, global, NULL);
  scope_t block = scope_create(vm_alloc, SCOPE_BLOCK, func, NULL);

  /* "bool" is in global, block should find it through func → global */
  name_t found = scope_lookup(block, "bool");
  ASSERT_NE(found, nullptr);
  EXPECT_NE(found->ref, nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_scope, lookup_null_scope) {
  name_t found = scope_lookup(NULL, "x");
  EXPECT_EQ(found, nullptr);
  delete_allocator(allocator);
}

TEST_F(it_scope, lookup_null_name) {
  scope_t s = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  name_t found = scope_lookup(s, NULL);
  EXPECT_EQ(found, nullptr);
  scope_dispose(s);
  delete_allocator(allocator);
}

/* ---- scope with vm: lookup builtin names ---- */

TEST_F(it_scope, vm_global_has_builtin_names) {
  vm_t vm = vm_create(allocator);
  scope_t global = vm_get_global_scope(vm);

  EXPECT_NE(scope_lookup(global, "type"), nullptr);
  EXPECT_NE(scope_lookup(global, "bool"), nullptr);
  EXPECT_NE(scope_lookup(global, "void"), nullptr);
  EXPECT_NE(scope_lookup(global, "error"), nullptr);
  /* wildcard has no name */
  EXPECT_EQ(scope_lookup(global, "?"), nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_scope, vm_child_lookup_builtin) {
  vm_t vm = vm_create(allocator);
  allocator_t vm_alloc = vm_get_allocator(vm);
  scope_t global = vm_get_global_scope(vm);
  scope_t child = scope_create(vm_alloc, SCOPE_BLOCK, global, NULL);

  EXPECT_NE(scope_lookup(child, "type"), nullptr);
  EXPECT_NE(scope_lookup(child, "bool"), nullptr);
  EXPECT_NE(scope_lookup(child, "void"), nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_scope, vm_nested_child_lookup) {
  vm_t vm = vm_create(allocator);
  allocator_t vm_alloc = vm_get_allocator(vm);
  scope_t global = vm_get_global_scope(vm);
  scope_t func = scope_create(vm_alloc, SCOPE_FUNCTION, global, NULL);
  scope_t block = scope_create(vm_alloc, SCOPE_BLOCK, func, NULL);

  EXPECT_NE(scope_lookup(block, "bool"), nullptr);
  EXPECT_NE(scope_lookup(block, "void"), nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_scope, vm_child_lookup_not_found) {
  vm_t vm = vm_create(allocator);
  allocator_t vm_alloc = vm_get_allocator(vm);
  scope_t global = vm_get_global_scope(vm);
  scope_t child = scope_create(vm_alloc, SCOPE_BLOCK, global, NULL);

  EXPECT_EQ(scope_lookup(child, "nonexistent"), nullptr);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

/* ---- Resource cleanup with values ---- */

TEST_F(it_scope, dispose_scope_with_values) {
  vm_t vm = vm_create(allocator);
  /* create_bool_value adds to current_scope (global) */
  value_t v = create_bool_value(vm, true);
  (void)v;
  scope_t global = vm_get_global_scope(vm);
  /* 32 bootstrap types (incl. error struct) + 1 wildcard value + 1 bool value = 34 */
  EXPECT_EQ(vec_get_size(global->values), 34u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}

TEST_F(it_scope, vm_dispose_cleans_child_scope_values) {
  vm_t vm = vm_create(allocator);
  allocator_t vm_alloc = vm_get_allocator(vm);
  scope_t global = vm_get_global_scope(vm);
  scope_t child = scope_create(vm_alloc, SCOPE_BLOCK, global, NULL);

  value_t v = create_bool_value(vm, true);
  (void)v;
  /* bool value is in vm->current_scope (global) */
  EXPECT_EQ(vec_get_size(global->values), 34u); /* 32 bootstrap types + wildcard value + bool_value */

  /* child scope has no values */
  EXPECT_EQ(vec_get_size(child->values), 0u);

  vm_dispose(vm, allocator);
  delete_allocator(allocator);
}
