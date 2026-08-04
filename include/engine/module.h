#ifndef _H_CUBEC_ENGINE_MODULE_
#define _H_CUBEC_ENGINE_MODULE_

#include "core/allocator.h"
#include "core/node.h"
#include "core/strmap.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _module_t {
  allocator_t allocator;
  const char *filename;  /* owned copy (strdup) */
  char *source;          /* owned source text */
  vec_t tokens;          /* owned token list (auto-dispose vec) */
  node_t program;        /* owned AST root node */
};

typedef struct _module_t *module_t;

/**
 * @brief Create a module with pre-compiled tokens and AST.
 *        Does NOT perform tokenization or parsing — caller provides results.
 * @param allocator  Allocator for all allocations
 * @param filename   Source file path (copied via strdup)
 * @param source     Source text (ownership taken)
 * @param tokens     Token list (ownership taken, must be auto-dispose vec)
 * @param program    AST root node (ownership taken)
 */
module_t module_create(allocator_t allocator, const char *filename,
                       const char *source, vec_t tokens, node_t program);

/** @brief Dispose module and all owned resources. */
void module_dispose(module_t module);

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_ENGINE_MODULE_ */
