#include "core/rbtree.h"
#include "core/allocator.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct _cubec_rbtree_node_t {
  bool color;
  void *key;
  cubec_rbtree_node_t left;
  cubec_rbtree_node_t right;
  cubec_rbtree_node_t parent;
};
struct _cubec_rbtree_t {
  cubec_rbtree_node_t root;
  bool autofree;
  size_t size;
  size_t keysize;
  cubec_compare_fn_t compare;
  cubec_allocator_t allocator;
};

static void cubec_rbtree_dispose_node(cubec_rbtree_t self,
                                      cubec_rbtree_node_t node,
                                      cubec_allocator_t allocator) {
  if (node) {
    cubec_rbtree_dispose_node(self, node->left, allocator);
    cubec_rbtree_dispose_node(self, node->right, allocator);
    if (self->autofree) {
      cubec_allocator_free(allocator, node->key);
    }
    cubec_allocator_free(allocator, node);
  }
}

static void cubec_rbtree_dispose(cubec_allocator_t allocator,
                                 cubec_rbtree_t self) {
  cubec_rbtree_dispose_node(self, self->root, allocator);
}

cubec_rbtree_t cubec_create_rbtree(cubec_allocator_t allocator,
                                   cubec_rbtree_initialize_t *initialize) {
  cubec_rbtree_t tree =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_rbtree_t),
                            (cubec_dispose_fn_t)cubec_rbtree_dispose);
  tree->root = NULL;
  tree->autofree = false;
  tree->size = 0;
  tree->compare = NULL;
  if (initialize) {
    tree->autofree = initialize->autofree;
    tree->compare = initialize->compare;
  }
  tree->allocator = allocator;
  return tree;
}

static void cubec_rbtree_left_rotate(cubec_rbtree_t self,
                                     cubec_rbtree_node_t x) {
  /*
   *     p               p
   *     |               |
   *     x               y
   *    / \    =>       / \
   *   a   y           x   c
   *      / \         / \
   *     b   c       a   b
   */
  cubec_rbtree_node_t y = x->right;
  cubec_rbtree_node_t b = y->left;
  cubec_rbtree_node_t p = x->parent;

  x->right = b;
  if (b) {
    b->parent = x;
  }

  if (p == NULL) {
    self->root = y;
  } else if (p->left == x) {
    p->left = y;
  } else {
    p->right = y;
  }
  y->parent = p;

  y->left = x;
  x->parent = y;
}

static void cubec_rbtree_right_rotate(cubec_rbtree_t self,
                                      cubec_rbtree_node_t y) {
  /*
   *      p            p
   *      |            |
   *      y            x
   *     / \    =>    / \
   *    x   c        a   y
   *   / \              / \
   *  a   b            b   c
   */

  cubec_rbtree_node_t x = y->left;
  cubec_rbtree_node_t b = x->right;
  cubec_rbtree_node_t p = y->parent;

  y->left = b;
  if (b) {
    b->parent = y;
  }

  if (p == NULL) {
    self->root = x;
  } else if (p->left == y) {
    p->left = x;
  } else {
    p->right = x;
  }
  x->parent = p;

  x->right = y;
  y->parent = x;
}
static void cubec_rbtree_insert_fixup(cubec_rbtree_t self,
                                      cubec_rbtree_node_t node) {
  cubec_rbtree_node_t parent, gparent;
  while ((parent = node->parent) && parent && parent->color) {
    gparent = parent->parent;
    if (parent == gparent->left) {
      cubec_rbtree_node_t uncle = gparent->right;
      if (uncle && uncle->color) {
        uncle->color = false;
        parent->color = false;
        gparent->color = true;
        node = gparent;
        continue;
      }
      if (parent->right == node) {
        cubec_rbtree_left_rotate(self, parent);
        cubec_rbtree_node_t tmp = parent;
        parent = node;
        node = tmp;
      }
      parent->color = false;
      gparent->color = true;
      cubec_rbtree_right_rotate(self, gparent);
    } else {
      cubec_rbtree_node_t uncle = gparent->left;
      if (uncle && uncle->color) {
        uncle->color = false;
        parent->color = false;
        gparent->color = true;
        node = gparent;
        continue;
      }
      if (parent->left == node) {
        cubec_rbtree_right_rotate(self, parent);
        cubec_rbtree_node_t tmp = parent;
        parent = node;
        node = tmp;
      }
      parent->color = false;
      gparent->color = true;
      cubec_rbtree_left_rotate(self, gparent);
    }
  }
  self->root->color = false;
}

