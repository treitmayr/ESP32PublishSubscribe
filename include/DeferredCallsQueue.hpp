/**
 * @file DeferredCallsQueue.hpp
 * @author Thomas Reitmayr (treitmayr@devbase.at)
 * @brief Execute functions in a deferred and asynchronous way
 *
 * @copyright Copyright (c) 2023 Thomas Reitmayr
 * MIT License
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <mutex>
#include <esp_task.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

class DeferredCallsQueue
{
public:
    using CallType = std::function<void()>;

    inline static const UBaseType_t itsQueueSize = 20;
    inline static const BaseType_t itsCurrentAffinity = tskNO_AFFINITY - 1;

    /**
     * @brief Returns a PubSub instance, use this to instantiate the PubSub object.
     * @return PublishSubscribe&
     */
    static DeferredCallsQueue& getInstance();

    /**
     * @brief Alias for getInstance().
     *
     * @return DeferredCallsQueue&
     */
    inline static constexpr auto get = &getInstance;

    /**
     * @brief Adds a call to be deferred to a task with the given priority
     *        and core affinity.
     *
     * @param p_call the function to call
     * @param p_priority the priority with which to execte the function (default: main priority)
     * @param p_core_id the core where to execute the function (default: current task's setting)
     */
    void addDeferredCall(const CallType& p_call, UBaseType_t p_priority = ESP_TASK_MAIN_PRIO,
                         BaseType_t p_core_id = itsCurrentAffinity);

private:
    std::mutex itsQueueListMutex;
    std::unordered_map<uint32_t, QueueHandle_t> itsQueueList;

    DeferredCallsQueue();

    QueueHandle_t getQueueList(UBaseType_t p_priority, BaseType_t p_core_id);
    char coreToChar(BaseType_t p_core_id) const;

    void callerTask(QueueHandle_t p_queue);
    static void callerTaskWrapper(void* pvParameter);
};
