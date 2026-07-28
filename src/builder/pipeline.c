#include "builder/pipeline.h"
#include "builder/compile.h"
#include "builder/link.h"
#include "core/env.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pipeline_t pipeline_create(const char *build_dir, const char *entry_name,
                            bool build_library) {
  pipeline_t pipe;
  pipe.build_dir = build_dir;
  pipe.entry_name = entry_name;
  pipe.build_library = build_library;
  pipe.artifacts = NULL;
  pipe.artifact_count = 0;
  pipe.artifact_capacity = 0;
  return pipe;
}

int pipeline_compile(pipeline_t *pipe, const char *src_path) {
  const env_config_t *env = env_get();
  char *obj_path = NULL;
  int ret = compile_source(src_path, pipe->build_dir,
                           env->cc ? env->cc : "cc",
                           env->cflags ? env->cflags : "",
                           &obj_path);
  if (ret != 0) return ret;

  /* Grow artifacts array if needed */
  if (pipe->artifact_count >= pipe->artifact_capacity) {
    int new_cap = pipe->artifact_capacity == 0 ? 8 : pipe->artifact_capacity * 2;
    build_artifact_t *new_arr = (build_artifact_t *)realloc(
        pipe->artifacts, sizeof(build_artifact_t) * new_cap);
    if (!new_arr) { free(obj_path); return 1; }
    pipe->artifacts = new_arr;
    pipe->artifact_capacity = new_cap;
  }

  pipe->artifacts[pipe->artifact_count].obj_path = obj_path;
  pipe->artifacts[pipe->artifact_count].src_path = src_path;
  pipe->artifact_count++;
  return 0;
}

int pipeline_link(pipeline_t *pipe) {
  if (pipe->artifact_count == 0) {
    fprintf(stderr, "error: no object files to link\n");
    return 1;
  }

  const env_config_t *env = env_get();
  const char *cc = env->cc ? env->cc : "cc";
  const char *ldflags = env->ldflags ? env->ldflags : "";

  /* Platform-specific default link flags */
#ifdef _WIN32
  const char *plat_ldflags = "-Xlinker /subsystem:console";
#else
  const char *plat_ldflags = "";
#endif

  /* Combine ldflags: platform defaults + user ldflags */
  size_t combined_len = strlen(plat_ldflags) + strlen(ldflags) + 2;
  char *combined = (char *)malloc(combined_len);
  if (plat_ldflags[0] && ldflags[0])
    snprintf(combined, combined_len, "%s %s", plat_ldflags, ldflags);
  else if (plat_ldflags[0])
    snprintf(combined, combined_len, "%s", plat_ldflags);
  else
    snprintf(combined, combined_len, "%s", ldflags);

  /* Build output path */
  size_t dir_len = strlen(pipe->build_dir);
  size_t name_len = strlen(pipe->entry_name);
  char *output_path;

  if (pipe->build_library) {
    /* lib<name>.a */
    output_path = (char *)malloc(dir_len + 1 + 4 + name_len + 3);
    memcpy(output_path, pipe->build_dir, dir_len);
    output_path[dir_len] = '/';
    memcpy(output_path + dir_len + 1, "lib", 3);
    memcpy(output_path + dir_len + 4, pipe->entry_name, name_len);
    memcpy(output_path + dir_len + 4 + name_len, ".a", 3);
    output_path[dir_len + 4 + name_len + 2] = '\0';

    const char **obj_paths = (const char **)malloc(sizeof(const char *) * pipe->artifact_count);
    for (int i = 0; i < pipe->artifact_count; i++)
      obj_paths[i] = pipe->artifacts[i].obj_path;

    int ret = link_static_lib(obj_paths, pipe->artifact_count, output_path, env->ar);
    free(obj_paths);
    if (ret == 0)
      fprintf(stdout, "Generated: %s\n", output_path);
    free(output_path);
    free(combined);
    return ret;
  } else {
    /* <name>.exe on Windows, <name> elsewhere */
#ifdef _WIN32
    output_path = (char *)malloc(dir_len + 1 + name_len + 5);
    memcpy(output_path, pipe->build_dir, dir_len);
    output_path[dir_len] = '/';
    memcpy(output_path + dir_len + 1, pipe->entry_name, name_len);
    memcpy(output_path + dir_len + 1 + name_len, ".exe", 5);
#else
    output_path = (char *)malloc(dir_len + 1 + name_len + 1);
    memcpy(output_path, pipe->build_dir, dir_len);
    output_path[dir_len] = '/';
    memcpy(output_path + dir_len + 1, pipe->entry_name, name_len);
    output_path[dir_len + 1 + name_len] = '\0';
#endif

    const char **obj_paths = (const char **)malloc(sizeof(const char *) * pipe->artifact_count);
    for (int i = 0; i < pipe->artifact_count; i++)
      obj_paths[i] = pipe->artifacts[i].obj_path;

    int ret = link_executable(obj_paths, pipe->artifact_count, output_path, cc, combined);
    free(obj_paths);
    if (ret == 0)
      fprintf(stdout, "Generated: %s\n", output_path);
    free(output_path);
    free(combined);
    return ret;
  }
}

void pipeline_dispose(pipeline_t *pipe) {
  for (int i = 0; i < pipe->artifact_count; i++) {
    free((void *)pipe->artifacts[i].obj_path);
  }
  free(pipe->artifacts);
  pipe->artifacts = NULL;
  pipe->artifact_count = 0;
  pipe->artifact_capacity = 0;
}
