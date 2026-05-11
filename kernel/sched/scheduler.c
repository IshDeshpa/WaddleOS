#include "scheduler.h"
#include "interrupts.h"
#include "utils.h"
#include "string.h"

static uint32_t next_tid = 1;

void sched_task_init(tcb_t *task, char *name, thread_state_t init_state){
  memcpy(task->name, name, TASK_NAME_SIZE);

  task->magic = TASK_MAGIC;
  task->tid = next_tid++;
  task->state = init_state;
}

void sched_setup_init_task(){
  ASSERT(interrupts_active() == false);
  
  tcb_t *curr_tcb = sched_get_curr_task();
  sched_task_init(curr_tcb, "init", T_RUNNING);
}

tcb_t *sched_get_curr_task(){
  // Get current sp
  void *sp;
  asm volatile ("mov %%rsp, %0" : "=r"(sp));
  
  // Round down
  tcb_t *tcb_base = (tcb_t *)ROUND_DOWN_TO((uintptr_t)sp, 4096);
  return tcb_base;
}
