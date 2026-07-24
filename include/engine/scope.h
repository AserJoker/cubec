#ifndef _H_CUBEC_ENGINE_SCOPE_
#define _H_CUBEC_ENGINE_SCOPE_
#include "core/location.h"
#include "core/type.h"
#include "core/vec.h"
#include "engine/symbol.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Scope kinds for different semantic contexts.
 */
enum scope_kind {
  SCOPE_GLOBAL,        /**< Top-level module scope */
  SCOPE_FUNCTION,      /**< Function body scope */
  SCOPE_BLOCK,         /**< Block scope (if/else/while/etc.) */
  SCOPE_FOR,           /**< For loop scope (includes loop variable) */
  SCOPE_FOREACH,       /**< Foreach loop scope */
  SCOPE_COMPTIME,      /**< comptime block scope */
  SCOPE_TYPE_INSTANCE, /**< Instance method scope (. member access) */
  SCOPE_TYPE_STATIC,   /**< Static scope (:: member access) */
};

/**
 * @brief A lexical scope in the semantic analysis.
 *        Scopes form a parent chain; name lookup walks up the chain.
 */
struct _scope_t;
typedef struct _scope_t *scope_t;

/** @brief Virtual table for scope. */
extern type_t g_scope_type;

/**
 * @brief Create a new scope.
 * @param parent   Parent scope (NULL for global scope).
 * @param kind     Scope kind.
 * @param location Location where the scope begins.
 */
scope_t scope_create(allocator_t allocator, scope_t parent,
                     enum scope_kind kind, location_t location);

/** @brief Get the parent scope. */
scope_t scope_get_parent(scope_t self);

/** @brief Get the scope kind. */
enum scope_kind scope_get_kind(scope_t self);

/** @brief Add a symbol to this scope. */
void scope_push_symbol(scope_t self, struct symbol *sym);

/**
 * @brief Look up a symbol by name, walking up the scope chain.
 *        For TYPE_INSTANCE and TYPE_STATIC scopes, only searches that scope
 *        and its type-parent chain (not the lexical parent).
 * @return The symbol, or NULL if not found.
 */
struct symbol *scope_lookup(scope_t self, const char *name);

/**
 * @brief Look up a symbol in this scope only (no parent chain).
 * @return The symbol, or NULL if not found.
 */
struct symbol *scope_lookup_local(scope_t self, const char *name);

/**
 * @brief Get the symbols vector for this scope.
 *        Used for iterating all symbols (e.g. re-export).
 * @return The vec of symbol* (do NOT modify).
 */
vec_t scope_get_symbols(scope_t self);

/**
 * @brief Look up a symbol in instance scope (. access).
 *        Searches the TYPE_INSTANCE scope chain only.
 */
struct symbol *scope_lookup_instance(scope_t self, const char *name);

/**
 * @brief Look up a symbol in static scope (:: access).
 *        Searches the TYPE_STATIC scope chain only.
 */
struct symbol *scope_lookup_static(scope_t self, const char *name);

#ifdef __cplusplus
}
#endif
#endif
