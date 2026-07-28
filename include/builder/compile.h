#ifndef _H_CUBEC_BUILD_COMPILE_
#define _H_CUBEC_BUILD_COMPILE_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compile a single .c file to .o. Returns 0 on success.
   On success, *out_obj_path is set to a malloc'd string the caller must free. */
int compile_source(const char *src_path, const char *build_dir,
                   const char *cc, const char *cflags,
                   char **out_obj_path);

/* Check whether obj_path is newer than src_path (incremental build). */
bool compile_is_up_to_date(const char *obj_path, const char *src_path);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_BUILD_COMPILE_ */
