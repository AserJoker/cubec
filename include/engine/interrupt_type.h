#ifndef _H_CUBEC_ENGINE_INTERRUPT_TYPE_
#define _H_CUBEC_ENGINE_INTERRUPT_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Interrupt kind — indicates the source of a function interruption.
 *  RETURN: return statement (return expr)
 *  Future: YIELD (coroutine), BREAK/CONTINUE (loops) */
typedef enum interrupt_kind_t {
  INTERRUPT_KIND_RETURN = 0,
} interrupt_kind_t;

/** @brief Payload for interrupt values.
 *  value is a borrowed reference — the interrupt does not own the value's
 *  lifecycle. During propagation, the value remains alive in its original
 *  scope. At the function boundary, the value is cloned into the caller's
 *  scope. */
struct interrupt_data_t {
  interrupt_kind_t kind;
  value_t value; /* borrowed reference */
};

/** @brief Get the "interrupt" type_t (static singleton). */
type_t type_get_interrupt_type(allocator_t allocator);

/** @brief Create an interrupt value with the given kind and borrowed value.
 *  value.data = interrupt_data_t* (owned struct, borrowed value ref).
 *  Added to current_scope->values. */
value_t create_interrupt_value(vm_t vm, interrupt_kind_t kind, value_t value);

/** @brief Read interrupt kind from an interrupt value. */
interrupt_kind_t interrupt_get_kind(value_t self);

/** @brief Read the borrowed value from an interrupt value. */
value_t interrupt_get_value(value_t self);

#ifdef __cplusplus
}
#endif
#endif
