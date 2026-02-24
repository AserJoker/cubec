#include "core/value.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/map.h"
#include "core/string.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static void cubec_value_dispose(cubec_value_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->value);
}
cubec_value_t cubec_create_value(cubec_allocator_t allocator) {
  cubec_value_t value =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_value_t),
                            (cubec_dispose_fn_t)cubec_value_dispose);
  value->value = NULL;
  value->type = CUBEC_VALUE_TYPE_NULL;
  return value;
}

cubec_value_t cubec_value_set_null(cubec_value_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->value);
  self->type = CUBEC_VALUE_TYPE_NULL;
  return self;
}

cubec_value_t cubec_value_set_boolean(cubec_value_t self,
                                      cubec_allocator_t allocator, bool value) {
  if (self->type != CUBEC_VALUE_TYPE_BOOLEAN) {
    cubec_allocator_free(allocator, self->value);
    self->value = cubec_allocator_alloc(allocator, sizeof(bool), NULL);
    self->type = CUBEC_VALUE_TYPE_BOOLEAN;
  }
  *(bool *)(self->value) = value;
  return self;
}

cubec_value_t cubec_value_set_number(cubec_value_t self,
                                     cubec_allocator_t allocator,
                                     double value) {
  if (self->type != CUBEC_VALUE_TYPE_NUMBER) {
    cubec_allocator_free(allocator, self->value);
    self->value = cubec_allocator_alloc(allocator, sizeof(double), NULL);
    self->type = CUBEC_VALUE_TYPE_NUMBER;
  }
  *(double *)(self->value) = value;
  return self;
}

cubec_value_t cubec_value_set_string(cubec_value_t self,
                                     cubec_allocator_t allocator,
                                     const char *value) {
  if (self->type != CUBEC_VALUE_TYPE_STRING) {
    cubec_allocator_free(allocator, self->value);
    self->type = CUBEC_VALUE_TYPE_STRING;
  }
  size_t len = strlen(value);
  self->value = cubec_allocator_alloc(allocator, len + 1, NULL);
  memcpy(self->value, value, len);
  ((char *)self->value)[len] = 0;
  return self;
}

cubec_value_t cubec_value_set_array(cubec_value_t self,
                                    cubec_allocator_t allocator) {
  if (self->type != CUBEC_VALUE_TYPE_ARRAY) {
    cubec_allocator_free(allocator, self->value);
    self->type = CUBEC_VALUE_TYPE_ARRAY;
  }
  cubec_array_initialize_t initialize = {0, true};
  self->value = cubec_create_array(allocator, &initialize);
  return self;
}

cubec_value_t cubec_value_set_object(cubec_value_t self,
                                     cubec_allocator_t allocator) {
  if (self->type != CUBEC_VALUE_TYPE_OBJECT) {
    cubec_allocator_free(allocator, self->value);
    self->type = CUBEC_VALUE_TYPE_OBJECT;
  }
  cubec_map_initialize_t initialize = {true, true, (cubec_compare_fn_t)strcmp};
  self->value = cubec_create_map(allocator, &initialize);
  return self;
}

bool cubec_value_get_boolean(cubec_value_t self, cubec_allocator_t allocator) {
  if (self->type == CUBEC_VALUE_TYPE_BOOLEAN) {
    return *(bool *)self->value;
  }
  return false;
}

double cubec_value_get_number(cubec_value_t self, cubec_allocator_t allocator) {
  if (self->type == CUBEC_VALUE_TYPE_NUMBER) {
    return *(double *)self->value;
  }
  return 0;
}

const char *cubec_value_get_string(cubec_value_t self,
                                   cubec_allocator_t allocator) {
  if (self->type == CUBEC_VALUE_TYPE_STRING) {
    return (const char *)self->value;
  }
  return "";
}

size_t cubec_value_get_length(cubec_value_t self) {
  if (self->type == CUBEC_VALUE_TYPE_ARRAY) {
    return cubec_array_get_size((cubec_array_t)self->value);
  }
  if (self->type == CUBEC_VALUE_TYPE_OBJECT) {
    return cubec_map_get_size((cubec_map_t)self->value);
  }
  return 0;
}

cubec_value_t cubec_value_get_field(cubec_value_t self,
                                    cubec_allocator_t allocator,
                                    const char *field) {
  if (self->type == CUBEC_VALUE_TYPE_OBJECT) {
    cubec_map_t map = (cubec_map_t)self->value;
    return (cubec_value_t)cubec_map_get(map, field, NULL);
  }
  return NULL;
}

bool cubec_value_has_field(cubec_value_t self, cubec_allocator_t allocator,
                           const char *field) {
  if (self->type == CUBEC_VALUE_TYPE_OBJECT) {
    cubec_map_t map = (cubec_map_t)self->value;
    return cubec_map_has(map, field, NULL);
  }
  return false;
}

cubec_value_t cubec_value_set_field(cubec_value_t self,
                                    cubec_allocator_t allocator,
                                    const char *field, cubec_value_t value) {
  if (self->type == CUBEC_VALUE_TYPE_OBJECT) {
    size_t len = strlen(field);
    char *key = cubec_allocator_alloc(allocator, len + 1, NULL);
    memcpy(key, field, len + 1);
    cubec_map_t map = (cubec_map_t)self->value;
    cubec_map_set(map, allocator, key, value, NULL);
    return self;
  }
  return NULL;
}

