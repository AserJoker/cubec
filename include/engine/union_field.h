#ifndef _H_CUBEC_ENGINE_UNION_FIELD_
#define _H_CUBEC_ENGINE_UNION_FIELD_

#include "core/allocator.h"
#include "engine/stype.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Union field — a single field in a union or cunion.
 *
 * No offset — all union fields overlap at offset 0.
 * name is owned (strdup'd).
 * type is borrowing (points into context->types).
 * is_pub controls cross-module field visibility.
 */
struct _union_field_t {
  allocator_t allocator;
  char *name;    /* owned */
  stype_t type;  /* borrowing */
  bool is_pub;   /* cross-module visibility */
};

typedef struct _union_field_t *union_field_t;

union_field_t union_field_create(allocator_t allocator, const char *name,
                                 stype_t type, bool is_pub);
void union_field_dispose(union_field_t field);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_UNION_FIELD_ */
