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

struct _path_t {
  list_t parts;
};

bool path_append(path_t path, allocator_t allocator, const char *part) {
  if (strcmp(part, ".") == 0) {
    return false;
  }
  if (strcmp(part, "..") == 0) {
    if (list_get_size(path->parts) == 0) {
      list_append(path->parts, create_cstring(allocator, part));
      return true;
    }
    char *last = list_node_get(list_get_last(path->parts));
    if (strcmp(last, "..") == 0) {
      list_append(path->parts, create_cstring(allocator, part));
      return true;
    }
    list_erase(path->parts, list_get_last(path->parts));
    return false;
  }
  list_append(path->parts, create_cstring(allocator, part));
  return true;
}

static void path_dispose(path_t self, allocator_t allocator) {
  allocator_free(allocator, self->parts);
}

path_t create_path(allocator_t allocator, const char *source) {
  path_t path = allocator_alloc(allocator, sizeof(struct _path_t),
                                (dispose_fn_t)path_dispose);
  list_initialize_t initialze = {true};
  path->parts = create_list(allocator, &initialze);
  if (source) {
    char *part = NULL;
    size_t offset = 0;
    size_t len = strlen(source) + 1;
    if (*source == '/') {
      list_append(path->parts, create_cstring(allocator, "/"));
      source++;
    }
    if (*source) {
      part = allocator_alloc(allocator, sizeof(char) * len, NULL);
      while (*source) {
        if (*source == '/' || *source == '\\') {
          part[offset] = 0;
          path_append(path, allocator, part);
          offset = 0;
        } else {
          part[offset] = *source;
          offset++;
        }
        source++;
      }
      part[offset] = 0;
      if (offset > 0) {
        list_append(path->parts, part);
      } else {
        allocator_free(allocator, part);
      }
    }
  }
  return path;
}

path_t path_concat(path_t current, allocator_t allocator, path_t another) {
  if (list_get_size(another->parts)) {
    char *first = list_node_get(list_get_first(another->parts));
    if (strcmp(first, "/") == 0) {
      return path_clone(another, allocator);
    }
    if (strlen(first) > 1 && first[1] == ':') {
      return path_clone(another, allocator);
    }
  }
  current = path_clone(current, allocator);
  for (list_node_t it = list_get_first(another->parts);
       it != list_get_end(another->parts); it = list_node_next(it)) {
    char *part = list_node_get(it);
    part = create_cstring(allocator, part);
    path_append(current, allocator, part);
    allocator_free(allocator, part);
  }
  return current;
}
path_t path_clone(path_t current, allocator_t allocator) {
  path_t path = create_path(allocator, NULL);
  for (list_node_t it = list_get_first(current->parts);
       it != list_get_end(current->parts); it = list_node_next(it)) {
    list_append(path->parts, create_cstring(allocator, list_node_get(it)));
  }
  return path;
}

path_t path_current(allocator_t allocator) {
  char *source = getcwd(NULL, 0);
  if (!source) {
    return NULL;
  }
  path_t path = create_path(allocator, source);
  free(source);
  return path;
}

path_t path_absolute(path_t current, allocator_t allocator) {
  path_t cwd = path_current(allocator);
  path_t result = path_concat(cwd, allocator, current);
  allocator_free(allocator, cwd);
  return result;
}

path_t path_parent(path_t current, allocator_t allocator) {
  if (list_get_size(current->parts) < 1) {
    return NULL;
  }
  path_t result = create_path(allocator, NULL);
  for (list_node_t it = list_get_first(current->parts);
       it != list_get_last(current->parts); it = list_node_next(it)) {
    char *part = list_node_get(it);
    list_append(result->parts, create_cstring(allocator, part));
  }
  return result;
}
const char *path_filename(path_t current) {
  if (list_get_size(current->parts)) {
    return list_node_get(list_get_last(current->parts));
  }
  return NULL;
}
const char *path_extname(path_t current) {
  const char *filename = path_filename(current);
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

char *path_to_string(path_t path, allocator_t allocator) {
  size_t len = 0;
  for (list_node_t it = list_get_first(path->parts);
       it != list_get_end(path->parts); it = list_node_next(it)) {
    const char *part = list_node_get(it);
    len += strlen(part);
    len += 1;
  }
  char *result = allocator_alloc(allocator, len, NULL);
  size_t offset = 0;
  for (list_node_t it = list_get_first(path->parts);
       it != list_get_end(path->parts); it = list_node_next(it)) {
    if (it != list_get_first(path->parts) &&
        result[offset - 1] != DEFAULT_PATH_SPLITER) {
      result[offset++] = DEFAULT_PATH_SPLITER;
    }
    const char *part = list_node_get(it);
    strcpy(&result[offset], part);
    offset += strlen(part);
  }
  result[offset] = 0;
  return result;
}