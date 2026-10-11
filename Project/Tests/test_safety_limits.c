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
 * @file test_safety_limits.c
 * @brief Extended safety checks for PTL initialization.
 * @author Member 4 (Davide) Group 38
 *
 * Verifies that PTL_Init() correctly rejects invalid configurations
 * such as NULL pointers or zero tasks.
 */

#include "ptl.h"
#include "uart.h"

static void vJob_NoOp(void *pvParameters) { (void)pvParameters; }
static unsigned int failures;

static void check_rejected(const PTL_GlobalConfig_t *global,
                           const PTL_TaskConfig_t *tasks, UBaseType_t count) {
  if (PTL_Init(global, tasks, count) != pdFAIL) {
    failures++;
    UART_printf("[FAIL] Invalid configuration accepted.\n");
  }
}

int main(void) {
  UART_init();
  PTL_GlobalConfig_t global = {PTL_POLICY_SKIP, pdTRUE, 2};
  const PTL_TaskConfig_t valid = {"Valid", 100, 100, 2, 256,
                                  vJob_NoOp, NULL, PTL_POLICY_SKIP};
  PTL_TaskConfig_t tasks[2] = {valid, valid};
  check_rejected(NULL, tasks, 1);
  check_rejected(&global, NULL, 1);
  check_rejected(&global, tasks, 0);
  check_rejected(&global, tasks, PTL_MAX_TASKS + 1);
  global.uxMaxTasks = 1;
  check_rejected(&global, tasks, 2);
  global.uxMaxTasks = PTL_MAX_TASKS + 1;
  check_rejected(&global, tasks, 1);
  global.uxMaxTasks = 0;
  check_rejected(&global, tasks, 1);
  global.uxMaxTasks = 2;
  global.eOverrunPolicy = PTL_POLICY_USE_GLOBAL;
  check_rejected(&global, tasks, 1);
  global.eOverrunPolicy = (PTL_OverrunPolicy_t)99;
  check_rejected(&global, tasks, 1);
  global.eOverrunPolicy = PTL_POLICY_SKIP;

  tasks[0].pvEntryFunction = NULL;
  check_rejected(&global, tasks, 1);
  tasks[0] = valid; tasks[0].xPeriod = 0;
  check_rejected(&global, tasks, 1);
  tasks[0] = valid; tasks[0].xDeadline = 101;
  check_rejected(&global, tasks, 1);
  tasks[0] = valid; tasks[0].uxPriority = configMAX_PRIORITIES - 1;
  check_rejected(&global, tasks, 1);
  tasks[0] = valid; tasks[0].usStackDepth = 0;
  check_rejected(&global, tasks, 1);
  tasks[0] = valid; tasks[0].pcName = NULL;
  check_rejected(&global, tasks, 1);
  tasks[0] = valid; tasks[0].pcName = "";
  check_rejected(&global, tasks, 1);
  tasks[0] = valid; tasks[0].pcName = "NameTooLongForPTL";
  check_rejected(&global, tasks, 1);
  tasks[0] = valid; tasks[0].eOverrunPolicy = (PTL_OverrunPolicy_t)99;
  check_rejected(&global, tasks, 1);

  /* An invalid second task must not create an orphan first worker. */
  tasks[0] = valid; tasks[1] = valid; tasks[1].pvEntryFunction = NULL;
  check_rejected(&global, tasks, 2);
  tasks[1] = valid;
  if (PTL_Init(&global, tasks, 2) != pdPASS) {
    failures++;
    UART_printf("[FAIL] Rejected configurations corrupted initialization.\n");
  }
  if (failures == 0) {
    UART_printf("[PASS] Invalid inputs rejected; valid retry succeeded.\n");
  }
  for (;;) {}
}
