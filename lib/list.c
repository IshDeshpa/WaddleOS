#include "list.h"
#include "utils.h"
#include <stdint.h>
#include <stdbool.h>

void list_init(list_head_t *head){
  head->first = NULL;
  head->len = 0;
}

static void _list_insert(list_head_t *head, list_elem_t *prev, list_elem_t *next, list_elem_t *new){
  ASSERT(prev->next == next);
  ASSERT(next->prev == prev);
  ASSERT(new->head == NULL);

  prev->next = new;
  next->prev = new;
  new->prev = prev;
  new->next = next;

  new->head = head;
  
  head->len++;
}

static list_elem_t *_list_remove(list_head_t *head, list_elem_t *elem){
  list_elem_t *prev = elem->prev;
  list_elem_t *next = elem->next;
  prev->next = next;
  next->prev = prev;

  elem->next = NULL;
  elem->prev = NULL;
  elem->head = NULL;

  head->len--;

  return elem;
}

list_elem_t *list_get(list_head_t *head, int index){
  ASSERT(head != NULL);
  ASSERT(index >= 0);
  ASSERT(index < head->len);

  // Traverse in the direction that's closest
  bool backwards = (index > head->len/2);
  int ctr = backwards?head->len:0;
  int8_t dir = backwards?-1:1;

  list_elem_t *curr = head->first;

  while(ctr != index){
    ctr += dir;
    curr = backwards?curr->prev:curr->next;
  }

  return curr;
}

void list_insert(list_head_t *head, list_elem_t *new, int index){
  ASSERT(head != NULL);
  ASSERT(index >= 0);
  ASSERT(index <= head->len);
  ASSERT(new != NULL);

  if(index == 0){
    list_push_first(head, new);
    return;
  }

  if(index == head->len){
    list_push(head, new);
    return;
  }

  list_elem_t *elem = list_get(head, index);
  _list_insert(head, elem->prev, elem, new);
}

void list_remove(list_head_t *head, int index){
  ASSERT(head != NULL);
  ASSERT(index >= 0);
  ASSERT(index < head->len);

  if(index == 0){
    list_pop_first(head);
    return;
  }

  if(index == head->len - 1){
    list_pop(head);
    return;
  }

  list_elem_t *elem = list_get(head, index);
  _list_remove(head, elem);
}

void list_push(list_head_t *head, list_elem_t *new){
  ASSERT(head != NULL);
  ASSERT(new != NULL);

  if(head->len == 0){
    head->first = new;
    new->next = new;
    new->prev = new;
    new->head = head;
    head->len++;
    return;
  }
  _list_insert(head, head->first->prev, head->first, new);
}

void list_push_first(list_head_t *head, list_elem_t *new){
  ASSERT(head != NULL);
  ASSERT(new != NULL);

  list_push(head, new);
  head->first = head->first->prev;
}

void list_pop(list_head_t *head){
  ASSERT(head != NULL);
  ASSERT(head->len > 0);

  _list_remove(head, head->first->prev);
  
  if(head->len == 0) head->first = NULL;
}

void list_pop_first(list_head_t *head){
  ASSERT(head != NULL);
  ASSERT(head->len > 0);

  list_elem_t *new_first = head->first->next;
  _list_remove(head, head->first);

  head->first = (head->len==0)?NULL:new_first;
}
