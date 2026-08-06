#ifndef _H_CUBEC_ENGINE_DEF_
#define _H_CUBEC_ENGINE_DEF_

#include "core/allocator.h"
#include "core/node.h"
#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 *  Definition kinds
 * -------------------------------------------------------------------------- */

enum def_kind {
  DEF_FUNC,
  DEF_STRUCT,
  DEF_UNION,
  DEF_ENUM,
  DEF_INTERFACE,
  DEF_TYPE_ALIAS,
  DEF_VAR,
  DEF_NAMESPACE,
  DEF_CUNION,
};

/* --------------------------------------------------------------------------
 *  Definition type
 *  TODO: detailed def subtypes will be designed alongside eval system
 * -------------------------------------------------------------------------- */

struct _def_t {
  allocator_t allocator;
  enum def_kind kind;
  node_t node; /* borrowing: original AST declaration node */
};

typedef struct _def_t *def_t;

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_ENGINE_DEF_ */
