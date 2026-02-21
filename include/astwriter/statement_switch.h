#ifndef _H_CUBEC_ASTWRITER_STATEMENT_SWITCH_
#define _H_CUBEC_ASTWRITER_STATEMENT_SWITCH_
#include "ast/statement_switch.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_statement_switch(cubec_allocator_t allocator,
                                 cubec_ast_statement_switch_t statement);
#ifdef __cplusplus
}
#endif
#endif