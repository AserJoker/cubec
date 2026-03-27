#ifndef _H_CUBEC_EVAL_PTR_DECLARATOR_
#define _H_CUBEC_EVAL_PTR_DECLARATOR_
#include "ast/ptr_declarator.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cpluplus
extern "C" {
#endif
cubec_value_t cubec_eval_ptr_declarator(cubec_context_t ctx,
                                        cubec_ast_node_t type,
                                        const char *filename);
#ifdef __cpluplus
}
#endif
#endif