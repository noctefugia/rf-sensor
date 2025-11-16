#include "taskman.h" 

struct taskman_struct taskman = {0};


void Taskman_Init(uint16_t upd_prd, void (*tick_evt)(uint8_t))
{
	uint8_t i;
	
	taskman.update_period = upd_prd;
	taskman.tick_event = tick_evt;
	
	for (i = 0; i < TASK_COUNT; ++i)
		taskman.id[i].enabled = FALSE;
}


void Taskman_Update(uint16_t cur_t)
{
	uint8_t i;
	
	for (i = 0; i < TASK_COUNT; ++i) {
		if ( (taskman.id[i].enabled) && \
		( ((taskman.id[i].looped) && ((cur_t % taskman.id[i].period) == 0)) \
		|| ((!taskman.id[i].looped) && ((taskman.id[i].period--) == 0)) ) ) {
			(*taskman.tick_event)(i);
			if (!taskman.id[i].looped)
				taskman.id[i].enabled = FALSE;
		}
	}
}


void Task_Create(uint8_t tid, uint16_t prd, bool enable, bool loop)
{
	assert_param(IS_TASK_ID_OK(tid));
	
	taskman.id[tid].period = (uint16_t)(prd / taskman.update_period);
	taskman.id[tid].looped = loop ? TRUE : FALSE;
	Task_Enable(tid, enable);
}


void Task_Enable(uint8_t tid, bool enable)
{
	assert_param(IS_TASK_ID_OK(tid));
	
	taskman.id[tid].enabled = enable ? TRUE : FALSE;
}


bool Task_IsEnabled(uint8_t tid)
{
	assert_param(IS_TASK_ID_OK(tid));
	
	return ((taskman.id[tid].enabled) ? TRUE : FALSE);
}