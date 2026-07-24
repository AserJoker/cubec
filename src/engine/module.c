/**
 * @file module.c
 * @brief Module system: path resolution, file loading, module entry management.
 */

#include "engine/module.h"
#include "core/strmap.h"
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

/* ===== extended import resolution ===== */

#ifdef _WIN32
#include <sys/stat.h>
#define STAT_FUNC _stat
typedef struct _stat stat_t;
#else
#include <sys/stat.h>
#define STAT_FUNC stat
typedef struct stat stat_t;
#endif

/**
 * @brief Check if a path is a directory.
 */
static bool _is_directory(const char *path) {
  if (!path) return false;
  stat_t st;
  if (STAT_FUNC(path, &st) != 0) return false;
  return (st.st_mode & S_IFDIR) != 0;
}

/**
 * @brief Resolve a directory path to index.cubec if it's a directory.
 * Returns a new malloc'd string. If path is not a directory, returns a copy.
 */
static char *_resolve_directory_index(const char *path) {
  if (!path) return NULL;

  /* Already a .cubec file — no directory resolution needed */
  size_t plen = strlen(path);
  if (plen >= 7 && strcmp(path + plen - 7, ".cubec") == 0) {
    return strdup(path);
  }

  /* Check if it's a directory */
  if (_is_directory(path)) {
    size_t len = strlen(path);
    char *with_index = (char *)malloc(len + 13); /* /index.cubec */
    if (!with_index) return strdup(path);
    memcpy(with_index, path, len);
    memcpy(with_index + len, "/index.cubec", 13);
    return with_index;
  }

  /* Not a directory — append .cubec if not already present */
  const char *ext = ".cubec";
  size_t ext_len = strlen(ext);
  bool has_ext = (plen >= ext_len && strcmp(path + plen - ext_len, ext) == 0);
  if (has_ext) {
    return strdup(path);
  }
  char *with_ext = (char *)malloc(plen + ext_len + 1);
  if (!with_ext) return strdup(path);
  memcpy(with_ext, path, plen);
  memcpy(with_ext + plen, ext, ext_len);
  with_ext[plen + ext_len] = '\0';
  return with_ext;
}

/**
 * @brief Extract the first path segment (dependency name).
 * Returns a malloc'd string, or NULL.
 * E.g. "collections/vec" → "collections"
 */
static char *_extract_dep_name(const char *import_path) {
  if (!import_path) return NULL;
  const char *sep = strchr(import_path, '/');
#ifdef _WIN32
  const char *bs = strchr(import_path, '\\');
  if (bs && (!sep || bs < sep)) sep = bs;
#endif
  if (!sep) return strdup(import_path);
  size_t len = (size_t)(sep - import_path);
  char *name = (char *)malloc(len + 1);
  if (!name) return NULL;
  memcpy(name, import_path, len);
  name[len] = '\0';
  return name;
}

char *module_resolve_import(const char *import_path,
                            const char *current_file,
                            const char *cubec_home,
                            const char *project_root,
                            strmap_t manifest_deps,
                            bool *is_ghost) {
  if (!import_path) return NULL;
  if (is_ghost) *is_ghost = false;

  /* 1. Relative path → delegate to existing module_resolve_path */
  if (import_path[0] == '.' &&
      (import_path[1] == '/' || import_path[1] == '\\')) {
    char *resolved = module_resolve_path(import_path, current_file);
    if (resolved) {
      char *with_index = _resolve_directory_index(resolved);
      free(resolved);
      return with_index;
    }
    return NULL;
  }
  if (import_path[0] == '.' && import_path[1] == '.' &&
      (import_path[2] == '/' || import_path[2] == '\\')) {
    char *resolved = module_resolve_path(import_path, current_file);
    if (resolved) {
      char *with_index = _resolve_directory_index(resolved);
      free(resolved);
      return with_index;
    }
    return NULL;
  }

  /* Non-relative path: determine base directory */
  const char *home = cubec_home ? cubec_home : ".";
  char *dep_name = _extract_dep_name(import_path);

  /* 2. std/ prefix → ${cubec_home}/library/std/... */
  bool is_std = (dep_name && strcmp(dep_name, "std") == 0);

  /* 3. Check ghost dependency */
  if (!is_std && manifest_deps && dep_name) {
    if (!strmap_find(manifest_deps, dep_name)) {
      if (is_ghost) *is_ghost = true;
    }
  }

  /* 4. Build resolved path */
  const char *base = NULL;
  if (is_std) {
    /* std → ${cubec_home}/library/std/... */
    base = home;
  } else if (project_root && manifest_deps && dep_name &&
             strmap_find(manifest_deps, dep_name)) {
    /* Declared project dependency → ${project_root}/library/<dep>/... */
    base = project_root;
  } else {
    /* Global dependency or no manifest → ${cubec_home}/library/<dep>/... */
    base = home;
  }

  /* Build: base + /library/ + import_path */
  size_t base_len = strlen(base);
  const char *suffix = "/library/";
  size_t suffix_len = strlen(suffix);
  size_t import_len = strlen(import_path);
  size_t total = base_len + suffix_len + import_len;
  char *full = (char *)malloc(total + 1);
  if (!full) { free(dep_name); return NULL; }
  memcpy(full, base, base_len);
  memcpy(full + base_len, suffix, suffix_len);
  memcpy(full + base_len + suffix_len, import_path, import_len);
  full[total] = '\0';

  /* Simplify path */
  char *simplified = _path_simplify(full);
  free(full);

  /* Resolve directory → index.cubec or append .cubec */
  char *final = _resolve_directory_index(simplified);
  free(simplified);
  free(dep_name);
  return final;
}
