#ifndef _H_CUBEC_ASTWRITER_INITIALIZE_LIST_
#define _H_CUBEC_ASTWRITER_INITIALIZE_LIST_
#include "ast/initialize_list.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_write_ast_initialize_list(cubec_allocator_t allocator,
                                              cubec_ast_initialize_list_t self);
#ifdef __cplusplus
}
#endif
#endif