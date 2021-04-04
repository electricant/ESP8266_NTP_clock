#include "qd_sched.h"

// The latest millis() value when loop() was entered
unsigned long entry_time;
// Elapsed time (time spent doing stuff) during the last loop() run
unsigned long elapsed_time;
// Tasks to run
task_t tasks[SCHED_NUM_TASKS] = {0};

size_t sched_put_task(void (*taskFunction)(void), unsigned long rate)
{
  size_t emptyIdx = 0;
  
  // search for the first empty task entry within tasks
  while(tasks[emptyIdx].taskFunc != 0)
  {
    emptyIdx++;
    if(emptyIdx == SCHED_NUM_TASKS)
      return -1;
  }

  tasks[emptyIdx].taskFunc = taskFunction;
  tasks[emptyIdx].rateMillis = rate;
  return emptyIdx;
}

size_t sched_put_taskID(size_t id, void (*taskFunction)(void), unsigned long rate)
{
  if (id >= SCHED_NUM_TASKS)
    return -1;

  tasks[id].taskFunc = taskFunction;
  tasks[id].rateMillis = rate;
  return id;
}

uint8_t sched_get_CPU_usage()
{
  return (100*elapsed_time)/SCHED_TICK_MS;
}

void loop() {
  entry_time = millis();

  for(size_t i = 0; i < SCHED_NUM_TASKS; i++)
  {
    if (tasks[i].taskFunc && (entry_time - tasks[i].lastRunMillis) >= tasks[i].rateMillis)
    {
      tasks[i].lastRunMillis = millis();
      (*tasks[i].taskFunc)();
    }
  }
  
  elapsed_time = millis() - entry_time;
 
  // sleep until the next full tick
  delay(SCHED_TICK_MS - (elapsed_time % SCHED_TICK_MS));
}
