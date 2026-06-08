#ifndef _H_CORE_FS_
#define _H_CORE_FS_
#include "core/allocator.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
char *fs_read_file(allocator_t allocator, const char *filename);
bool fs_is_exists(const char *filename);
#ifdef __cplusplus
}
#endif
#endif