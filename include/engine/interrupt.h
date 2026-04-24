#ifndef _H_ENGINE_INTERRUPT_
#define _H_ENGINE_INTERRUPT_
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
void interrupt_init(context_t ctx);
value_t create_interrupt(context_t ctx, value_t value);
value_t interrupt_get_value(value_t self);
#ifdef __cplusplus
}
#endif
#endif