#ifndef _H_CUBEC_CUBEC_EXPRESSION_POSTFIX_UNARY_
#define _H_CUBEC_CUBEC_EXPRESSION_POSTFIX_UNARY_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Postfix unary expression for deref and addr.
 *        - <value>.+  (postfix dereference, e.g. ptr.*)
 *        - <value>.&  (postfix address-of, e.g. &obj)
 *
 *        This reuses cubec_expression_binary_t struct with left=NULL
 *        and opt containing ".*" or ".&".
 */
typedef cubec_expression_binary_t cubec_expression_postfix_unary_t;

extern type_t g_cubec_expression_postfix_unary_type;

struct _cubec_expression_postfix_unary_init_t {
  location_t location;
  node_t parent;
  node_t host;
  string_t opt;
};
typedef struct _cubec_expression_postfix_unary_init_t
    cubec_expression_postfix_unary_init_t;

/**
 * @brief Try to parse a postfix unary expression: <value>.+ or <value>.&+
 *        where .+ is deref (.*) and .& is addr (.&).
 * @param host The already-parsed left operand (the value before the operator)
 * @return A new cubec_expression_postfix_unary_t node, or NULL if
 *         the current token is not .+ or .&.
 */
node_t read_expression_postfix_unary(allocator_t allocator, vec_t tokens,
                                     size_t *position, const char *filename,
                                     node_t host);

#ifdef __cplusplus
}
#endif
#endif
