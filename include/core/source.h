#ifndef _H_CUBEC_CORE_SOURCE_
#define _H_CUBEC_CORE_SOURCE_
#include "core/strmap.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vec.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

struct source_entry {
  string_t content;
  vec_t line_offsets;
};

struct _source_cache_t;
typedef struct _source_cache_t *source_cache_t;

extern type_t g_source_cache_type;

struct source_entry *source_cache_load(source_cache_t self,
                                       const char *filename,
                                       const char *content,
                                       bool take_ownership);

struct source_entry *source_cache_find(source_cache_t self,
                                       const char *filename);

const char *source_entry_get_line(struct source_entry *entry, size_t line);

size_t source_entry_get_line_count(struct source_entry *entry);

#ifdef __cplusplus
}
#endif
#endif
