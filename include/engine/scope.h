#ifndef _H_CUBEC_ENGINE_SCOPE_
#define _H_CUBEC_ENGINE_SCOPE_

#include "core/allocator.h"
#include "core/strmap.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

enum scope_kind {
  SCOPE_GLOBAL,
  SCOPE_MODULE,
  SCOPE_FUNCTION,
  SCOPE_BLOCK,
  SCOPE_FOR,
  SCOPE_FOREACH
};

struct _scope_t {
  allocator_t allocator;
  enum scope_kind kind;
  struct _scope_t *parent; /* parent scope */
  vec_t children;          /* child scopes (auto-dispose vec) */
  strmap_t names;          /* name table: text → name_t */
  vec_t defers;            /* defer entries (empty for now) */
  void *owner;             /* borrowing pointer to owning object (module/function) */
};

typedef struct _scope_t *scope_t;

scope_t scope_create(allocator_t allocator, enum scope_kind kind,
                     struct _scope_t *parent, void *owner);
void scope_add_child(struct _scope_t *parent, scope_t child);
void scope_remove_child(struct _scope_t *parent, scope_t child);
void scope_dispose(scope_t scope);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_ENGINE_SCOPE_ */
