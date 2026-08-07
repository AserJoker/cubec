#ifndef _H_CUBEC_ENGINE_STRUCT_FIELD_
#define _H_CUBEC_ENGINE_STRUCT_FIELD_

#include "core/allocator.h"
#include "engine/stype.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Struct field — a single field in a struct.
 *
 * name is owned (strdup'd).
 * type is borrowing (points into context->types).
 * offset is byte offset within the struct (computed during layout).
 * is_pub controls cross-module field visibility.
 */
struct _struct_field_t {
  allocator_t allocator;
  char *name;       /* owned */
  stype_t type;     /* borrowing */
  uint64_t offset;  /* byte offset within the struct */
  bool is_pub;      /* cross-module visibility */
};

typedef struct _struct_field_t *struct_field_t;

struct_field_t struct_field_create(allocator_t allocator, const char *name,
                                   stype_t type, uint64_t offset, bool is_pub);
void struct_field_dispose(struct_field_t field);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_STRUCT_FIELD_ */
