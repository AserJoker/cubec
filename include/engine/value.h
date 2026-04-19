#ifndef _H_ENGINE_VALUE_
#define _H_ENGINE_VALUE_
#include "core/allocator.h"
#include "type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _value_t *value_t;
value_t create_value(allocator_t allocator, type_t type, bool mutable,
                     void *data, bool comptime);
bool value_is_mutable(value_t value);
bool value_is_comptime(value_t value);
void *value_get_data(value_t value);
type_t value_get_type(value_t value);
#ifdef __cplusplus
}
#endif
#endif