#ifndef _H_CUBEC_CUBEC_EXPRESSION_NAMESPACE_ACCESS_
#define _H_CUBEC_CUBEC_EXPRESSION_NAMESPACE_ACCESS_
#include "core/allocator.h"
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "cubec/literal_identifier.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_namespace_access_t;
struct _cubec_expression_namespace_access_t {
  struct _cubec_expression_t super;
  node_t host;                          /**< The left-hand expression */
  cubec_literal_identifier_t field;     /**< The namespace/type name (identifier literal) */
};
typedef struct _cubec_expression_namespace_access_t
    *cubec_expression_namespace_access_t;

extern type_t g_cubec_expression_namespace_access_type;

struct _cubec_expression_namespace_access_init_t {
  location_t location;
  node_t parent;
  node_t host;
  cubec_literal_identifier_t field;
};
typedef struct _cubec_expression_namespace_access_init_t
    cubec_expression_namespace_access_init_t;

/**
 * @brief Try to parse a namespace/type-member access expression: \c ::identifier
 *
 * Used for namespace navigation (std::vec::Vec) and type member access
 * (Vec::create, File::open). The :: operator is the type-level accessor,
 * while . is reserved for instance member access (obj.field).
 *
 * @param host  The already-parsed left-hand expression.
 * @return A new cubec_expression_namespace_access_t node, or NULL if the
 *         current token is not \c "::".
 */
node_t read_expression_namespace_access(context_t ctx, vec_t tokens,
                                        size_t *position, const char *filename,
                                        node_t host);

#ifdef __cplusplus
}
#endif
#endif
