/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/**
 * @file ptl_wrapper.c
 * @brief PTL Task Wrapper and Initialization
 *
 * This file implements the core PTL functionality including:
 * - Generic task wrapper for periodic job execution
 * - System initialization and configuration validation
 * - Runtime state management and statistics collection
 * - Policy resolution for overrun handling
 *
 * @author  Member 2 (Group 38) - Rocco Caliandro
 */

#include "ptl.h"
#include "ptl_trace.h"
#include "ptl_events.h"
#include "uart.h"
#include <string.h>

static PTL_TaskObj_t xTaskPool[PTL_MAX_TASKS];
static UBaseType_t uxRegisteredTaskCount = 0;
static BaseType_t xIsInitialized = pdFALSE;

/* Global config (stored at init) */
static PTL_OverrunPolicy_t eGlobalPolicy = PTL_POLICY_SKIP;
static volatile BaseType_t xTracingEnabled = pdFALSE;
static UBaseType_t uxMaxTasksAllowed = 0;

/* Required for PTL_Start() */
extern BaseType_t PTL_Scheduler_Start(void);

/**
 * @brief Converts a uint32_t value to string.
 *
 * This helper function converts an unsigned 32-bit integer to its decimal
 * string representation without using printf (to minimize dependencies).
 *
 * @param[in]  num  The unsigned integer value to convert.
 * @param[out] buf  Pointer to the output buffer (must be at least 12 bytes).
 */
