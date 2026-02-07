#ifndef _H_CUBEC_ASTWRITER_IMPORT_DECLARATOR_
#define _H_CUBEC_ASTWRITER_IMPORT_DECLARATOR_
#include "ast/import_declarator.h"
#include "core/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t
cubec_write_ast_import_declarator(cubec_allocator_t allocator,
                                  cubec_ast_import_declarator declarator);
#ifdef __cplusplus
}
#endif
#endif