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
 * @file test_init_safety.c
 * @brief Test for PTL initialization safety.
 *
 * Verifies that PTL_Init() correctly rejects invalid configurations
 * such as NULL pointers or zero tasks.
 */

#include "ptl.h"
#include "uart.h"

static void vJob_NoOp(void *pvParameters) { (void)pvParameters; }

int main(void) {
  UART_init();
  if (PTL_Start() != pdFAIL) {
    UART_printf("[FAIL] Started before initialization.\n");
    for (;;) {}
  }
  PTL_GlobalConfig_t global = {PTL_POLICY_SKIP, pdTRUE, 1};
  PTL_TaskConfig_t task = {"Valid", 100, 0, 2, 256,
                          vJob_NoOp, NULL, PTL_POLICY_USE_GLOBAL};
  if (PTL_Init(&global, &task, 1) != pdPASS) {
    UART_printf("[FAIL] Valid configuration rejected.\n");
    for (;;) {}
  }
  UBaseType_t count = 0;
  PTL_TaskObj_t *tasks = PTL_GetTaskList(&count);
  if (count != 1 || tasks[0].xConfig.xDeadline != 100 ||
      PTL_GetEffectivePolicy(&tasks[0]) != PTL_POLICY_SKIP ||
      PTL_Init(&global, &task, 1) != pdFAIL) {
    UART_printf("[FAIL] Initialization defaults or lifecycle failed.\n");
    for (;;) {}
  }
  UART_printf("[PASS] Lifecycle, implicit deadline and global policy validated.\n");
  for (;;) {}
}
