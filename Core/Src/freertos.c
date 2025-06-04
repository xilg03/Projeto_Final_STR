#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void);

void MX_FREERTOS_Init(void) {

  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

}

void StartDefaultTask(void *argument){
  for(;;){
    osDelay(1);
  }
}


