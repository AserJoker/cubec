#ifndef _H_cubec_CUBEC_PATH_
#define _H_cubec_CUBEC_PATH_

#include "core/allocator.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cubec_path_t *cubec_path_t;

cubec_path_t cubec_create_path(cubec_allocator_t allocator, const char *source);

cubec_path_t cubec_path_concat(cubec_path_t current,
                               cubec_allocator_t allocator,
                               cubec_path_t another);

bool cubec_path_append(cubec_path_t path, cubec_allocator_t allocator,
                       const char *part);

cubec_path_t cubec_path_clone(cubec_path_t current,
                              cubec_allocator_t allocator);

cubec_path_t cubec_path_current(cubec_allocator_t allocator);

cubec_path_t cubec_path_absolute(cubec_path_t current,
                                 cubec_allocator_t allocator);

cubec_path_t cubec_path_parent(cubec_path_t current,
                               cubec_allocator_t allocator);

const char *cubec_path_filename(cubec_path_t current);

const char *cubec_path_extname(cubec_path_t current);

char *cubec_path_to_string(cubec_path_t path, cubec_allocator_t allocator);

#ifdef __cplusplus
};
#endif
#endif