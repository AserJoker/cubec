#ifndef _H_CUBEC_ENGINE_DEF_
#define _H_CUBEC_ENGINE_DEF_

#include "core/allocator.h"
#include "core/node.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Semantic object classification. */
enum def_kind {
  DEF_TYPE,      /**< type_t — named or composite type */
  DEF_VALUE,     /**< value_t — variable/constant binding */
  DEF_FUNCTION,  /**< function_t — function template */
  DEF_NAMESPACE  /**< namespace_t — imported module namespace */
};

/**
 * @brief Common header embedded in all semantic objects.
 *
 * This struct is always the FIRST field of type_t, value_t, function_t,
 * and namespace_t. It allows type-erased access via def_kind.
 *
 * def_t is never instantiated standalone — it is only embedded.
 */
struct _def_t {
  allocator_t allocator;
  enum def_kind kind;
  node_t node; /* borrowing: original AST declaration node */
};

typedef struct _def_t def_t;

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_ENGINE_DEF_ */
