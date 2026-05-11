#ifndef LIST_H
#define LIST_H

#include <stddef.h>

// Circular doubly linked list

typedef struct list_elem list_elem_t;

#define GET_LIST_NODE(elem, struct_name, elem_name) ((struct_name *)((uintptr_t)elem - offsetof(struct_name, elem_name)))

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
void list_foreach(list_head_t *head, void (*func)(list_elem_t *, int, void *), void *aux);

#endif
