#include "engine/runner.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "engine/void_type.h"
#include "cubec/program.h"
#include "cubec/node.h"
#include "core/vec.h"

value_t run_program(vm_t vm, node_t node) {
  (void)vm;
  (void)node;
  /* TODO: iterate program->statements, run each statement */
  return create_void_value(vm);
}
