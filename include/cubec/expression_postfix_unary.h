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
 * @brief Postfix unary expression for deref, addr, try, and assert.
 *        - <value>.?  (postfix try/unwrap, e.g. result.?) - kind=TRY
 *        - <value>.!  (postfix assert/panic, e.g. result.!) - kind=ASSERT
 *        - <value>.*  (postfix dereference, e.g. ptr.*) - kind=DEREF
 *        - <value>.&  (postfix address-of, e.g. obj.&) - kind=ADDR
 *
 *        This reuses cubec_expression_binary_t struct with left=NULL
 *        and opt containing ".?", ".!", ".*" or ".&".
 */
typedef cubec_expression_binary_t cubec_expression_postfix_unary_t;

extern type_t g_cubec_expression_postfix_unary_type;

struct _cubec_expression_postfix_unary_init_t {
  location_t location;
  node_t parent;
  node_t host;
  string_t opt;
  cubec_node_kind_t kind;
};
typedef struct _cubec_expression_postfix_unary_init_t
    cubec_expression_postfix_unary_init_t;

/**
 * @brief Try to parse postfix unary expression: <value>.? or <value>.! or <value>.* or <value>.&
 *        - .?  is try operator (two tokens: . + ?)
 *        - .!  is assert/panic operator (two tokens: . + !)
 *        - .*  is deref (two tokens)
 *        - .&  is addr (two tokens)
 * @param host The already-parsed left operand (the value before the operator)
 * @return A new cubec_expression_postfix_unary_t node, or NULL if
 *         the current token is not a valid postfix operator.
 */
node_t read_expression_postfix_unary(allocator_t allocator, vec_t tokens,
                                     size_t *position, const char *filename,
                                     node_t host);

#ifdef __cplusplus
}
#endif
#endif
