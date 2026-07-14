#include "engine/symbol.h"
#include "core/allocator.h"
#include <string.h>

static void _symbol_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  struct symbol *sym = (struct symbol *)self;
  const char *name = (const char *)arg;
  memset(sym, 0, sizeof(struct symbol));
  sym->name = name;
  sym->state = SYMBOL_TDZ;
}

static void _symbol_dispose(void *self, allocator_t allocator) {
  (void)self;
  (void)allocator;
  /* symbol doesn't own name or type pointers */
}

type_t g_symbol_type = {
    .size = sizeof(struct symbol),
    .name = "cubec.engine.symbol",
    .init = (type_init_fn_t)_symbol_init,
    .dispose = (type_dispose_fn_t)_symbol_dispose,
};

struct symbol *symbol_create(allocator_t allocator, const char *name,
                             enum symbol_kind kind, location_t location) {
  struct symbol *sym =
      (struct symbol *)allocator_create(allocator, &g_symbol_type, (void *)name);
  sym->kind = kind;
  sym->location = location;
  return sym;
}
