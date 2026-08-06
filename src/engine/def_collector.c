#include "engine/def_collector.h"
#include "engine/def.h"
#include "engine/module.h"

/* --------------------------------------------------------------------------
 *  Public API
 *  TODO: definition collection will be implemented alongside eval system
 * -------------------------------------------------------------------------- */

void def_collector_run(context_t ctx, module_t mod) {
  if (!mod || !mod->program)
    return;
  if (mod->state < MODULE_COLLECTED)
    return;
  if (mod->state >= MODULE_RESOLVED)
    return;

  mod->state = MODULE_RESOLVING;

  /* TODO: create def_t objects for each declaration and bind to name->ref */

  mod->state = MODULE_RESOLVED;
}
