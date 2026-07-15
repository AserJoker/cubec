#ifndef _H_CUBEC_AST_FACTORY_INTERNAL_
#define _H_CUBEC_AST_FACTORY_INTERNAL_
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#include "cubec/literal_identifier.h"

/**
 * Internal helpers for cubec_ast_create_* functions.
 * Not part of the public API — only for use within src/cubec/ implementation files.
 */

string_t _make_string(allocator_t alloc, const char *str);

cubec_literal_identifier_t _make_ident_node(allocator_t alloc, location_t loc,
                                             const char *name);

#endif