static void cubec_rbtree_node_dispose(cubec_allocator_t allocator,
                                      cubec_rbtree_node_t self) {}
static cubec_rbtree_node_t
cubec_create_rbtree_node(cubec_allocator_t allocator) {
  cubec_rbtree_node_t node =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_rbtree_node_t),
                            (cubec_dispose_fn_t)cubec_rbtree_node_dispose);
  node->color = false;
  node->key = 0;
  node->left = NULL;
  node->right = NULL;
  node->parent = NULL;
  return node;
}

void cubec_rbtree_put(cubec_rbtree_t self, void *key, void *cmp_arg) {
  cubec_rbtree_node_t y = NULL;
  cubec_rbtree_node_t x = self->root;
  while (x != NULL) {
    y = x;
    if (self->compare ? self->compare(key, x->key, cmp_arg) < 0
                      : key < x->key) {
      x = x->left;
    } else if (self->compare ? self->compare(key, x->key, cmp_arg) == 0
                             : key == x->key) {
      if (x->key != key && self->autofree) {
        cubec_allocator_free(self->allocator, x->key);
      }
      x->key = key;
      return;
    } else {
      x = x->right;
    }
  }
  cubec_rbtree_node_t node = cubec_create_rbtree_node(self->allocator);
  node->key = key;
  node->parent = y;
  if (y != NULL) {
    if (self->compare ? self->compare(node->key, y->key, cmp_arg) < 0
                      : node->key < y->key) {
      y->left = node;
    } else {
      y->right = node;
    }
  } else {
    self->root = node;
  }
  node->color = true;
  cubec_rbtree_insert_fixup(self, node);
  self->size++;
}
bool cubec_rbtree_has(cubec_rbtree_t self, const void *key, void *cmp_arg) {
  return cubec_rbtree_get(self, key, cmp_arg) != NULL;
}
void *cubec_rbtree_get(cubec_rbtree_t self, const void *key, void *cmp_arg) {
  cubec_rbtree_node_t y = NULL;
  cubec_rbtree_node_t x = self->root;
  while (x != NULL) {
    y = x;
    if (self->compare ? self->compare(key, x->key, cmp_arg) < 0
                      : key < x->key) {
      x = x->left;
    } else if (self->compare ? self->compare(key, x->key, cmp_arg) == 0
                             : key == x->key) {
      return x->key;
    } else {
      x = x->right;
    }
  }
  return NULL;
}

static void cubec_rbtree_remove_fixup(cubec_rbtree_t self,
                                      cubec_rbtree_node_t node) {
  cubec_rbtree_node_t parent = node->parent;
  cubec_rbtree_node_t sibling = NULL;
  if (node == parent->left) {
    sibling = parent->right;
    if (!sibling->color) {
      // parent->right[Black]
      if (sibling->right && sibling->right->color) {
        // RR S[Black] parent->right[Black]->right[Red]
        sibling->right->color = sibling->color;
        sibling->color = parent->color;
        parent->color = false;
        cubec_rbtree_left_rotate(self, parent);
      } else if (sibling->left && sibling->left->color) {
        // RL
        // parent->right[Black]->left[Red]
        // parent->right[Black]->right?[Black]
        sibling->left->color = parent->color;
        parent->color = false;
        cubec_rbtree_right_rotate(self, sibling);
        cubec_rbtree_left_rotate(self, parent);
      } else if ((!sibling->left || !sibling->left->color) &&
                 (!sibling->right || !sibling->right->color)) {
        // parent->right[Black]->left?[Black]
        // parent->right[Black]->right?[Black]
        sibling->color = true;
        if (parent->color) {
          parent->color = false;
        } else if (parent != self->root) {
          cubec_rbtree_remove_fixup(self, parent);
        }
      }
    } else {
      bool color = sibling->color;
      sibling->color = parent->color;
      parent->color = color;
      cubec_rbtree_left_rotate(self, parent);
      cubec_rbtree_remove_fixup(self, node);
    }
  } else {
    sibling = parent->left;
    if (!sibling->color) {
      if (sibling->right && sibling->right->color) { // LR
        sibling->right->color = parent->color;
        parent->color = false;
        cubec_rbtree_left_rotate(self, sibling);
        cubec_rbtree_right_rotate(self, parent);
      } else if (sibling->left && sibling->left->color) { // LL
        sibling->left->color = sibling->color;
        sibling->color = parent->color;
        parent->color = false;
        cubec_rbtree_right_rotate(self, parent);
      } else if ((!sibling->left || !sibling->left->color) &&
                 (!sibling->right || !sibling->right->color)) {
        sibling->color = true;
        if (parent->color) {
          parent->color = false;
        } else if (parent != self->root) {
          cubec_rbtree_remove_fixup(self, parent);
        }
      }
    } else {
      bool color = sibling->color;
      sibling->color = parent->color;
      parent->color = color;
      cubec_rbtree_right_rotate(self, parent);
      cubec_rbtree_remove_fixup(self, node);
    }
  }
}

