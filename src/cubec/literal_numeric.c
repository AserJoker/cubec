#include "cubec/literal_numeric.h"
#include "core/emit_context.h"
#include "core/token.h"
#include "cubec/node_error.h"
#include "cubec/token.h"
#include <string.h>

static void _cubec_literal_numeric_init(cubec_literal_numeric_t self,
                                        allocator_t allocator,
                                        cubec_literal_numeric_init_t *init) {
  if (!init)
    return;
  cubec_literal_init_t super_init = {
      .kind = CUBEC_NODE_LITERAL_NUMERIC,
      .parent = NULL,
  };
  super_init.location = init->location;
  self->kind = init->kind;
  self->numeric_type = init->numeric_type;
  g_cubec_literal_type.init(&self->super, allocator, &super_init);
  if (init->value) {
    self->value = allocator_create(allocator, &g_string_type,
                                   &(string_init_t){.str = init->value});
  } else {
    self->value = allocator_create(allocator, &g_string_type, NULL);
  }
}

static void _cubec_literal_numeric_dispose(cubec_literal_numeric_t self,
                                           allocator_t allocator) {
  if (self->value) {
    allocator_free(allocator, &self->value);
  }
  g_cubec_literal_type.dispose(&self->super, allocator);
}

static void _cubec_literal_numeric_clone(cubec_literal_numeric_t self,
                                         allocator_t allocator,
                                         cubec_literal_numeric_t another) {
  g_cubec_literal_type.clone(&self->super, allocator, &another->super);
  self->kind = another->kind;
  self->numeric_type = another->numeric_type;
  self->value = alloc_clone(allocator, another->value);
}

static void _cubec_literal_numeric_move(cubec_literal_numeric_t self,
                                        allocator_t allocator,
                                        cubec_literal_numeric_t another) {
  g_cubec_literal_type.move(&self->super, allocator, &another->super);
  self->kind = another->kind;
  self->numeric_type = another->numeric_type;
  self->value = alloc_move(allocator, another->value);
}

type_t g_cubec_literal_numeric_type = {
    .name = "cubec.cubec.literal_numeric",
    .size = sizeof(struct _cubec_literal_numeric_t),
    .init = (type_init_fn_t)_cubec_literal_numeric_init,
    .dispose = (type_dispose_fn_t)_cubec_literal_numeric_dispose,
    .clone = (type_clone_fn_t)_cubec_literal_numeric_clone,
    .move = (type_move_fn_t)_cubec_literal_numeric_move,
};

const char *
cubec_literal_numeric_type_to_string(cubec_literal_numeric_type_t type) {
  switch (type) {
  case CUBEC_LITERAL_NUMERIC_TYPE_I8:
    return "i8";
  case CUBEC_LITERAL_NUMERIC_TYPE_I16:
    return "i16";
  case CUBEC_LITERAL_NUMERIC_TYPE_I32:
    return "i32";
  case CUBEC_LITERAL_NUMERIC_TYPE_I64:
    return "i64";
  case CUBEC_LITERAL_NUMERIC_TYPE_U8:
    return "u8";
  case CUBEC_LITERAL_NUMERIC_TYPE_U16:
    return "u16";
  case CUBEC_LITERAL_NUMERIC_TYPE_U32:
    return "u32";
  case CUBEC_LITERAL_NUMERIC_TYPE_U64:
    return "u64";
  case CUBEC_LITERAL_NUMERIC_TYPE_F16:
    return "f16";
  case CUBEC_LITERAL_NUMERIC_TYPE_F32:
    return "f32";
  case CUBEC_LITERAL_NUMERIC_TYPE_F64:
    return "f64";
  default:
    return "";
  }
}

