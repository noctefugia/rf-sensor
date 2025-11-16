#ifndef __TASKMAN_H
#define __TASKMAN_H

#include "utils.h"

#define TASK_COUNT 3
#define IS_TASK_ID_OK(id) \
	((id) < (TASK_COUNT)) 

struct task_id_struct {
	uint16_t period : 14;
	uint16_t enabled : 1, looped : 1;
};

struct taskman_struct {
	struct task_id_struct id[TASK_COUNT];
	void (*tick_event)(uint8_t);
	uint16_t update_period;
};

void Taskman_Init(uint16_t upd_prd, void (*tick_evt)(uint8_t));
void Taskman_Update(uint16_t cur_t);
void Task_Create(uint8_t tid, uint16_t prd, bool enable, bool loop);
void Task_Enable(uint8_t tid, bool enable);
bool Task_IsEnabled(uint8_t tid);

#endif /* __TASKMAN_H */