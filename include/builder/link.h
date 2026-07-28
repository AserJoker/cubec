#ifndef _H_CUBEC_BUILD_LINK_
#define _H_CUBEC_BUILD_LINK_

#ifdef __cplusplus
extern "C" {
#endif

/* Link object files into an executable. Returns 0 on success. */
int link_executable(const char **obj_paths, int obj_count,
                    const char *output_path,
                    const char *cc, const char *ldflags);

/* Archive object files into a static library. Returns 0 on success.
   ar_path: path to 'ar' tool (or NULL for auto-detect). */
int link_static_lib(const char **obj_paths, int obj_count,
                    const char *output_path, const char *ar_path);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_BUILD_LINK_ */
