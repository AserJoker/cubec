#include "engine/runner.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "engine/interrupt_type.h"
#include "engine/exception_type.h"
#include "engine/void_type.h"
#include "cubec/program.h"
#include "cubec/node.h"
#include "core/vec.h"

value_t run_program(vm_t vm, node_t node) {
  (void)vm;
  (void)node;
  /* TODO: iterate program->statements, run each statement */
  return vm_create_value(vm, (type_t)value_get_data(vm_get_void_type(vm)), NULL,
                         NULL);
}
