/**
 * @file module.c
 * @brief Module system: path resolution, file loading, module entry management.
 */

#include "engine/module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#define ACCESS _access
#else
#include <unistd.h>
#define ACCESS access
#endif

/* ===== module_entry lifecycle ===== */

module_entry_t module_entry_create(const char *resolved_path) {
  module_entry_t entry = (module_entry_t)malloc(sizeof(struct module_entry));
  if (!entry) return NULL;
  memset(entry, 0, sizeof(struct module_entry));
  entry->state = MODULE_PARSING;
  if (resolved_path) {
    entry->resolved_path = strdup(resolved_path);
  }
  return entry;
}

void module_entry_dispose(module_entry_t entry) {
  if (!entry) return;
  /* No separate checker to dispose — modules are compiled in the
     same checker context. scopes/types/tokens/program are owned
     by the main checker's allocator. */
  /* Free the source buffer */
  if (entry->source) free(entry->source);
  /* Free the resolved path */
  if (entry->resolved_path) free(entry->resolved_path);
  free(entry);
}

/* ===== path resolution ===== */

/**
 * Extract the directory portion of a file path.
 * Returns a malloc'd string (caller must free).
 */
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

/**
 * Simplify a path by resolving . and .. components.
 * Returns a malloc'd string (caller must free).
 */
static char *_path_simplify(const char *path) {
  if (!path) return NULL;
  size_t len = strlen(path);
  char *result = (char *)malloc(len + 1);
  if (!result) return strdup(path);

  const char *src = path;
  char *dst = result;

  while (*src) {
    /* Skip multiple slashes */
    while (*src == '/' || *src == '\\') {
      if (dst == result || *(dst - 1) != '/') *dst++ = '/';
      src++;
      continue;
    }
    if (!*src) break;

    /* Find the next component */
    const char *start = src;
    while (*src && *src != '/' && *src != '\\') src++;
    size_t comp_len = (size_t)(src - start);

    if (comp_len == 1 && start[0] == '.') {
      /* Skip "." */
      continue;
    }
    if (comp_len == 2 && start[0] == '.' && start[1] == '.') {
      /* ".." — remove last component */
      if (dst > result + 1) {
        dst--; /* skip trailing slash */
        while (dst > result && *(dst - 1) != '/') dst--;
      }
      continue;
    }

    /* Copy component */
    memcpy(dst, start, comp_len);
    dst += comp_len;
  }

  /* Remove trailing slash (unless root) */
  if (dst > result + 1 && *(dst - 1) == '/') dst--;
  *dst = '\0';
  return result;
}

char *module_resolve_path(const char *import_path, const char *current_file) {
  if (!import_path) return NULL;

  char *base_dir = _path_dirname(current_file);
  size_t base_len = strlen(base_dir);
  size_t import_len = strlen(import_path);

  /* Build full path: base_dir + / + import_path */
  size_t full_len = base_len + 1 + import_len;
  char *full_path = (char *)malloc(full_len + 1);
  if (!full_path) { free(base_dir); return NULL; }
  memcpy(full_path, base_dir, base_len);
  full_path[base_len] = '/';
  memcpy(full_path + base_len + 1, import_path, import_len);
  full_path[full_len] = '\0';
  free(base_dir);

  /* Append .cubec extension if not already present */
  size_t fplen = strlen(full_path);
  const char *ext = ".cubec";
  size_t ext_len = strlen(ext);
  bool has_ext = (fplen >= ext_len &&
                  strcmp(full_path + fplen - ext_len, ext) == 0);
  char *with_ext;
  if (has_ext) {
    with_ext = full_path;
  } else {
    with_ext = (char *)malloc(fplen + ext_len + 1);
    if (!with_ext) { free(full_path); return NULL; }
    memcpy(with_ext, full_path, fplen);
    memcpy(with_ext + fplen, ext, ext_len);
    with_ext[fplen + ext_len] = '\0';
    free(full_path);
  }

  /* Simplify the path (resolve . and ..) */
  char *simplified = _path_simplify(with_ext);
  free(with_ext);
  return simplified;
}

/* ===== file loading ===== */

char *module_read_file(const char *path, size_t *out_len) {
  if (!path) return NULL;
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  if (len < 0) { fclose(f); return NULL; }
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc((size_t)len + 1);
  if (!buf) { fclose(f); return NULL; }
  size_t n = fread(buf, 1, (size_t)len, f);
  buf[n] = '\0';
  fclose(f);
  if (out_len) *out_len = n;
  return buf;
}
