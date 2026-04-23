#include "engine/interrupt.h"
#include "core/allocator.h"
#include "core/position.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
void interrupt_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_t interrupt =
      create_type(allocator, TYPE_KIND_INTERRUPT, sizeof(value_t),
                  sizeof(value_t), "interrupt", "interrupt", NULL, NULL);
  context_store_type(ctx, interrupt);
}
value_t create_interrupt(context_t ctx, value_t value) {
  type_t type = context_load_type(ctx, "interrupt");
  allocator_t allocator = context_get_allocator(ctx);
  return create_value(allocator, type, false, &value, true);
}