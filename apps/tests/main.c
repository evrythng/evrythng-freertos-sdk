/*
 * (c) Copyright 2012 EVRYTHNG Ltd London / Zurich
 * www.evrythng.com
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"

#include "evrythng/platform.h"

#include "tests.h"

void vApplicationIdleHook(void)
{
    sleep(1);
}

void task()
{
    RunAllTests();
    vTaskEndScheduler();
}

int main(void)
{
    xTaskCreate(task, "tests", 1024, 0, 0, NULL);
    vTaskStartScheduler();

    return 0;
}
