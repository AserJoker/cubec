/**
 * @file manifest.c
 * @brief Manifest.json parsing for the Cubec module system.
 */

#include "engine/manifest.h"
#include "engine/module.h"
#include <cJSON.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define ACCESS _access
#else
#include <unistd.h>
#define ACCESS access
#endif

/* ===== helpers ===== */

static char *_path_dirname(const char *path) {
  if (!path) return strdup(".");
  const char *last_sep = strrchr(path, '/');
#ifdef _WIN32
  const char *last_bs = strrchr(path, '\\');
  if (last_bs && (!last_sep || last_bs > last_sep)) last_sep = last_bs;
#endif
  if (!last_sep) return strdup(".");
  if (last_sep == path) return strdup("/");
  size_t len = (size_t)(last_sep - path);
  char *dir = (char *)malloc(len + 1);
  if (!dir) return strdup(".");
  memcpy(dir, path, len);
  dir[len] = '\0';
  return dir;
}

static bool _file_exists(const char *path) {
  return path && ACCESS(path, 0) == 0;
}

static char *_path_join(const char *a, const char *b) {
  size_t alen = strlen(a);
  size_t blen = strlen(b);
  char *result = (char *)malloc(alen + 1 + blen + 1);
  if (!result) return NULL;
  memcpy(result, a, alen);
  result[alen] = '/';
  memcpy(result + alen + 1, b, blen);
  result[alen + 1 + blen] = '\0';
  return result;
}

/* ===== public API ===== */

int manifest_parse(const char *dir, char **out_name, char ***out_dep_names) {
  if (!dir) return -1;

  char *path = _path_join(dir, "manifest.json");
  if (!path) return -1;

  if (!_file_exists(path)) {
    free(path);
    return -1;
  }

  /* Read the file */
  size_t len;
  char *content = module_read_file(path, &len);
  free(path);
  if (!content) return -1;

  /* Parse JSON */
  cJSON *root = cJSON_Parse(content);
  free(content);
  if (!root) return -1;

  /* Extract name */
  if (out_name) {
    cJSON *name_obj = cJSON_GetObjectItemCaseSensitive(root, "name");
    if (name_obj && cJSON_IsString(name_obj) && name_obj->valuestring) {
      *out_name = strdup(name_obj->valuestring);
    } else {
      *out_name = NULL;
    }
  }

  /* Extract deps keys */
  if (out_dep_names) {
    *out_dep_names = NULL;
    cJSON *deps_obj = cJSON_GetObjectItemCaseSensitive(root, "deps");
    if (deps_obj && cJSON_IsObject(deps_obj)) {
      /* Count deps */
      int count = 0;
      cJSON *item = NULL;
      cJSON_ArrayForEach(item, deps_obj) { count++; }

      if (count > 0) {
        char **names = (char **)malloc((size_t)(count + 1) * sizeof(char *));
        if (names) {
          int i = 0;
          cJSON_ArrayForEach(item, deps_obj) {
            if (item->string) {
              names[i++] = strdup(item->string);
            }
          }
          names[i] = NULL;
          *out_dep_names = names;
        }
      }
    }
  }

  cJSON_Delete(root);
  return 0;
}

void manifest_free_dep_names(char **dep_names) {
  if (!dep_names) return;
  for (int i = 0; dep_names[i]; i++) {
    free(dep_names[i]);
  }
  free(dep_names);
}

char *manifest_find_root(const char *file_path) {
  if (!file_path) return NULL;

  char *dir = _path_dirname(file_path);
  if (!dir) return NULL;

  /* Walk up the directory tree looking for manifest.json */
  for (int depth = 0; depth < 32; depth++) {
    char *manifest_path = _path_join(dir, "manifest.json");
    if (!manifest_path) { free(dir); return NULL; }

    if (_file_exists(manifest_path)) {
      free(manifest_path);
      return dir;
    }
    free(manifest_path);

    /* Go up one level */
    char *parent = _path_dirname(dir);

    /* If parent == dir, we've reached the root */
    if (strcmp(parent, dir) == 0) {
      free(parent);
      free(dir);
      return NULL;
    }
    free(dir);
    dir = parent;
  }

  free(dir);
  return NULL;
}
