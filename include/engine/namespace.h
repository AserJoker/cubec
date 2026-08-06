#ifndef _H_CUBEC_ENGINE_NAMESPACE_
#define _H_CUBEC_ENGINE_NAMESPACE_

#include "core/allocator.h"
#include "core/node.h"
#include "engine/def.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _module_t;
typedef struct _module_t *module_t;

/**
 * @brief Namespace — represents an imported module namespace.
 *
 * namespace_t is a declaration-level wrapper around a module reference.
 * It is NOT module_t itself — module_t is a file-level concept.
 *
 * Owned by scope_t. Borrowed by name_t.ref.
 */
struct _namespace_t {
  def_t header;
  module_t *module;   /* pointing to the imported module (NULL in phase 1) */
};

typedef struct _namespace_t *namespace_t;

/**
 * @brief Create a namespace_t.
 * @param allocator  Allocator for this object
 * @param node       AST import statement node (borrowing reference)
 * @return New namespace_t with module=NULL
 */
namespace_t namespace_create(allocator_t allocator, node_t node);

/** @brief Dispose a namespace_t. */
void namespace_dispose(namespace_t ns);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_ENGINE_NAMESPACE_ */
