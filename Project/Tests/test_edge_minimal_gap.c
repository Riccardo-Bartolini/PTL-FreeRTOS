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

/**
 * @file test_edge_minimal_gap.c
 * @brief Stress Test: Minimal Workload Gaps.
 * @author Member 4 (Davide) Group 38
 *
 * Tests the scheduler's behavior when tasks use almost 100% of their
 * slice (8ms work in 10ms period), checking for jitter-induced misses.
 */

#include "burner.h"
#include "ptl.h"
#include "ptl_trace.h"
#include "uart.h"

static void vJob_Tight(void *pvParameters) {
  (void)pvParameters;
  Burn(10); /* 10ms work in 10ms period = 100% load */
}

static void vJob_Check(void *pvParameters) {
  (void)pvParameters;
  PTL_TraceStats_t s;

  vTaskDelay(pdMS_TO_TICKS(500));

  PTL_GetTraceStatistics(&s);

  UART_printf("\n=== TEST: MINIMAL TIME GAPS ===\n");
  PTL_PrintStatistics();

  /* Success if no overruns occur despite the tight timing */
  if ((s.ulOverrunCount == 0) && (s.ulTotalCompletions > 10)) {
    UART_printf("[PASS] Timing consistency maintained.\n");
  } else {
    UART_printf("[FAIL] Jitter caused overruns.\n");
  }

  for (;;) {
    /* Trap */
  }
}

static PTL_TaskConfig_t xTaskConfig[] = {
    {"Tight", pdMS_TO_TICKS(10), pdMS_TO_TICKS(10), 2, 512, vJob_Tight, NULL, PTL_POLICY_CATCH_UP},
    {"REF", pdMS_TO_TICKS(600), pdMS_TO_TICKS(600), 3, 512, vJob_Check, NULL, PTL_POLICY_USE_GLOBAL}};

int main(void) {
  PTL_GlobalConfig_t xGlobalConfig = {PTL_POLICY_CATCH_UP, pdTRUE, 2};

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