void cubec_rbtree_remove(cubec_rbtree_t self, const void *key, void *cmp_arg) {
  cubec_rbtree_node_t node = self->root;
  while (node) {
    if (self->compare ? self->compare(key, node->key, cmp_arg) == 0
                      : key == node->key) {
      break;
    }
    if (self->compare ? self->compare(key, node->key, cmp_arg) < 0
                      : key < node->key) {
      node = node->left;
    } else {
      node = node->right;
    }
  }
  if (!node) {
    return;
  }
  if (node->left && node->right) {
    cubec_rbtree_node_t replace = node->right;
    while (replace->left) {
      replace = replace->left;
    }
    cubec_rbtree_node_t tmp = node;
    node->key = replace->key;
    replace->key = tmp->key;
    node = replace;
  }
  cubec_rbtree_node_t parent = node->parent;
  cubec_rbtree_node_t child = NULL;
  if (node->left) {
    child = node->left;
  } else if (node->right) {
    child = node->right;
  }
  if (child) {
    if (!parent) {
      self->root = child;
    } else if (node == parent->left) {
      parent->left = child;
    } else {
      parent->right = child;
    }
    child->parent = parent;
    child->color = false;
  } else if (!parent) {
    self->root = NULL;
  } else {
    if (node->color) {
      if (parent->left == node) {
        parent->left = NULL;
      } else {
        parent->right = NULL;
      }
    } else {
      cubec_rbtree_remove_fixup(self, node);
      if (node == parent->left) {
        parent->left = NULL;
      } else if (node == parent->right) {
        parent->right = NULL;
      }
    }
  }
  if (self->autofree) {
    cubec_allocator_free(self->allocator, node->key);
  }
  cubec_allocator_free(self->allocator, node);
  self->size--;
}
size_t cubec_rbtree_size(cubec_rbtree_t self) { return self->size; }
cubec_rbtree_node_t cubec_rbtree_get_first(cubec_rbtree_t self) {
  if (self->root) {
    cubec_rbtree_node_t node = self->root;
    while (node->left) {
      node = node->left;
    }
    return node;
  }
  return NULL;
}
cubec_rbtree_node_t cubec_rbtree_get_last(cubec_rbtree_t self) {
  if (self->root) {
    cubec_rbtree_node_t node = self->root;
    while (node->right) {
      node = node->right;
    }
    return node;
  }
  return NULL;
}
cubec_rbtree_node_t cubec_rbtree_node_next(cubec_rbtree_node_t self) {
  if (self->right) {
    cubec_rbtree_node_t node = self->right;
    while (node->left) {
      node = node->left;
    }
    return node;
  }
  if (self->parent) {
    if (self == self->parent->left) {
      return self->parent;
    }
    while (self->parent && self == self->parent->right) {
      self = self->parent;
    }
    return self->parent;
  }
  return NULL;
}
cubec_rbtree_node_t cubec_rbtree_node_last(cubec_rbtree_node_t self) {
  if (self->left) {
    cubec_rbtree_node_t node = self->left;
    while (node->right) {
      node = node->right;
    }
    return node;
  }
  if (self->parent) {
    if (self == self->parent->right) {
      return self->parent;
    }
    while (self->parent && self == self->parent->left) {
      self = self->parent;
    }
    return self->parent;
  }
  return NULL;
}
void *cubec_rbtree_node_get(cubec_rbtree_node_t self) { return self->key; }