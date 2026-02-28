#include "engine/variable.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/list.h"
#include "core/map.h"
#include "engine/value.h"
static void cubec_variable_free_value(cubec_value_t value,
                                      cubec_allocator_t allocator) {
  if (value->autofree) {
    switch (value->kind) {
    case CUBEC_VALUE_TYPE_ERROR: {
      cubec_error_data_t data = value->data;
      cubec_variable_free_value(data->error, allocator);
    } break;
    case CUBEC_VALUE_TYPE_ARRAY: {
      cubec_array_data_t data = value->data;
      for (size_t idx = 0; idx < cubec_array_get_size(data->value); idx++) {
        cubec_value_t item = cubec_array_get_index(data->value, idx);
        cubec_variable_free_value(item, allocator);
      }
    } break;
    case CUBEC_VALUE_TYPE_STRUCT: {
      cubec_struct_data_t data = value->data;
      cubec_list_node_t it = cubec_map_get_first(data->fields);
      while (it != cubec_map_get_end(data->fields)) {
        cubec_value_t item = cubec_map_node_get_value(it);
        cubec_variable_free_value(item, allocator);
        it = cubec_map_node_get_next(it);
      }
    } break;
    default:
      break;
    }
    cubec_allocator_free(allocator, value);
  }
}
static void cubec_variable_dispose(cubec_variable_t self,
                                   cubec_allocator_t allocator) {
  cubec_variable_free_value(self->value, allocator);
}
cubec_variable_t cubec_create_varaible(cubec_allocator_t allocator,
                                       cubec_value_t value, bool mutable) {
  cubec_variable_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_variable_t),
                            (cubec_dispose_fn_t)cubec_variable_dispose);
  self->mutable = mutable;
  self->value = value;
  return self;
}