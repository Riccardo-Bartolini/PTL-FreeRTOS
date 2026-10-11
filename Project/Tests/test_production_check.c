/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */


/** * @file test_production_check.c
 * @brief Production Check: Trace-Based Validation with robust timing.
 * @author Member 4 (Davide) Group 38
 *
 */
 
#include "burner.h"
#include "ptl.h"
#include "ptl_trace.h"
#include "uart.h"
#include <stdio.h>

/* We only need a few jobs to calculate stable statistics */
static volatile int iJobsCompleted = 0;
#define TOTAL_TEST_JOBS 6

static void vJob_Stress_Worker(void *pvParameters) {
  (void)pvParameters;

  /* 60ms work in 100ms period = 60% load per task */
  Burn(60);

  iJobsCompleted++;

  /* The last job triggers the Trace Report */
  if (iJobsCompleted == TOTAL_TEST_JOBS) {
    PTL_TraceStats_t xStats;
    char pcBuffer[128];


    vTaskSuspendAll();

    PTL_GetTraceStatistics(&xStats);

    /* Overhead Calculation logic:
     * User Work = 6 jobs * 60ms = 360ms.
     * Active Time = Total Time - Idle Time.
     * System Overhead = Active Time - User Work.
     */
    uint32_t ulTotalTime = xStats.ulTotalTimeMs;
    uint32_t ulIdleTime = xStats.ulIdleTimeMs;
    uint32_t ulActiveTime =
        (ulTotalTime > ulIdleTime) ? (ulTotalTime - ulIdleTime) : 0;
    uint32_t ulUserTime = 360;

    uint32_t ulSysOverhead = 0;
    if (ulActiveTime > ulUserTime) {
      ulSysOverhead = ulActiveTime - ulUserTime;
    }

    uint32_t ulOverheadPct = 0;
    if (ulTotalTime > 0) {
      ulOverheadPct = (ulSysOverhead * 10000) / ulTotalTime;
    }

    snprintf(pcBuffer, sizeof(pcBuffer),
             "Stats: Total=%u ms, Idle=%u ms, Active=%u ms, User=%u ms\n",
             (unsigned int)ulTotalTime, (unsigned int)ulIdleTime,
             (unsigned int)ulActiveTime, (unsigned int)ulUserTime);
    UART_printf(pcBuffer);

    snprintf(pcBuffer, sizeof(pcBuffer), "System Overhead: %u.%02u%%\n",
             (unsigned int)(ulOverheadPct / 100),
             (unsigned int)(ulOverheadPct % 100));
    UART_printf(pcBuffer);

    PTL_PrintTrace();
    PTL_PrintStatistics();

    /* Success check: Overhead must be <= 10.00% */
    if (ulOverheadPct <= 1000) /* 1000 = 10.00% */
    {
      UART_printf("[PASS] Overhead within limits.\n");
    } else {
      UART_printf("[FAIL] Overhead exceeded 10%.\n");
    }

   

    for (;;) {
      /* Trap */
    }
  }
}

static PTL_TaskConfig_t xTaskConfig[] = {
    {"Worker_A", pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), 2, 512, vJob_Stress_Worker, NULL, PTL_POLICY_SKIP},
    {"Worker_B", pdMS_TO_TICKS(100), pdMS_TO_TICKS(100), 2, 512, vJob_Stress_Worker, NULL, PTL_POLICY_SKIP}};

static PTL_GlobalConfig_t xGlobalConfig = {PTL_POLICY_SKIP, pdTRUE, 2};

int main(void) {
  UART_init();
  PTL_TraceInit();

  if (PTL_Init(&xGlobalConfig, xTaskConfig, 2) == pdPASS) {
    PTL_Start();
  }

  for (;;) {
    /* Trap */
  }
  return 0;
}