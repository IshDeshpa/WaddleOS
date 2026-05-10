#ifndef LIST_H
#define LIST_H

// Circular doubly linked list

#include <stddef.h>

typedef struct list_elem list_elem_t;

#define GET_LIST_NODE(elem, type) ((type *)((uintptr_t)elem - offsetof(type, elem)))

typedef struct {
  list_elem_t *first;
  int len;
} list_head_t;

struct list_elem {
  list_head_t *head;
  list_elem_t *prev;
  list_elem_t *next;
};

void list_init(list_head_t *head);
list_elem_t *list_get(list_head_t *head, int index);
void list_insert(list_head_t *head, list_elem_t *new, int index);
void list_remove(list_head_t *head, int index);
void list_push(list_head_t *head, list_elem_t *new);
void list_push_first(list_head_t *head, list_elem_t *new);
void list_pop(list_head_t *head);
void list_pop_first(list_head_t *head);

#endif