static void prvUint32ToString(uint32_t num, char* buf)
{
    char temp[12];
    int i = 0;
    
    if (num == 0) {
        buf[0] = '0';
        buf[1] = '\0'; /* null-terminated */
        return;
    }
    
    while (num > 0) {
        temp[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

/**
 * @brief Generic wrapper function for all PTL periodic tasks.
 *
 * This function implements the typical infinite loop pattern required by FreeRTOS tasks.
 * It waits for a notification from the supervisor, executes the user-defined
 * job body, logs start/complete events if tracing is enabled, and tracks
 * job completion statistics.
 *
 * @param[in] pvParameters Pointer to the PTL_TaskObj_t structure for this task.
 *
 * @note This function never returns (infinite loop).
 * @note The user's job body function is called once per period.
 */
void PTL_GenericWrapper(void* pvParameters)
{
    PTL_TaskObj_t* pxTask = (PTL_TaskObj_t*)pvParameters;
    
    configASSERT(pxTask != NULL);
    configASSERT(pxTask->xConfig.pvEntryFunction != NULL);
    
    
    for (;;)
    {
        /* Wait for notification from scheduler */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        /* Mark active, log start */
        pxTask->xIsActive = pdTRUE;
        TickType_t xJobStartTime = xTaskGetTickCount();
        
        if (xTracingEnabled)
        {
            PTL_LogEvent(pxTask->xConfig.pcName, PTL_EVENT_START, xJobStartTime);
        }
        
        /* Execute user job body */
        UART_printf("[PTL] Executing: ");
        UART_printf(pxTask->xConfig.pcName);
        UART_printf("\n");
        pxTask->xConfig.pvEntryFunction(pxTask->xConfig.pvParameters);
        
        /* Record finish, log end */
        TickType_t xJobEndTime = xTaskGetTickCount();
        
        if (xTracingEnabled)
        {
            PTL_LogEvent(pxTask->xConfig.pcName, PTL_EVENT_COMPLETE, xJobEndTime);
        }

         /* Check deadline: F_k > R_k + D → DEADLINE_MISS */
        TickType_t xAbsoluteDeadline = pxTask->xCurrentReleaseTime + 
                                       pxTask->xConfig.xDeadline;

        if ( (xJobEndTime > xAbsoluteDeadline) &&
             (pxTask->xDeadlineMissed == pdFALSE) )
        {
            pxTask->xDeadlineMissed = pdTRUE;
            pxTask->ulDeadlineMisses++;

            if (xTracingEnabled)
            {
                PTL_LogEvent(pxTask->xConfig.pcName, PTL_EVENT_DEADLINE_MISS,
                             xJobEndTime);
            }
        }
        
        /* Mark complete */
        pxTask->xIsActive = pdFALSE;
        pxTask->ulJobsCompleted++;
    }
}

/**
 * @brief Initializes the Periodic Task Layer (PTL) system.
 *
 * This function validates the provided configuration, stores global settings,
 * initializes the trace system if enabled, and creates FreeRTOS tasks for
 * each periodic task using the generic wrapper.
 *
 * @param[in] pxGlobalConfig  Pointer to the global PTL configuration structure.
 * @param[in] pxTaskConfigs   Array of per-task configuration structures.
 * @param[in] uxTaskCount     Number of tasks in the pxTaskConfigs array.
 *
 * @return pdPASS if initialization succeeded, pdFAIL otherwise.
 *
 * @note Must be called before PTL_Start().
 * @note If a task's deadline is set to 0, it defaults to the period value.
 * @note This function can only be called once; subsequent calls will fail.
 */
BaseType_t PTL_Init(const PTL_GlobalConfig_t* pxGlobalConfig,
                    const PTL_TaskConfig_t* pxTaskConfigs, 
                    UBaseType_t uxTaskCount)
{
    char numBuf[12];
    
    /* Input validation */
    if (pxGlobalConfig == NULL || pxTaskConfigs == NULL)
    {
        UART_printf("[PTL] Error: NULL config\n");
        return pdFAIL;
    }
    
    if (uxTaskCount == 0 || uxTaskCount > PTL_MAX_TASKS)
    {
        UART_printf("[PTL] Error: Invalid task count\n");
        return pdFAIL;
    }
    
    if (uxTaskCount > pxGlobalConfig->uxMaxTasks)
    {
        UART_printf("[PTL] Error: Exceeds max_tasks\n");
        return pdFAIL;
    }
    
    if (xIsInitialized == pdTRUE)
    {
        UART_printf("[PTL] Error: Already initialized\n");
        return pdFAIL;
    }
    
    /* Validate the complete configuration before creating any worker. */
    if (pxGlobalConfig->uxMaxTasks == 0 ||
        pxGlobalConfig->uxMaxTasks > PTL_MAX_TASKS ||
        pxGlobalConfig->eOverrunPolicy < PTL_POLICY_SKIP ||
        pxGlobalConfig->eOverrunPolicy > PTL_POLICY_CATCH_UP)
    {
        UART_printf("[PTL] Error: Invalid global configuration\n");
        return pdFAIL;
    }

    for (UBaseType_t i = 0; i < uxTaskCount; i++)
    {
        const PTL_TaskConfig_t* pxConfig = &pxTaskConfigs[i];
        if (pxConfig->pcName == NULL || pxConfig->pcName[0] == '\0' ||
            strlen(pxConfig->pcName) >= PTL_TASK_NAME_MAX_LEN ||
            pxConfig->pvEntryFunction == NULL || pxConfig->xPeriod == 0 ||
            pxConfig->xDeadline > pxConfig->xPeriod ||
            pxConfig->uxPriority >= (configMAX_PRIORITIES - 1) ||
            pxConfig->usStackDepth < configMINIMAL_STACK_SIZE ||
            pxConfig->eOverrunPolicy < PTL_POLICY_USE_GLOBAL ||
            pxConfig->eOverrunPolicy > PTL_POLICY_CATCH_UP)
        {
            UART_printf("[PTL] Error: Invalid task configuration\n");
            return pdFAIL;
        }
    }

    /* Store global config */
    eGlobalPolicy = pxGlobalConfig->eOverrunPolicy;
    xTracingEnabled = pxGlobalConfig->xTracingEnabled;
    uxMaxTasksAllowed = pxGlobalConfig->uxMaxTasks;
    
    /* Initialize trace system if enabled */
    if (xTracingEnabled)
    {
        PTL_TraceInit();
    }
    
    UART_printf("[PTL] Initializing ");
    prvUint32ToString(uxTaskCount, numBuf);
    UART_printf(numBuf);
    UART_printf(" tasks...\n");
    
    /* Create wrapper tasks */
    for (UBaseType_t i = 0; i < uxTaskCount; i++)
    {
        PTL_TaskObj_t* pxTask = &xTaskPool[i];
        const PTL_TaskConfig_t* pxConfig = &pxTaskConfigs[i];
        
        if (pxConfig->pvEntryFunction == NULL)
        {
            UART_printf("[PTL] Error: NULL entry function\n");
            return pdFAIL;
        }
        
        /* Copy configuration */
        memcpy(&pxTask->xConfig, pxConfig, sizeof(PTL_TaskConfig_t));
        
        /* If deadline=0, use period */
        if (pxTask->xConfig.xDeadline == 0)
        {
            pxTask->xConfig.xDeadline = pxTask->xConfig.xPeriod;
        }
        
        /* Initialize runtime state */
        pxTask->xTaskHandle         = NULL;
        pxTask->xNextReleaseTime    = 0;  
        pxTask->xCurrentReleaseTime = 0;
        pxTask->xIsActive           = pdFALSE;
        pxTask->xDeadlineMissed     = pdFALSE;
        pxTask->ulJobsCompleted     = 0;
        pxTask->ulDeadlineMisses    = 0;
        pxTask->ulOverrunKills      = 0;
        pxTask->ulOverrunCatchUps   = 0;
        pxTask->ulOverrunSkips      = 0;
        
        /* Create FreeRTOS task */
        BaseType_t xResult = xTaskCreate(
            PTL_GenericWrapper,
            pxConfig->pcName,
            pxConfig->usStackDepth,
            (void*)pxTask,
            pxConfig->uxPriority,
            &pxTask->xTaskHandle
        );
        
        if (xResult != pdPASS)
        {
            UART_printf("[PTL] Error: Task creation failed\n");
            for (UBaseType_t j = 0; j < i; j++)
            {
                vTaskDelete(xTaskPool[j].xTaskHandle);
                xTaskPool[j].xTaskHandle = NULL;
            }
            return pdFAIL;
        }
        
        UART_printf("[PTL] Created: ");
        UART_printf(pxConfig->pcName);
        UART_printf("\n");
    }
    
    uxRegisteredTaskCount = uxTaskCount;
    xIsInitialized = pdTRUE;
    
    UART_printf("[PTL] Init complete\n");
    return pdPASS;
}

/**
 * @brief Starts the PTL scheduler and begins periodic task execution.
 *
 * This function starts the supervisor task which manages job releases,
 * deadline checks, and overrun handling. It must be called after a
 * successful PTL_Init() call.
 *
 * @return pdPASS if the scheduler started successfully, pdFAIL otherwise.
 *
 * @note This function starts the FreeRTOS scheduler and does not return
 *       under normal operation.
 * @note PTL_Init() must be called successfully before calling this function.
 */
BaseType_t PTL_Start(void)
{
    if (xIsInitialized != pdTRUE)
    {
        UART_printf("[PTL] Error: Not initialized\n");
        return pdFAIL;
    }
    
    UART_printf("[PTL] Starting dispatcher...\n");
    return PTL_Scheduler_Start();
}

/**
 * @brief Checks if the PTL trace system is enabled.
 *
 * @return pdTRUE if tracing is enabled, pdFALSE otherwise.
 */
BaseType_t PTL_IsTracingEnabled(void)
{
    return xTracingEnabled;
}

/**
 * @brief Gets the global overrun policy.
 *
 * @return The global PTL_OverrunPolicy_t value configured during PTL_Init().
 */
PTL_OverrunPolicy_t PTL_GetPolicy(void)
{
    return eGlobalPolicy;
}

/**
 * @brief Gets the effective overrun policy for a specific task.
 *
 * Returns the per-task policy if explicitly set, otherwise falls back
 * to the global policy.
 *
 * @param[in] pxTask Pointer to the task object to query.
 *
 * @return The effective PTL_OverrunPolicy_t for the specified task.
 *         Returns the global policy if pxTask is NULL.
 */
PTL_OverrunPolicy_t PTL_GetEffectivePolicy(const PTL_TaskObj_t* pxTask)
{
    if (pxTask == NULL)
    {
        return eGlobalPolicy;
    }
    
    /* If per-task policy is set (not USE_GLOBAL), use it. */
    if (pxTask->xConfig.eOverrunPolicy != PTL_POLICY_USE_GLOBAL)
    {
        return pxTask->xConfig.eOverrunPolicy;
    }
    
    /* Otherwise fall back to global policy. */
    return eGlobalPolicy;
}

/**
 * @brief Retrieves runtime statistics for a specific periodic task.
 *
 * This function provides thread-safe access to task execution statistics
 * including completed jobs, deadline misses, and total overruns.
 *
 * @param[in]  uxTaskIndex Index of the task (0 to registered count - 1).
 * @param[out] pulJobs     Pointer to store total completed jobs (can be NULL).
 * @param[out] pulMisses   Pointer to store total deadline misses (can be NULL).
 * @param[out] pulOverruns Pointer to store total overruns (can be NULL).
 *
 * @note This function uses critical sections for thread safety.
 * @note If uxTaskIndex is out of range, the function returns without action.
 */
void PTL_GetTaskStats(UBaseType_t uxTaskIndex, uint32_t* pulJobs, 
                      uint32_t* pulMisses, uint32_t* pulOverruns)
{
    if (uxTaskIndex >= uxRegisteredTaskCount)
    {
        return;
    }
    
    PTL_TaskObj_t* pxTask = &xTaskPool[uxTaskIndex];
    
    taskENTER_CRITICAL();
    {
        if (pulJobs != NULL)
        {
            *pulJobs = pxTask->ulJobsCompleted;
        }
        if (pulMisses != NULL)
        {
            *pulMisses = pxTask->ulDeadlineMisses;
        }
        if (pulOverruns != NULL)
        {
            *pulOverruns = pxTask->ulOverrunSkips + 
                           pxTask->ulOverrunKills + 
                           pxTask->ulOverrunCatchUps;
        }
    }
    taskEXIT_CRITICAL();
}

/**
 * @brief Gets a pointer to the internal task pool array.
 *
 * This function provides direct access to the PTL task pool for
 * advanced monitoring or debugging purposes.
 *
 * @param[out] puxCount Pointer to store the number of registered tasks
 *                      (can be NULL if count is not needed).
 *
 * @return Pointer to the first element of the task pool array.
 *
 * @warning The returned pointer provides direct access to internal state.
 *          Modifications should be avoided unless absolutely necessary.
 */
PTL_TaskObj_t* PTL_GetTaskList(UBaseType_t* puxCount)
{
    if (puxCount != NULL)
    {
        *puxCount = uxRegisteredTaskCount;
    }
    
    return xTaskPool;
}