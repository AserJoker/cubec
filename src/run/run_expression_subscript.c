#include "run/run.h"
#include "engine/vm.h"
#include "engine/exception_type.h"
#include "engine/value.h"
#include "engine/type.h"
#include "cubec/expression_subscript.h"
#include "core/vec.h"

value_t run_expression_subscript(vm_t vm, node_t node, bool shadow) {
  cubec_expression_subscript_t sub = (cubec_expression_subscript_t)node;

  value_t host = run_expression(vm, sub->host, shadow);
  if (value_is_abnormal(host)) return host;

  /* Disambiguate: generic instantiation vs subscript.
   * Generic values are TYPE_KIND_GENERIC / TYPE_KIND_GENERIC_FN — their own values,
   * not wrapped in TYPE_KIND_TYPE. value.data holds the create_instance callback. */
  type_kind_t kind = type_get_kind(value_get_type(host));
  if (kind == TYPE_KIND_GENERIC || kind == TYPE_KIND_GENERIC_FN) {
    /* Generic instantiation path — evaluate all arguments, call value_instantiate.
     * Shadow is ignored: instantiation is always concrete. */
    size_t argc = vec_get_size(sub->arguments);
    value_t argv_stack[8];
    value_t *argv = argv_stack;
    bool heap_alloc = false;
    if (argc > 8) {
      argv = (value_t *)allocator_alloc(vm_get_allocator(vm), sizeof(value_t) * argc);
      if (!argv)
        return create_exception_value(vm, "out of memory for generic arguments");
      heap_alloc = true;
    }

    for (size_t i = 0; i < argc; i++) {
      node_t arg_node = (node_t)vec_get(sub->arguments, i);
      argv[i] = run_expression(vm, arg_node, false);
      if (value_is_abnormal(argv[i])) {
        if (heap_alloc) {
          void *p = argv;
          allocator_free(vm_get_allocator(vm), &p);
        }
        return argv[i];
      }
    }

    value_t result = value_instantiate(vm, host, argc, argv);

    if (heap_alloc) {
      void *p = argv;
      allocator_free(vm_get_allocator(vm), &p);
    }
    return result;
  }

  /* Subscript path (existing logic) */
  size_t argc = vec_get_size(sub->arguments);
  if (argc != 1)
    return create_exception_value(vm,
                                  "run: subscript requires exactly one argument, got %zu",
                                  argc);

  node_t index_node = (node_t)vec_get(sub->arguments, 0);
  value_t index = run_expression(vm, index_node, shadow);
  if (value_is_abnormal(index)) return index;

  return value_get_item(vm, host, index);
}
