#include "c/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/statement_declaration.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "core/map.h"
#include "core/path.h"
#include "engine/context.h"
#include "engine/module.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/statement_test.h"
#include <inttypes.h>
#ifdef _WIN32
#include <io.h>
#define access _access
#else
#include <unistd.h>
#endif

cubec_value_t cubec_c_write_program(cubec_context_t self,
                                    cubec_ast_node_t program,
                                    const char *filename,
                                    cubec_string_t *output) {
  cubec_module_t module = cubec_context_get_module(self, filename);
  cubec_module_t parent = cubec_context_set_module(self, module);
  cubec_ast_node_t list = cubec_map_get(program->children, "statements", NULL);
  cubec_array_t items = list->items;
  for (size_t idx = 0; idx < cubec_array_get_size(items); idx++) {
    cubec_ast_node_t node = cubec_array_get(items, idx);
    if (node->type == CUBEC_NODE_TYPE_STATEMENT_IMPORT) {
      cubec_ast_node_t source = cubec_map_get(node->children, "source", NULL);
      char *src = cubec_location_get_str(source->loc, self->allocator);
      cubec_module_t dep = NULL;
      dep = cubec_context_get_module(self, src);
      char *fullsrc = src;
      if (!dep) {
        cubec_path_t current =
            cubec_create_path(self->allocator, module->dirname);
        cubec_path_t next = cubec_create_path(self->allocator, src);
        cubec_path_t fp = cubec_path_concat(current, self->allocator, next);
        cubec_allocator_free(self->allocator, current);
        cubec_allocator_free(self->allocator, next);
        fullsrc = cubec_path_to_string(fp, self->allocator);
        cubec_allocator_free(self->allocator, fp);
      }
      if (access(fullsrc, 0) != 0) {
        cubec_value_t err = cubec_context_create_compile_error(
            self, node, filename,
            "Failed to load module '%s', file is not exists", src);
        cubec_allocator_free(self->allocator, src);
        cubec_allocator_free(self->allocator, fullsrc);
        return err;
      }
      cubec_value_t err = cubec_context_load_module(self, fullsrc);
      if (err->type == CUBEC_TYPE_KIND_ERROR) {
        const char *msg = *(const char **)err->data;
        err = cubec_context_create_compile_error(
            self, node, filename, "Failed to load module '%s'\ncaused by: %s",
            src, msg);
        cubec_allocator_free(self->allocator, src);
        cubec_allocator_free(self->allocator, fullsrc);
        return err;
      }
      dep = cubec_context_get_module(self, fullsrc);
      cubec_allocator_free(self->allocator, src);
      cubec_allocator_free(self->allocator, fullsrc);
      cubec_map_set(module->dependences, dep->filename, dep, NULL);
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_FUNCTION) {
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_STRUCT) {
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_ENUM) {
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_DECLARATION) {
      cubec_value_t err =
          cubec_c_write_statement_declaration(self, node, filename, output);
      if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return err;
      }
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_TEST) {
      cubec_eval_statement_test(self, node, filename);
    } else {
      return cubec_context_create_compile_error(
          self, node, filename,
          "Top statement only support "
          "import,function,struct,enum,variable declaration,test");
    }
  }
  cubec_context_set_module(self, parent);
  return self->value_undefined;
}
