#include "c/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/statement_import.h"
#include "c/statement_declaration.h"
#include "c/writer.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/location.h"
#include "core/path.h"
#include "engine/context.h"
#include "engine/module.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <unistd.h>

cubec_value_t cubec_c_write_program(cubec_context_t self,
                                    cubec_ast_program_t program,
                                    const char *filename,
                                    cubec_string_t *output) {
  cubec_module_t module = cubec_context_get_module(self, filename);
  cubec_module_t parent = cubec_context_set_module(self, module);
  cubec_ast_list_node_t list = (cubec_ast_list_node_t)program->statements;
  cubec_list_t items = list->items;
  cubec_list_node_t it = cubec_list_get_first(items);
  while (it != cubec_list_get_end(items)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    if (node->type == CUBEC_NODE_TYPE_STATEMENT_IMPORT) {
      cubec_ast_statement_import_t sts = (cubec_ast_statement_import_t)node;
      char *src = cubec_location_get_str(sts->source->loc, self->allocator);
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
        cubec_value_t err = cubec_c_create_error(
            self, node, filename,
            "Failed to load module '%s', file is not exists", src);
        cubec_allocator_free(self->allocator, src);
        cubec_allocator_free(self->allocator, fullsrc);
        return err;
      }
      cubec_value_t err = cubec_context_load_module(self, fullsrc);
      if (err->type == CUBEC_TYPE_KIND_ERROR) {
        const char *msg = *(const char **)err->data;
        err = cubec_c_create_error(self, node, filename,
                                   "Failed to load module '%s'\ncaused by: %s",
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
      cubec_value_t err = cubec_c_write_statement_declaration(
          self, (cubec_ast_statement_declaration_t)node, filename, output);
      if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
        return err;
      }
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_TEST) {
    } else {
      return cubec_c_create_error(
          self, node, filename,
          "Top statement only support "
          "import,function,struct,enum,variable declaration,test");
    }
    it = cubec_list_node_next(it);
  }
  cubec_context_set_module(self, parent);
  return self->value_undefined;
}
