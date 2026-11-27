/*
 * scheduler.c
 *
 * O(1) cooperative scheduler using sorted relative-delay array.
 * Reference: relative-delay delta queue where SCH_Update() only touches index 0.
 */

#include "scheduler.h"
#include "main.h"
#include <stddef.h>

sTask SCH_tasks_G[SCH_MAX_TASKS];
static volatile uint8_t SCH_count = 0;
static uint32_t next_task_id = 1;

static uint32_t sch_enter_critical(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void sch_exit_critical(uint32_t primask) {
    __set_PRIMASK(primask);
}

static uint32_t sch_alloc_task_id(void) {
    uint32_t id = next_task_id++;
    if (next_task_id == NO_TASK_ID) {
        next_task_id = 1;
    }
    return id;
}

static void sch_delete_at_index_unlocked(uint8_t index) {
    if (index >= SCH_count) {
        return;
    }

    /* Preserve relative timing of the following task */
    if (index < (uint8_t)(SCH_count - 1)) {
        SCH_tasks_G[index + 1].Delay += SCH_tasks_G[index].Delay;
    }

    for (uint8_t i = index; i < (uint8_t)(SCH_count - 1); i++) {
        SCH_tasks_G[i] = SCH_tasks_G[i + 1];
    }

    SCH_count--;
    SCH_tasks_G[SCH_count].pTask  = NULL;
    SCH_tasks_G[SCH_count].Delay  = 0;
    SCH_tasks_G[SCH_count].Period = 0;
    SCH_tasks_G[SCH_count].RunMe  = 0;
    SCH_tasks_G[SCH_count].TaskID = NO_TASK_ID;
}

static uint32_t sch_insert_task_unlocked(void (*pFunc)(void), uint32_t DELAY, uint32_t PERIOD, uint32_t taskID) {
    if (pFunc == NULL || SCH_count >= SCH_MAX_TASKS) {
        return NO_TASK_ID;
    }

    uint32_t time_to_run = DELAY;
    uint8_t i = 0;

    while ((i < SCH_count) && (time_to_run >= SCH_tasks_G[i].Delay)) {
        time_to_run -= SCH_tasks_G[i].Delay;
        i++;
    }

    for (uint8_t j = SCH_count; j > i; j--) {
        SCH_tasks_G[j] = SCH_tasks_G[j - 1];
    }

    SCH_tasks_G[i].pTask  = pFunc;
    SCH_tasks_G[i].Delay  = time_to_run;
    SCH_tasks_G[i].Period = PERIOD;
    SCH_tasks_G[i].RunMe  = (i == 0 && time_to_run == 0) ? 1U : 0U;
    SCH_tasks_G[i].TaskID = taskID;

    if (i < SCH_count) {
        SCH_tasks_G[i + 1].Delay -= time_to_run;
    }

    SCH_count++;
    return taskID;
}

void SCH_Init(void) {
    uint32_t primask = sch_enter_critical();
    SCH_count = 0;
    next_task_id = 1;
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        SCH_tasks_G[i].pTask  = NULL;
        SCH_tasks_G[i].Delay  = 0;
        SCH_tasks_G[i].Period = 0;
        SCH_tasks_G[i].RunMe  = 0;
        SCH_tasks_G[i].TaskID = NO_TASK_ID;
    }
    sch_exit_critical(primask);
}

uint32_t SCH_Add_Task(void (*pFunc)(void), uint32_t DELAY, uint32_t PERIOD) {
    uint32_t primask = sch_enter_critical();
    uint32_t id = NO_TASK_ID;
    if (pFunc != NULL && SCH_count < SCH_MAX_TASKS) {
        id = sch_insert_task_unlocked(pFunc, DELAY, PERIOD, sch_alloc_task_id());
    }
    sch_exit_critical(primask);
    return id;
}

uint8_t SCH_Delete_Task(uint32_t taskID) {
    if (taskID == NO_TASK_ID) {
        return 0;
    }

    uint32_t primask = sch_enter_critical();
    uint8_t deleted = 0;
    for (uint8_t i = 0; i < SCH_count; i++) {
        if (SCH_tasks_G[i].TaskID == taskID) {
            sch_delete_at_index_unlocked(i);
            deleted = 1;
            break;
        }
    }
    sch_exit_critical(primask);
    return deleted;
}

/* Called in TIM2 ISR: O(1) */
void SCH_Update(void) {
    if (SCH_count == 0) {
        return;
    }

    if (SCH_tasks_G[0].Delay > 0) {
        SCH_tasks_G[0].Delay--;
    }

    if (SCH_tasks_G[0].Delay == 0) {
        SCH_tasks_G[0].RunMe = 1;
    }
}

void SCH_Dispatch_Tasks(void) {
    while (1) {
        uint32_t primask = sch_enter_critical();
        if (SCH_count == 0 || (SCH_tasks_G[0].Delay > 0 && SCH_tasks_G[0].RunMe == 0)) {
            sch_exit_critical(primask);
            break;
        }

        sTask current = SCH_tasks_G[0];
        sch_delete_at_index_unlocked(0);

        if (current.Period > 0) {
            sch_insert_task_unlocked(current.pTask, current.Period, current.Period, current.TaskID);
        }
        sch_exit_critical(primask);

        if (current.pTask != NULL) {
            (*current.pTask)();
        }
    }
}
