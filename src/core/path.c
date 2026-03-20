#include "core/path.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/string.h"
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif
#include <wchar.h>
#ifdef _WIN32
#define DEFAULT_PATH_SPLITER '\\'
#else
#define DEFAULT_PATH_SPLITER '/'
#endif

struct _cubec_path_t {
  cubec_list_t parts;
};

bool cubec_path_append(cubec_path_t path, cubec_allocator_t allocator,
                       const char *part) {
  if (strcmp(part, ".") == 0) {
    return false;
  }
  if (strcmp(part, "..") == 0) {
    if (cubec_list_get_size(path->parts) == 0) {
      cubec_list_append(path->parts, 
                        cubec_create_cstring(allocator, part));
      return true;
    }
    char *last = cubec_list_node_get(cubec_list_get_last(path->parts));
    if (strcmp(last, "..") == 0) {
      cubec_list_append(path->parts, 
                        cubec_create_cstring(allocator, part));
      return true;
    }
    cubec_list_erase(path->parts, cubec_list_get_last(path->parts));
    return false;
  }
  cubec_list_append(path->parts,
                    cubec_create_cstring(allocator, part));
  return true;
}

static void cubec_path_dispose(cubec_path_t self, cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->parts);
}

cubec_path_t cubec_create_path(cubec_allocator_t allocator,
                               const char *source) {
  cubec_path_t path =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_path_t),
                            (cubec_dispose_fn_t)cubec_path_dispose);
  cubec_list_initialize_t initialze = {true};
  path->parts = cubec_create_list(allocator, &initialze);
  if (source) {
    char *part = NULL;
    size_t offset = 0;
    size_t len = strlen(source) + 1;
    if (*source == '/') {
      cubec_list_append(path->parts, 
                        cubec_create_cstring(allocator, "/"));
      source++;
    }
    if (*source) {
      part = cubec_allocator_alloc(allocator, sizeof(char) * len, NULL);
      while (*source) {
        if (*source == '/' || *source == '\\') {
          part[offset] = 0;
          cubec_path_append(path, allocator, part);
          offset = 0;
        } else {
          part[offset] = *source;
          offset++;
        }
        source++;
      }
      part[offset] = 0;
      if (offset > 0) {
        cubec_list_append(path->parts,  part);
      } else {
        cubec_allocator_free(allocator, part);
      }
    }
  }
  return path;
}

cubec_path_t cubec_path_concat(cubec_path_t current,
                               cubec_allocator_t allocator,
                               cubec_path_t another) {
  if (cubec_list_get_size(another->parts)) {
    char *first = cubec_list_node_get(cubec_list_get_first(another->parts));
    if (strcmp(first, "/") == 0) {
      return cubec_path_clone(another, allocator);
    }
    if (strlen(first) > 1 && first[1] == ':') {
      return cubec_path_clone(another, allocator);
    }
  }
  current = cubec_path_clone(current, allocator);
  for (cubec_list_node_t it = cubec_list_get_first(another->parts);
       it != cubec_list_get_end(another->parts);
       it = cubec_list_node_next(it)) {
    char *part = cubec_list_node_get(it);
    part = cubec_create_cstring(allocator, part);
    cubec_path_append(current, allocator, part);
    cubec_allocator_free(allocator, part);
  }
  return current;
}
cubec_path_t cubec_path_clone(cubec_path_t current,
                              cubec_allocator_t allocator) {
  cubec_path_t path = cubec_create_path(allocator, NULL);
  for (cubec_list_node_t it = cubec_list_get_first(current->parts);
       it != cubec_list_get_end(current->parts);
       it = cubec_list_node_next(it)) {
    cubec_list_append(path->parts,
                      cubec_create_cstring(allocator, cubec_list_node_get(it)));
  }
  return path;
}

cubec_path_t cubec_path_current(cubec_allocator_t allocator) {
  char *source = getcwd(NULL, 0);
  if (!source) {
    return NULL;
  }
  cubec_path_t path = cubec_create_path(allocator, source);
  free(source);
  return path;
}

cubec_path_t cubec_path_absolute(cubec_path_t current,
                                 cubec_allocator_t allocator) {
  cubec_path_t cwd = cubec_path_current(allocator);
  cubec_path_t result = cubec_path_concat(cwd, allocator, current);
  cubec_allocator_free(allocator, cwd);
  return result;
}

cubec_path_t cubec_path_parent(cubec_path_t current,
                               cubec_allocator_t allocator) {
  if (cubec_list_get_size(current->parts) < 1) {
    return NULL;
  }
  cubec_path_t result = cubec_create_path(allocator, NULL);
  for (cubec_list_node_t it = cubec_list_get_first(current->parts);
       it != cubec_list_get_last(current->parts);
       it = cubec_list_node_next(it)) {
    char *part = cubec_list_node_get(it);
    cubec_list_append(result->parts,
                      cubec_create_cstring(allocator, part));
  }
  return result;
}
const char *cubec_path_filename(cubec_path_t current) {
  if (cubec_list_get_size(current->parts)) {
    return cubec_list_node_get(cubec_list_get_last(current->parts));
  }
  return NULL;
}
const char *cubec_path_extname(cubec_path_t current) {
  const char *filename = cubec_path_filename(current);
  if (filename) {
    size_t idx = strlen(filename) - 1;
    while (idx != 0) {
      if (filename[idx] == '.') {
        return &filename[idx];
      }
      idx--;
    }
  }
  return NULL;
}

char *cubec_path_to_string(cubec_path_t path, cubec_allocator_t allocator) {
  size_t len = 0;
  for (cubec_list_node_t it = cubec_list_get_first(path->parts);
       it != cubec_list_get_end(path->parts); it = cubec_list_node_next(it)) {
    const char *part = cubec_list_node_get(it);
    len += strlen(part);
    len += 1;
  }
  char *result = cubec_allocator_alloc(allocator, len, NULL);
  size_t offset = 0;
  for (cubec_list_node_t it = cubec_list_get_first(path->parts);
       it != cubec_list_get_end(path->parts); it = cubec_list_node_next(it)) {
    if (it != cubec_list_get_first(path->parts)) {
      result[offset++] = DEFAULT_PATH_SPLITER;
    }
    const char *part = cubec_list_node_get(it);
    strcpy(&result[offset], part);
    offset += strlen(part);
  }
  result[offset] = 0;
  return result;
}