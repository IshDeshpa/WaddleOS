#include "list.h"
#include "waddletest.h"
#include <stddef.h>

typedef struct {
  int value;
  list_elem_t elem;
} node_t;

#define NODE_COUNT 8
static node_t nodes[NODE_COUNT];
static list_head_t lst;

void setup_list() {
  list_init(&lst);
  for (int i = 0; i < NODE_COUNT; i++) {
    nodes[i].value = i;
    nodes[i].elem.head = NULL;
    nodes[i].elem.prev = NULL;
    nodes[i].elem.next = NULL;
  }
}

// list_init zeroes the head and sets len to 0
bool test_list_init() {
  TEST_ASSERT_EQ_PTR(lst.first, NULL);
  TEST_ASSERT_EQ_INT(lst.len, 0);
  return true;
}

// list_push appends to the back; order is preserved
bool test_list_push() {
  for (int i = 0; i < 4; i++) {
    list_push(&lst, &nodes[i].elem);
    TEST_ASSERT_EQ_INT(lst.len, i + 1);
    // elem tracks its owning list
    TEST_ASSERT_EQ_PTR(nodes[i].elem.head, &lst);
  }
  for (int i = 0; i < 4; i++) {
    node_t *n = GET_LIST_NODE(list_get(&lst, i), node_t, elem);
    TEST_ASSERT_EQ_INT(n->value, i);
  }
  return true;
}

// list_push_first prepends to the front; order is reversed
bool test_list_push_first() {
  for (int i = 0; i < 4; i++) {
    list_push_first(&lst, &nodes[i].elem);
    TEST_ASSERT_EQ_INT(lst.len, i + 1);
    TEST_ASSERT_EQ_PTR(lst.first, &nodes[i].elem);
    TEST_ASSERT_EQ_PTR(nodes[i].elem.head, &lst);
  }
  // Pushed 0,1,2,3 to front → stored as 3,2,1,0
  for (int i = 0; i < 4; i++) {
    node_t *n = GET_LIST_NODE(list_get(&lst, i), node_t, elem);
    TEST_ASSERT_EQ_INT(n->value, 3 - i);
  }
  return true;
}

// list_pop removes from the back and clears the element's pointers
bool test_list_pop() {
  for (int i = 0; i < 4; i++)
    list_push(&lst, &nodes[i].elem);

  for (int i = 3; i >= 0; i--) {
    list_pop(&lst);
    TEST_ASSERT_EQ_INT(lst.len, i);
    // Popped element is fully detached
    TEST_ASSERT_EQ_PTR(nodes[i].elem.head, NULL);
    TEST_ASSERT_EQ_PTR(nodes[i].elem.next, NULL);
    TEST_ASSERT_EQ_PTR(nodes[i].elem.prev, NULL);
  }
  // Empty list: first must be NULL
  TEST_ASSERT_EQ_PTR(lst.first, NULL);
  return true;
}

// list_pop_first removes from the front and advances head correctly
bool test_list_pop_first() {
  for (int i = 0; i < 4; i++)
    list_push(&lst, &nodes[i].elem);

  for (int i = 0; i < 4; i++) {
    node_t *front = GET_LIST_NODE(lst.first, node_t, elem);
    TEST_ASSERT_EQ_INT(front->value, i);
    list_pop_first(&lst);
    TEST_ASSERT_EQ_INT(lst.len, 3 - i);
    // Popped element is fully detached
    TEST_ASSERT_EQ_PTR(nodes[i].elem.head, NULL);
    TEST_ASSERT_EQ_PTR(nodes[i].elem.next, NULL);
    TEST_ASSERT_EQ_PTR(nodes[i].elem.prev, NULL);
  }
  TEST_ASSERT_EQ_PTR(lst.first, NULL);
  return true;
}

// list_get returns the correct node; tests both forward and backward traversal paths
bool test_list_get() {
  for (int i = 0; i < NODE_COUNT; i++)
    list_push(&lst, &nodes[i].elem);

  for (int i = 0; i < NODE_COUNT; i++) {
    node_t *n = GET_LIST_NODE(list_get(&lst, i), node_t, elem);
    TEST_ASSERT_EQ_INT(n->value, i);
  }
  return true;
}

// list_insert places a node at the given index and shifts later nodes right
bool test_list_insert() {
  // Build list: [0, 2]
  list_push(&lst, &nodes[0].elem);
  list_push(&lst, &nodes[2].elem);

  // Insert 1 in the middle → [0, 1, 2]
  list_insert(&lst, &nodes[1].elem, 1);
  TEST_ASSERT_EQ_INT(lst.len, 3);
  for (int i = 0; i < 3; i++) {
    node_t *n = GET_LIST_NODE(list_get(&lst, i), node_t, elem);
    TEST_ASSERT_EQ_INT(n->value, i);
  }

  // Insert at front → [3, 0, 1, 2]
  list_insert(&lst, &nodes[3].elem, 0);
  TEST_ASSERT_EQ_INT(lst.len, 4);
  node_t *head = GET_LIST_NODE(lst.first, node_t, elem);
  TEST_ASSERT_EQ_INT(head->value, 3);

  // Insert at back (index == len) → [3, 0, 1, 2, 4]
  list_insert(&lst, &nodes[4].elem, lst.len);
  TEST_ASSERT_EQ_INT(lst.len, 5);
  node_t *tail = GET_LIST_NODE(list_get(&lst, lst.len - 1), node_t, elem);
  TEST_ASSERT_EQ_INT(tail->value, 4);

  return true;
}

