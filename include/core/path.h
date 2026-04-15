#ifndef _H_CUBEC_PATH_
#define _H_CUBEC_PATH_

#include "core/allocator.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _path_t *path_t;

path_t create_path(allocator_t allocator, const char *source);

path_t path_concat(path_t current, allocator_t allocator, path_t another);

bool path_append(path_t path, allocator_t allocator, const char *part);

path_t path_clone(path_t current, allocator_t allocator);

path_t path_current(allocator_t allocator);

path_t path_absolute(path_t current, allocator_t allocator);

path_t path_parent(path_t current, allocator_t allocator);

const char *path_filename(path_t current);

const char *path_extname(path_t current);

char *path_to_string(path_t path, allocator_t allocator);

#ifdef __cplusplus
};
#endif
#endif