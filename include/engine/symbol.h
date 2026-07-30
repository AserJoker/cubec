#ifndef _H_CUBEC_ENGINE_SYMBOL_
#define _H_CUBEC_ENGINE_SYMBOL_
#include "core/location.h"
#include "core/type.h"
#include "core/node.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Symbol kinds in the semantic analysis.
 */
enum symbol_kind {
  SYMBOL_VARIABLE,      /**< Local/global variable */
  SYMBOL_FUNCTION,      /**< Function */
  SYMBOL_TYPE,          /**< Type name (struct, enum, union, interface, alias) */
  SYMBOL_MODULE,        /**< Imported module */
  SYMBOL_FIELD,         /**< Struct/union field */
  SYMBOL_ENUM_ITEM,     /**< Enum variant */
  SYMBOL_GENERIC_PARAM, /**< Generic type parameter */
};

/**
 * @brief Symbol resolution state (TDZ pattern).
 */
enum symbol_state {
  SYMBOL_TDZ,          /**< Temporal Dead Zone: name known, not yet initialized */
  SYMBOL_NAME_KNOWN,   /**< Name registered, type/body not yet resolved */
  SYMBOL_EVALUATED,    /**< Fully resolved and type-checked */
};

/* forward declarations */
struct _scope_t;
typedef struct _scope_t *scope_t;
#include "engine/semantic_type.h"

/**
 * @brief A named symbol in a scope.
 */
struct symbol {
  const char *name;              /**< Symbol name (not owned, points to AST or string_t) */
  enum symbol_kind kind;         /**< Symbol kind */
  enum symbol_state state;       /**< Resolution state */
  bool is_builtin;               /**< true for builtin declarations */
  bool is_export;                /**< true for export declarations (cross-module visibility) */
  bool is_exportlib;             /**< true for exportlib declarations (C ABI, no mangling) */
  location_t location;           /**< Declaration location */
  union {
    /* SYMBOL_VARIABLE */
    struct {
      semantic_type_t type;      /**< Variable type (NULL until resolved) */
      bool is_comptime;          /**< comptime var */
      bool is_mutable;           /**< var (true) vs val (false) */
      bool is_using;             /**< using var (auto-defer __dispose__) */
    } variable;

    /* SYMBOL_FUNCTION */
    struct {
      semantic_type_t type;      /**< Function type (NULL until resolved) */
      bool is_comptime;          /**< comptime func */
      struct symbol *self_param; /**< Method self parameter (NULL for free functions) */
      node_t ast_node;           /**< AST node (cubec_statement_function_t) for comptime dispatch */
      vec_t generic_params;     /**< vec of cubec_generic_param_t (NULL for non-generic) */
    } function;

    /* SYMBOL_TYPE */
    struct {
      semantic_type_t type;      /**< The semantic type (NULL until resolved) */
      vec_t generic_params;     /**< vec of cubec_generic_param_t (NULL for non-generic) */
    } type;

    /* SYMBOL_MODULE */
    struct {
      scope_t scope;             /**< Module's global scope */
    } module;

    /* SYMBOL_FIELD */
    struct {
      semantic_type_t type;      /**< Field type */
      size_t index;              /**< Field index in struct */
      size_t offset;             /**< Byte offset in struct layout */
      bool is_pub;               /**< pub field */
    } field;

    /* SYMBOL_ENUM_ITEM */
    struct {
      long long value;           /**< Enum item value */
      semantic_type_t owning_type; /**< The enum type */
    } enum_item;

    /* SYMBOL_GENERIC_PARAM */
    struct {
      vec_t constraints;         /**< vec of extends constraint types (NULL if unconstrained) */
      semantic_type_t value_type; /**< Value generic type like N: u64 (NULL if type param) */
      bool is_rest;              /**< true for variadic pack parameter (...Args) */
    } generic_param;
  };
};

/** @brief Virtual table for symbol. */
extern type_t g_symbol_type;

/**
 * @brief Create a symbol.
 */
struct symbol *symbol_create(allocator_t allocator, const char *name,
                             enum symbol_kind kind, location_t location);

#ifdef __cplusplus
}
#endif
#endif