// list_remove handles middle, head, and tail removal correctly
bool test_list_remove() {
  for (int i = 0; i < 5; i++)
    list_push(&lst, &nodes[i].elem);
  // List: [0, 1, 2, 3, 4]

  // Remove middle: index 2 (value 2) → [0, 1, 3, 4]
  list_remove(&lst, 2);
  TEST_ASSERT_EQ_INT(lst.len, 4);
  TEST_ASSERT_EQ_PTR(nodes[2].elem.head, NULL);
  node_t *mid = GET_LIST_NODE(list_get(&lst, 2), node_t, elem);
  TEST_ASSERT_EQ_INT(mid->value, 3);

  // Remove head: index 0 (value 0) → [1, 3, 4]
  list_remove(&lst, 0);
  TEST_ASSERT_EQ_INT(lst.len, 3);
  node_t *new_head = GET_LIST_NODE(lst.first, node_t, elem);
  TEST_ASSERT_EQ_INT(new_head->value, 1);
  TEST_ASSERT_EQ_PTR(nodes[0].elem.head, NULL);

  // Remove tail: index 2 (value 4) → [1, 3]
  list_remove(&lst, lst.len - 1);
  TEST_ASSERT_EQ_INT(lst.len, 2);
  node_t *new_tail = GET_LIST_NODE(list_get(&lst, lst.len - 1), node_t, elem);
  TEST_ASSERT_EQ_INT(new_tail->value, 3);
  TEST_ASSERT_EQ_PTR(nodes[4].elem.head, NULL);

  return true;
}

// Single-element lists are a common edge case for all push/pop pairs
bool test_list_single_elem() {
  // push / pop
  list_push(&lst, &nodes[0].elem);
  TEST_ASSERT_EQ_INT(lst.len, 1);
  TEST_ASSERT_EQ_PTR(lst.first, &nodes[0].elem);
  list_pop(&lst);
  TEST_ASSERT_EQ_INT(lst.len, 0);
  TEST_ASSERT_EQ_PTR(lst.first, NULL);

  // push_first / pop_first
  list_push_first(&lst, &nodes[0].elem);
  TEST_ASSERT_EQ_INT(lst.len, 1);
  TEST_ASSERT_EQ_PTR(lst.first, &nodes[0].elem);
  list_pop_first(&lst);
  TEST_ASSERT_EQ_INT(lst.len, 0);
  TEST_ASSERT_EQ_PTR(lst.first, NULL);

  // push / pop_first
  list_push(&lst, &nodes[0].elem);
  list_pop_first(&lst);
  TEST_ASSERT_EQ_INT(lst.len, 0);
  TEST_ASSERT_EQ_PTR(lst.first, NULL);

  // push_first / pop
  list_push_first(&lst, &nodes[0].elem);
  list_pop(&lst);
  TEST_ASSERT_EQ_INT(lst.len, 0);
  TEST_ASSERT_EQ_PTR(lst.first, NULL);

  return true;
}

// Elements are reusable after being removed from a list
bool test_list_reuse_elem() {
  list_push(&lst, &nodes[0].elem);
  list_pop(&lst);
  // nodes[0].elem.head is now NULL; push should succeed
  list_push(&lst, &nodes[0].elem);
  TEST_ASSERT_EQ_INT(lst.len, 1);
  TEST_ASSERT_EQ_PTR(nodes[0].elem.head, &lst);
  return true;
}

// GET_LIST_NODE recovers the containing struct from an embedded elem pointer
bool test_list_get_list_node() {
  for (int i = 0; i < NODE_COUNT; i++)
    list_push(&lst, &nodes[i].elem);

  for (int i = 0; i < NODE_COUNT; i++) {
    list_elem_t *e = list_get(&lst, i);
    node_t *n = GET_LIST_NODE(e, node_t, elem);
    // The recovered pointer must be the original node
    TEST_ASSERT_EQ_PTR(n, &nodes[i]);
    TEST_ASSERT_EQ_INT(n->value, i);
  }
  return true;
}

void operate(list_elem_t *elem, int index, void *aux){
  (void)aux;
  
  if(index % 2 == 0){
    node_t *n = GET_LIST_NODE(elem, node_t, elem);
    n->value = 0xFFFFFFFF;
  }
}

bool test_list_foreach(){
  for (int i = 0; i < NODE_COUNT; i++)
    list_push(&lst, &nodes[i].elem);

  list_foreach(&lst, operate, NULL);

  for (int i = 0; i < NODE_COUNT; i++){
    // Verify that each node index divisible by 2 is set to max value
    node_t *n = GET_LIST_NODE(list_get(&lst, i), node_t, elem);
    if(i%2 == 0) TEST_ASSERT_EQ_INT(n->value, (int)0xFFFFFFFF);
    else TEST_ASSERT_EQ_INT(n->value, i);
  }

  return true;
}
