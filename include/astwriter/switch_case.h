#ifndef _H_CUBEC_ASTWRITER_SWITCH_CASE_
#define _H_CUBEC_ASTWRITER_SWITCH_CASE_
#include "ast/switch_case.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_write_ast_switch_case(cubec_allocator_t allocator,
                                          cubec_ast_switch_case_t cas);
#ifdef __cplusplus
}
#endif
#endif