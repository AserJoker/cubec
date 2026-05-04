#ifndef _H_ENGINE_BUILTIN_
#define _H_ENGINE_BUILTIN_
#include "ast/node.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t builtin_error(context_t ctx, size_t argc, ast_node_t *argv);
ast_node_t builtin_typeof(context_t ctx, size_t argc, ast_node_t *argv);
ast_node_t builtin_alignof(context_t ctx, size_t argc, ast_node_t *argv);
ast_node_t builtin_sizeof(context_t ctx, size_t argc, ast_node_t *argv);
ast_node_t builtin_print(context_t ctx, size_t argc, ast_node_t *argv);
#ifdef __cplusplus
}
#endif
#endif