#include "c/type.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/ptr_declarator.h"
#include "c/ptr_declarator.h"
#include "c/writer.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"

cubec_value_t cubec_c_write_type(cubec_context_t self, cubec_ast_type_t type,
                                 const char *filename, cubec_string_t *output) {
  if (type->expression->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
    char *name = cubec_location_get(type->expression->loc, self->allocator);
    cubec_value_t val = cubec_context_load_value(self, name);
    if (!val) {
      cubec_value_t err =
          cubec_c_create_error(self, type->expression, filename,
                               "Use of undeclared identifier '%s'", name);
      cubec_allocator_free(self->allocator, name);
      return err;
    }
    if (val->type->kind == CUBEC_TYPE_KIND_ERROR) {
      cubec_allocator_free(self->allocator, name);
      return val;
    }
    if (val->type->kind != CUBEC_TYPE_KIND_TYPE) {
      cubec_value_t err = cubec_c_create_error(self, type->expression, filename,
                                               "'%s' is not type", name);
      cubec_allocator_free(self->allocator, name);
      return err;
    }
    cubec_type_t t = *(cubec_type_t *)val->data;
    switch (t->kind) {
    case CUBEC_TYPE_KIND_VOID:
      cubec_string_concat(*output, self->allocator, "void");
      break;
    case CUBEC_TYPE_KIND_INT8:
      cubec_string_concat(*output, self->allocator, "int8_t");
      break;
    case CUBEC_TYPE_KIND_INT16:
      cubec_string_concat(*output, self->allocator, "int16_t");
      break;
    case CUBEC_TYPE_KIND_INT32:
      cubec_string_concat(*output, self->allocator, "int32_t");
      break;
    case CUBEC_TYPE_KIND_INT64:
      cubec_string_concat(*output, self->allocator, "int64_t");
      break;
    case CUBEC_TYPE_KIND_UINT8:
      cubec_string_concat(*output, self->allocator, "uint8_t");
      break;
    case CUBEC_TYPE_KIND_UINT16:
      cubec_string_concat(*output, self->allocator, "uint16_t");
      break;
    case CUBEC_TYPE_KIND_UINT32:
      cubec_string_concat(*output, self->allocator, "uint32_t");
      break;
    case CUBEC_TYPE_KIND_UINT64:
      cubec_string_concat(*output, self->allocator, "uint64_t");
      break;
    case CUBEC_TYPE_KIND_FLOAT32:
      cubec_string_concat(*output, self->allocator, "float");
      break;
    case CUBEC_TYPE_KIND_FLOAT64:
      cubec_string_concat(*output, self->allocator, "double");
      break;
    case CUBEC_TYPE_KIND_BOOLEAN:
      cubec_string_concat(*output, self->allocator, "bool");
      break;
    case CUBEC_TYPE_KIND_STR:
      cubec_string_concat(*output, self->allocator, "const char*");
      break;
    case CUBEC_TYPE_KIND_OPAQUE:
      cubec_string_concat(*output, self->allocator, "void*");
      break;
    case CUBEC_TYPE_KIND_STRUCT:
      cubec_string_concat(*output, self->allocator, "struct ");
      cubec_string_concat(*output, self->allocator, name);
      break;
    case CUBEC_TYPE_KIND_UNION:
      cubec_string_concat(*output, self->allocator, "union ");
      cubec_string_concat(*output, self->allocator, name);
      break;
    case CUBEC_TYPE_KIND_PTR:
    case CUBEC_TYPE_KIND_PTR_ARRAY:
    case CUBEC_TYPE_KIND_ARRAY:
    case CUBEC_TYPE_KIND_ENUM:
    case CUBEC_TYPE_KIND_RESULT:
    case CUBEC_TYPE_KIND_FUNCTION:
      cubec_string_concat(*output, self->allocator, name);
      break;
    default: {
      cubec_value_t err = cubec_c_create_error(self, type->expression, filename,
                                               "'%s' is not type", name);
      cubec_allocator_free(self->allocator, name);
      return err;
    }
    }
    cubec_allocator_free(self->allocator, name);
    return self->value_undefined;
  } else if (type->expression->type == CUBEC_NODE_TYPE_PTR_DECLARATOR) {
    return cubec_c_write_ptr_declarator(
        self, (cubec_ast_ptr_declarator_t)type->expression, filename, output);
  } else if (type->expression->type == CUBEC_NODE_TYPE_STRUCT_DECLARATOR) {
  } else if (type->expression->type == CUBEC_NODE_TYPE_ARRAY_DECLARATOR) {
  } else if (type->expression->type == CUBEC_NODE_TYPE_ENUM_DECLARATOR) {
  } else {
    return cubec_c_create_error(self, &type->super, filename,
                                "Invalid type expression");
  }
  return self->value_undefined;
}