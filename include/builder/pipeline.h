#ifndef _H_CUBEC_BUILD_PIPELINE_
#define _H_CUBEC_BUILD_PIPELINE_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char *obj_path;   /* "build/m3a7_main.o" */
  const char *src_path;   /* "build/main.c" — source for incremental check */
} build_artifact_t;

typedef struct {
  const char *build_dir;        /* "build" */
  const char *entry_name;       /* output filename without extension */
  bool        build_library;    /* true = static lib, false = executable */
  build_artifact_t *artifacts;
  int artifact_count;
  int artifact_capacity;
} pipeline_t;

/* Create a pipeline context. entry_name is the base output name. */
pipeline_t pipeline_create(const char *build_dir, const char *entry_name,
                            bool build_library);

/* Add a generated .c file to the pipeline and compile it to .o.
   Uses env module for CC/CFLAGS. Returns 0 on success. */
int pipeline_compile(pipeline_t *pipe, const char *src_path);

/* Link all compiled artifacts into the final output. Returns 0 on success. */
int pipeline_link(pipeline_t *pipe);

/* Free pipeline resources. */
void pipeline_dispose(pipeline_t *pipe);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_BUILD_PIPELINE_ */
