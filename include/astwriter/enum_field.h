#ifndef _H_CUBEC_ENUM_FIELD_
#define _H_CUBEC_ENUM_FIELD_
#include "ast/enum_field.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_write_ast_enum_field(cubec_allocator_t allocator,
                                         cubec_ast_enum_field_t self);
#ifdef __cplusplus
}
#endif
#endif