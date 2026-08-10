#ifndef _H_CUBEC_ENGINE_NAME_
#define _H_CUBEC_ENGINE_NAME_

#include "core/allocator.h"
#ifdef __cplusplus
extern "C" {
#endif

enum name_kind {
  NAME_VARIABLE,
  NAME_TYPE,
  NAME_FUNCTION,
  NAME_NAMESPACE
};

struct _name_t {
  allocator_t allocator;
  enum name_kind kind;
  void *ref; /* borrowing: points to the semantic object (function_t/namespace_t/etc) */
};

typedef struct _name_t *name_t;

name_t name_create(allocator_t allocator, enum name_kind kind, void *ref);
void name_dispose(name_t name);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_ENGINE_NAME_ */
