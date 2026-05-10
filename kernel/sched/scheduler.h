#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t tid_t;

typedef enum {
  T_RUNNING,
  T_READY,
  T_BLOCKED,
  T_DYING
} thread_state_t;

#define TASK_MAGIC (0xF0CACC1A)
#define TASK_NAME_SIZE (16)

// Task control block
typedef struct {
  char name[TASK_NAME_SIZE];
  tid_t tid;
  thread_state_t state;

  uint32_t magic;
} tcb_t;

// Scheduling policy
typedef tcb_t *(*sched_policy_t)(void); 

// Runnable
typedef void (*runnable_t)(void *);

void sched_setup_init_task();
tcb_t *sched_get_curr_task();
void sched_set_policy(sched_policy_t policy);
void sched_create_task(const char *name, runnable_t runnable, void *aux);
void sched_schedule();

#endif
