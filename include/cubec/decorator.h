#ifndef _H_CUBEC_CUBEC_DECORATOR_
#define _H_CUBEC_CUBEC_DECORATOR_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for decorator (C++11 attribute style).
 *
 * Syntax:
 *   [[<expression>]]
 *
 * The expression inside [[...]] is a compile-time expression.
 * It can be a simple identifier or a function call with arguments.
 *
 * Examples:
 *   [[inline]]
 *   [[deprecated("use new_api instead")]]
 *   [[export]]
 */
struct _cubec_decorator_t;
struct _cubec_decorator_t {
  struct _node_t super;
  node_t expression;   /**< Compile-time expression (identifier or call) */
};
typedef struct _cubec_decorator_t *cubec_decorator_t;

extern type_t g_cubec_decorator_type;

struct _cubec_decorator_init_t {
  location_t location;
  node_t parent;
  node_t expression;
};
typedef struct _cubec_decorator_init_t cubec_decorator_init_t;

/**
 * @brief Try to parse a decorator: [[expression]]
 */
node_t read_decorator(allocator_t allocator, vec_t tokens,
                       size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
