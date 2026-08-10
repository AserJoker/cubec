#ifndef _H_CUBEC_CORE_NODE_
#define _H_CUBEC_CORE_NODE_
#include "core/allocator.h"
#include "core/location.h"
#include "core/class.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Base class for all AST nodes.
 *        Subclasses embed this struct as a `super` field for single inheritance.
 */
struct _node_t;
typedef struct _node_t *node_t;
struct _node_t {
  allocator_t allocator;  /**< The allocator that owns this node */
  uint32_t kind;          /**< AST node kind (see cubec/node.h for enum) */
  location_t location;    /**< Source-code span for this node */
  node_t parent;          /**< Parent node in the AST tree */
};

/** @brief Virtual table for node_t. */
extern class_t g_node_class;

/** @brief Initialization parameters for node_t (passed to allocator_create). */
struct _node_init_t {
  uint32_t kind;          /**< AST node kind */
  location_t location;    /**< Source-code span */
  node_t parent;          /**< Parent node (may be NULL for root) */
};
typedef struct _node_init_t node_init_t;
#ifdef __cplusplus
}
#endif
#endif