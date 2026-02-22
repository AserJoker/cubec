#ifndef _H_CUBEC_ASTWRITER_STATEMENT_FOREACH_
#define _H_CUBEC_ASTWRITER_STATEMENT_FOREACH_
#include "ast/statement_foreach.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_statement_foreach(cubec_allocator_t allocator,
                                  cubec_ast_statement_foreach_t statement);
#ifdef __cplusplus
}
#endif
#endif