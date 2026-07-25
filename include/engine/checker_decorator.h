#ifndef _H_CUBEC_ENGINE_CHECKER_DECORATOR_
#define _H_CUBEC_ENGINE_CHECKER_DECORATOR_
#include "engine/context.h"
#include "core/node.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  DECORATOR_TARGET_VAR,
  DECORATOR_TARGET_FUNC,
  DECORATOR_TARGET_TYPE,
} decorator_target_t;

/**
 * @brief Evaluate decorators on a declaration.
 *
 * For each decorator in the vec:
 * - [[myDec]]          → call myDec(decorated_item)
 * - [[myDec(arg)]]     → evaluate myDec(arg) → must return function F,
 *                        then call F(decorated_item)  (factory pattern)
 *
 * @param ctx       Checker context
 * @param decorators vec of cubec_decorator_t nodes
 * @param target    What kind of declaration is decorated
 * @param name      Name of the decorated symbol
 * @param ast_node  AST node of the decorated declaration
 */
void context_evaluate_decorators(context_t ctx, vec_t decorators,
                                  decorator_target_t target,
                                  const char *name, node_t ast_node);

#ifdef __cplusplus
}
#endif
#endif
