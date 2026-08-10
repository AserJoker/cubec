#include "core/node.h"

static void _node_init(node_t self, allocator_t allocator, node_init_t *init) {
  if (init) {
    self->kind = init->kind;
    self->location = init->location;
    self->parent = init->parent;
  } else {
    self->kind = 0;
    self->location = (location_t){};
    self->parent = NULL;
  }
  self->allocator = allocator;
}
static void _node_dispose(node_t self, allocator_t allocator) {
  (void)self;
  (void)allocator;
}

static void _node_clone(node_t self, allocator_t allocator, node_t another) {
  (void)allocator;
  self->kind = another->kind;
  self->location = another->location;
  self->parent = NULL;
}
static void _node_move(node_t self, allocator_t allocator, node_t another) {
  (void)allocator;
  self->kind = another->kind;
  self->location = another->location;
  self->parent = NULL;
}
class_t g_node_class = {
    .name = "cubec.core.node",
    .size = sizeof(struct _node_t),
    .init = (class_init_fn_t)_node_init,
    .dispose = (class_dispose_fn_t)_node_dispose,
    .clone = (class_clone_fn_t)_node_clone,
    .move = (class_move_fn_t)_node_move,
};