cubec_value_t cubec_value_resize(cubec_value_t self,
                                 cubec_allocator_t allocator, uint32_t size) {
  if (self->type == CUBEC_VALUE_TYPE_ARRAY) {
    cubec_array_t array = (cubec_array_t)self->value;
    cubec_array_resize(array, allocator, size);
  }
  return NULL;
}

cubec_value_t cubec_value_get_index(cubec_value_t self,
                                    cubec_allocator_t allocator,
                                    uint32_t index) {
  if (self->type == CUBEC_VALUE_TYPE_ARRAY) {
    cubec_array_t array = (cubec_array_t)self->value;
    return (cubec_value_t)cubec_array_get_index(array, index);
  }
  return NULL;
}

cubec_value_t cubec_value_set_index(cubec_value_t self,
                                    cubec_allocator_t allocator, uint32_t index,
                                    cubec_value_t value) {

  if (self->type == CUBEC_VALUE_TYPE_ARRAY) {
    cubec_array_t array = (cubec_array_t)self->value;
    cubec_array_set_index(array, allocator, index, value);
    return self;
  }
  return NULL;
}

cubec_value_t cubec_value_append(cubec_value_t self,
                                 cubec_allocator_t allocator,
                                 cubec_value_t value) {

  if (self->type == CUBEC_VALUE_TYPE_ARRAY) {
    cubec_array_t array = (cubec_array_t)self->value;
    cubec_array_push(array, allocator, value);
    return self;
  }
  return NULL;
}

void cubec_value_to_json_string(cubec_value_t self, cubec_allocator_t allocator,
                                cubec_string_t str) {
  switch (self->type) {

  case CUBEC_VALUE_TYPE_NULL:
    cubec_string_concat(str, allocator, "null");
    break;
  case CUBEC_VALUE_TYPE_BOOLEAN:
    if (*(bool *)self->value) {
      cubec_string_concat(str, allocator, "true");
    } else {
      cubec_string_concat(str, allocator, "false");
    }
    break;
  case CUBEC_VALUE_TYPE_NUMBER: {
    char s[32];
    sprintf(s, "%lg", *(double *)(self->value));
    cubec_string_concat(str, allocator, s);
    break;
  }
  case CUBEC_VALUE_TYPE_STRING: {
    const char *src = (const char *)(self->value);
    char *encode_str =
        cubec_allocator_alloc(allocator, strlen(src) * 2 + 1, NULL);
    char *dst = encode_str;
    while (*src) {
      if (*src == '\"') {
        *dst++ = '\\';
        *dst++ = '\"';
      } else if (*src == '\\') {
        *dst++ = '\\';
        *dst++ = '\\';
      } else if (*src == '\n') {
        *dst++ = '\\';
        *dst++ = 'n';
      } else if (*src == '\r') {
        *dst++ = '\\';
        *dst++ = 'r';
      } else {
        *dst++ = *src;
      }
      src++;
    }
    *dst = 0;
    cubec_string_concat(str, allocator, "\"");
    cubec_string_concat(str, allocator, encode_str);
    cubec_string_concat(str, allocator, "\"");
    cubec_allocator_free(allocator, encode_str);
    break;
  }
  case CUBEC_VALUE_TYPE_ARRAY: {
    cubec_array_t array = (cubec_array_t)self->value;
    size_t len = cubec_array_get_size(array);
    cubec_string_concat(str, allocator, "[");
    for (size_t idx = 0; idx < len; idx++) {
      if (idx != 0) {
        cubec_string_concat(str, allocator, ",");
      }
      cubec_value_t item = cubec_array_get_index(array, idx);
      cubec_value_to_json_string(item, allocator, str);
    }
    cubec_string_concat(str, allocator, "]");
  } break;
  case CUBEC_VALUE_TYPE_OBJECT: {
    cubec_map_t map = (cubec_map_t)self->value;
    cubec_string_concat(str, allocator, "{");
    cubec_list_node_t it = cubec_map_get_first(map);
    size_t idx = 0;
    while (it != cubec_map_get_end(map)) {
      const char *key = cubec_map_node_get_key(it);
      cubec_value_t item = cubec_map_node_get_value(it);
      if (idx != 0) {
        cubec_string_concat(str, allocator, ",");
      }
      cubec_string_concat(str, allocator, "\"");
      cubec_string_concat(str, allocator, key);
      cubec_string_concat(str, allocator, "\":");
      cubec_value_to_json_string(item, allocator, str);
      it = cubec_map_node_get_next(it);
      idx++;
    }
    cubec_string_concat(str, allocator, "}");
  } break;
  }
}

char *cubec_value_to_json(cubec_value_t self, cubec_allocator_t allocator) {
  cubec_string_t str = cubec_create_string(allocator, NULL);
  cubec_value_to_json_string(self, allocator, str);
  size_t len = cubec_string_len(str);
  char *s = cubec_allocator_alloc(allocator, len + 1, NULL);
  memcpy(s, cubec_string_get(str), len + 1);
  cubec_allocator_free(allocator, str);
  return s;
}