static cubec_literal_numeric_type_t parse_type_suffix(const char *suffix,
                                                      size_t len) {
  if (len == 2 && strncmp(suffix, "i8", 2) == 0)
    return CUBEC_LITERAL_NUMERIC_TYPE_I8;
  if (len == 3 && strncmp(suffix, "i16", 3) == 0)
    return CUBEC_LITERAL_NUMERIC_TYPE_I16;
  if (len == 3 && strncmp(suffix, "i32", 3) == 0)
    return CUBEC_LITERAL_NUMERIC_TYPE_I32;
  if (len == 3 && strncmp(suffix, "i64", 3) == 0)
    return CUBEC_LITERAL_NUMERIC_TYPE_I64;
  if (len == 2 && strncmp(suffix, "u8", 2) == 0)
    return CUBEC_LITERAL_NUMERIC_TYPE_U8;
  if (len == 3 && strncmp(suffix, "u16", 3) == 0)
    return CUBEC_LITERAL_NUMERIC_TYPE_U16;
  if (len == 3 && strncmp(suffix, "u32", 3) == 0)
    return CUBEC_LITERAL_NUMERIC_TYPE_U32;
  if (len == 3 && strncmp(suffix, "u64", 3) == 0)
    return CUBEC_LITERAL_NUMERIC_TYPE_U64;
  if (len == 3 && strncmp(suffix, "f16", 3) == 0)
    return CUBEC_LITERAL_NUMERIC_TYPE_F16;
  if (len == 3 && strncmp(suffix, "f32", 3) == 0)
    return CUBEC_LITERAL_NUMERIC_TYPE_F32;
  if (len == 3 && strncmp(suffix, "f64", 3) == 0)
    return CUBEC_LITERAL_NUMERIC_TYPE_F64;
  return CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT;
}

static bool is_float_token(const char *value, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (value[i] == '.')
      return true;
  }
  return false;
}

node_t read_literal_numeric(context_t ctx, vec_t tokens, size_t *position,
                            const char *filename) {
  allocator_t allocator = ctx->allocator;
  size_t current = *position;

  token_t first_token = vec_get(tokens, current);
  if (!token_is(first_token, CUBEC_TOKEN_NUMERIC, NULL)) {
    return NULL;
  }

  location_t start_location = *token_get_location(first_token);
  start_location.filename = filename;

  location_t *location = token_get_location(first_token);
  const char *token_str = token_get_string(first_token);
  size_t token_len = token_get_string_length(first_token);

  cubec_literal_numeric_kind_t kind = is_float_token(token_str, token_len)
                                          ? CUBEC_LITERAL_NUMERIC_KIND_FLOAT
                                          : CUBEC_LITERAL_NUMERIC_KIND_INTEGER;
  cubec_literal_numeric_type_t numeric_type =
      CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT;
  cubec_literal_numeric_init_t init = {
      .location = *location,
      .parent = NULL,
      .value = NULL,
      .kind = kind,
      .numeric_type = numeric_type,
  };
  cubec_literal_numeric_t node =
      allocator_create(allocator, &g_cubec_literal_numeric_type, &init);
  if (!node)
    goto onerror;
  node_t node_base = (node_t)node;
  node_base->location.filename = filename;

  string_nconcat(node->value, token_str, token_len);
  current++;

  token_t type_token = vec_get(tokens, current);
  if (token_is(type_token, CUBEC_TOKEN_IDENTIFIER, NULL)) {
    const char *ident_str = token_get_string(type_token);
    size_t ident_len = token_get_string_length(type_token);

    cubec_literal_numeric_type_t type = parse_type_suffix(ident_str, ident_len);
    if (type != CUBEC_LITERAL_NUMERIC_TYPE_DEFAULT) {
      string_nconcat(node->value, ident_str, ident_len);
      node->numeric_type = type;
      location_t *type_location = token_get_location(type_token);
      node_base->location.end = type_location->end;
      current++;
    }
  }

  *position = current;
  return node_base;
onerror:
  diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, start_location,
                       "invalid numeric literal");
  allocator_free(allocator, &node);
  return create_error(ctx, start_location);
}

node_t create_literal_numeric(context_t ctx, location_t loc, const char *value,
                              cubec_literal_numeric_kind_t kind,
                              cubec_literal_numeric_type_t ntype) {
  allocator_t alloc = ctx->allocator;
  cubec_literal_numeric_init_t init = {
      .value = value, .kind = kind, .numeric_type = ntype};
  return (node_t)allocator_create(alloc, &g_cubec_literal_numeric_type, &init);
}

void emit_literal_numeric(emit_context_t ctx, node_t node) {
  cubec_literal_numeric_t num = (cubec_literal_numeric_t)node;
  recover_comments_to(ctx, node->location.begin.offset);
  emit_numeric(ctx, string_get(num->value));
}