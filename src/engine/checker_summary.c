#include "engine/checker_summary.h"
#include "engine/scope.h"
#include "engine/semantic_type.h"
#include <string.h>

/**
 * @brief Post-check analysis: scan global scope and compute compilation summary.
 *
 * Counts all non-generic types/functions/variables, detects dead code
 * (use_count == 0), and tallies generic instantiations.
 */
void context_compute_summary(context_t ctx) {
  if (!ctx || !ctx->global_scope) return;

  memset(&ctx->summary, 0, sizeof(ctx->summary));

  vec_t symbols = scope_get_symbols(ctx->global_scope);
  if (!symbols) return;

  size_t count = vec_get_size(symbols);
  for (size_t i = 0; i < count; i++) {
    struct symbol *sym = (struct symbol *)vec_get(symbols, i);
    if (!sym) continue;
    if (sym->is_builtin) continue;
    if (sym->kind == SYMBOL_MODULE) continue; /* imports are not code entities */

    switch (sym->kind) {

    case SYMBOL_TYPE: {
      /* Generic type template: check instantiations */
      if (sym->type.generic_params) {
        if (sym->type.instances) {
          ctx->summary.generic_type_instantiations +=
              (int)vec_get_size(sym->type.instances);
        } else {
          ctx->summary.dead_generic_templates++;
        }
        continue; /* don't count template as flat type */
      }
      /* Non-generic type */
      ctx->summary.total_types++;
      if (sym->use_count == 0)
        ctx->summary.dead_types++;
      break;
    }

    case SYMBOL_FUNCTION: {
      /* Generic function template: check instantiations */
      if (sym->function.generic_params) {
        if (sym->function.instances) {
          ctx->summary.generic_func_instantiations +=
              (int)vec_get_size(sym->function.instances);
        } else {
          ctx->summary.dead_generic_templates++;
        }
        continue; /* don't count template as flat function */
      }
      /* Non-generic function */
      ctx->summary.total_functions++;
      if (sym->use_count == 0)
        ctx->summary.dead_functions++;
      break;
    }

    case SYMBOL_VARIABLE: {
      ctx->summary.total_global_variables++;
      break;
    }

    default:
      break;
    }
  }

  /* Count methods across all types */
  for (size_t i = 0; i < count; i++) {
    struct symbol *sym = (struct symbol *)vec_get(symbols, i);
    if (!sym || sym->kind != SYMBOL_TYPE || !sym->type.type) continue;

    semantic_type_t t = sym->type.type;

    /* Instance methods */
    if (t->instance_methods) {
      ctx->summary.total_methods += (int)vec_get_size(t->instance_methods);
      ctx->summary.total_functions += (int)vec_get_size(t->instance_methods);
    }

    /* Static methods */
    if (t->static_methods) {
      ctx->summary.total_methods += (int)vec_get_size(t->static_methods);
      ctx->summary.total_functions += (int)vec_get_size(t->static_methods);
    }
  }
}
