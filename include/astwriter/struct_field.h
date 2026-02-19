#ifndef _H_CUBEC_ASTWRITER_STRUCT_FIELD_
#define _H_CUBEC_ASTWRITER_STRUCT_FIELD_
#include "ast/struct_field.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_write_ast_struct_field(cubec_allocator_t allocator,
                                           cubec_ast_struct_field_t self);
#ifdef __cplusplus
}
#endif
#endif