/**
 * @file DeferredCallsQueue.cpp
 * @author Thomas Reitmayr (treitmayr@devbase.at)
 * @brief Execute functions in a deferred and asynchronous way
 *
 * @copyright Copyright (c) 2023 Thomas Reitmayr
 * MIT License
 */


#include <stdio.h>
#include <cstring>
#include "DeferredCallsQueue.hpp"
#include <esp_log.h>
#include <esp_err.h>

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

static const char TAG[] = "DeferredCallsQueue";


#define OS_ERROR_CHECK(cmd, format, ...)                                                                 \
    do                                                                                                   \
    {                                                                                                    \
        if (unlikely((cmd) != pdPASS))                                                                   \
        {                                                                                                \
            ESP_LOGE(TAG, "%s (%s:%d): - " format, __FUNCTION__, __FILENAME__, __LINE__, ## __VA_ARGS__); \
            ESP_ERROR_CHECK(ESP_FAIL);                                                                   \
        }                                                                                                \
    } while (0)


DeferredCallsQueue &DeferredCallsQueue::getInstance()
{
    static DeferredCallsQueue instance;
    return instance;
}


void DeferredCallsQueue::addDeferredCall(const CallType& p_call, UBaseType_t p_priority, BaseType_t p_core_id)
{
    // perform a clean copy of the function pointer
    CallType* callCopy = new CallType(p_call);
    void* queueEntry = static_cast<void*>(callCopy);

    UBaseType_t coreId = (p_core_id == itsCurrentAffinity) ? xTaskGetAffinity(NULL) : p_core_id;

    QueueHandle_t queue = getQueueList(p_priority, coreId);
    //ESP_LOGI(TAG, "Queue entries (p%dc%d): %d", p_priority, p_core_id, uxQueueMessagesWaiting(queue));
    OS_ERROR_CHECK(xQueueSend(queue, &queueEntry, 5000 / portTICK_PERIOD_MS),
                   "Cannot add entry to deferred calls queue");
}


DeferredCallsQueue::DeferredCallsQueue()
{}


QueueHandle_t DeferredCallsQueue::getQueueList(UBaseType_t p_priority, BaseType_t p_core_id)
{
    QueueHandle_t queue = nullptr;
    bool newEntry = false;
    const uint32_t key = ((p_priority & 0xffff) << 16) | (p_core_id & 0xffff);

    itsQueueListMutex.lock();

    auto it = itsQueueList.find(key);
    if (likely(it != itsQueueList.end()))
    {
        queue = it->second;
    }
    else
    {
        // create new queue
        queue = xQueueCreate(itsQueueSize, sizeof(CallType*));
        itsQueueList[key] = queue;
        newEntry = true;
    }

    itsQueueListMutex.unlock();

    if (unlikely(newEntry))
    {
        // create associated task with a significant name
        char taskName[30];
        snprintf(taskName, sizeof(taskName), "DefCalls-p%dc%c",
                 p_priority, coreToChar(p_core_id));
        //ESP_LOGI(TAG, "Creating new task '%s'", taskName);
        OS_ERROR_CHECK(xTaskCreatePinnedToCore(callerTaskWrapper, taskName, 4096 * 2,
                                               static_cast<void*>(queue), p_priority, NULL, p_core_id),
                       "Cannot create calls queue for priority %d, core %d", p_priority, p_core_id);
    }
    return queue;
}


char DeferredCallsQueue::coreToChar(BaseType_t p_core_id) const
{
    char coreChr = '?';
    if (p_core_id == tskNO_AFFINITY)
    {
        coreChr = '*';
    }
    else if ((p_core_id >= 0) && (p_core_id < 10))
    {
        coreChr = '0' + p_core_id;
    }
    return coreChr;
}


void DeferredCallsQueue::callerTask(QueueHandle_t p_queue)
{
    while (true)
    {
        CallType* functionToCall;
        //ESP_LOGI(TAG, "%s: Waiting for next call (queue size = %d)", pcTaskGetName(nullptr), uxQueueMessagesWaiting(p_queue));;
        if (likely(xQueueReceive(p_queue, &functionToCall, portMAX_DELAY) == pdPASS))
        {
            //ESP_LOGI(TAG, "%s: Invoking next call (queue size = %d)", pcTaskGetName(nullptr), uxQueueMessagesWaiting(p_queue));;
            (*functionToCall)();
            delete functionToCall;
        }
        else
        {
            ESP_LOGW(TAG, "%s: Error waiting for queue entry (queue size = %d)", pcTaskGetName(nullptr), uxQueueMessagesWaiting(p_queue));;
        }
        vTaskDelay(0);
    }
}


void DeferredCallsQueue::callerTaskWrapper(void* pvParameter)
{
    QueueHandle_t queue = static_cast<QueueHandle_t>(pvParameter);
    DeferredCallsQueue::get().callerTask(queue);
}
