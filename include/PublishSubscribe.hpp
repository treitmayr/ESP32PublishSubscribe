/**
 * @file PublishSubscribe.hpp
 * @author Thomas Reitmayr (treitmayr@devbase.at)
 * @brief Publish/Subscribe Pattern Implementation for the ESP32 microcontroller
 * @details
 * This header-only library implements the publish/subscribe pattern specifically
 * for the ESP32 microcontroller. Its main features are:
 *   * Thread-safe:
 *     Messages may be published and subscribed from multiple threads concurrently.
 *   * Reentrant:
 *     Messages may be published and new subscriptions may be requested from
 *     within subscription handlers.
 *   * Variable-length argument lists:
 *     Argument lists may be designed according to the needs of a specific
 *     event type.
 *   * Asynchronous and synchronous publishing and subscriptions
 *   * Flexible task priority for asynchronous operations, default to the task
 *     priority of the current task.
 *
 * @copyright Copyright (c) 2023 Thomas Reitmayr
 * MIT License
 *
 * @note
 * This library is originally based on work from https://github.com/nbdy/pubsupp
 * which is Copyright (c) 2021 Pascal Eberlein
 * MIT License
 */

#pragma once

#include <map>
#include <mutex>
#include <shared_mutex>
#include <queue>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <limits>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <esp_log.h>
#include <esp_err.h>

#include "DeferredCallsQueue.hpp"

/**
 * @brief Publish/Subscribe library for inter-class communication
 */

template <typename... Types>
class PublishSubscribe
{
public:
    using SubscribeCallback = std::function<void(Types...)> const;

    /**
     * @brief Returns a PubSub instance, use this to instantiate the PubSub object
     *
     * @return PublishSubscribe&
     */
    static PublishSubscribe &getInstance()
    {
        static PublishSubscribe<Types...> instance;
        return instance;
    }

    /**
     * @brief Alias for getInstance().
     *
     * @return PublishSubscribe&
     */
    static constexpr auto get = &getInstance;

    /**
     * @brief Publish a message to a specific channel
     *
     * @param p_channel
     * @param p_args
     */
    void publish(const std::string& p_channel, Types... p_args)
    {
        if (itsPubSubMutex.try_lock_shared())
        {
            publishUnguarded(p_channel, p_args...);
            itsPubSubMutex.unlock_shared();
            runQueuedCalls();
        }
        else
        {
            std::lock_guard<std::mutex> lock(itsDefCallsMutex);
            itsRecursiveCallsQueue.emplace([this, p_channel, p_args...]()
                                           { publish(p_channel, p_args...); });
        }
    }

    void publishAsync(const std::string& p_channel, Types... p_args)
    {
        if (itsPubSubMutex.try_lock_shared())
        {
            publishAsyncUnguarded(p_channel, p_args...);
            itsPubSubMutex.unlock_shared();
            runQueuedCalls();
        }
        else
        {
            std::lock_guard<std::mutex> lock(itsDefCallsMutex);
            itsRecursiveCallsQueue.emplace([this, p_channel, p_args...]()
                                           { publishAsyncUnguarded(p_channel, p_args...); });
        }
    }

    void publishAsyncWithPrio(const std::string& p_channel, Types... p_args, UBaseType_t p_priority)
    {
        if (itsPubSubMutex.try_lock_shared())
        {
            publishAsyncUnguarded(p_channel, p_args..., p_priority);
            itsPubSubMutex.unlock_shared();
            runQueuedCalls();
        }
        else
        {
            std::lock_guard<std::mutex> lock(itsDefCallsMutex);
            itsRecursiveCallsQueue.emplace([this, p_channel, p_priority, p_args...]()
                                           { publishAsyncUnguarded(p_channel, p_args..., p_priority); });
        }
    }

    /**
     * @brief Subscribe to a specific channel
     *
     * @param p_channel
     * @param p_callback
     * @return callback name, should be stored if you wanna unsubscribe
     */
    inline std::string subscribeSync(const std::string& p_channel, SubscribeCallback& p_callback)
    {
        UBaseType_t priority = uxTaskPriorityGet(NULL);
        BaseType_t affinity = xTaskGetAffinity(NULL);
        return subscribe(p_channel, p_callback, priority, affinity, false);
    }

    inline std::string subscribeAsync(const std::string& p_channel, SubscribeCallback& p_callback)
    {
        UBaseType_t priority = uxTaskPriorityGet(NULL);
        BaseType_t affinity = xTaskGetAffinity(NULL);
        return subscribe(p_channel, p_callback, priority, affinity, true);
    }

    inline std::string subscribeAsyncWithPrio(const std::string& p_channel, SubscribeCallback& p_callback,
                                              UBaseType_t p_priority)
    {
        BaseType_t affinity = xTaskGetAffinity(NULL);
        return subscribe(p_channel, p_callback, p_priority, affinity, true);
    }

    /**
     * @brief Subscribe to a topic
     *
     * @param p_channel
     * @param p_callbackName
     * @param p_callback
     */
    inline void subscribeSync(const std::string& p_channel, const std::string& p_callbackName,
                              SubscribeCallback& p_callback)
    {
        UBaseType_t priority = uxTaskPriorityGet(NULL);
        BaseType_t affinity = xTaskGetAffinity(NULL);
        subscribe(p_channel, p_callbackName, p_callback, priority, affinity, false);
    }

    inline void subscribeAsync(const std::string& p_channel, const std::string& p_callbackName,
                               SubscribeCallback& p_callback)
    {
        UBaseType_t priority = uxTaskPriorityGet(NULL);
        BaseType_t affinity = xTaskGetAffinity(NULL);
        subscribe(p_channel, p_callbackName, p_callback, priority, affinity, true);
    }

    /**
     * @brief Unsubscribe from a topic
     *
     * @param p_channel
     * @param p_callbackName
     */
    void unsubscribe(const std::string& p_channel, const std::string& p_callbackName)
    {
        if (itsPubSubMutex.try_lock())
        {
            unsubscribeUnguarded(p_channel, p_callbackName);
            itsPubSubMutex.unlock();
            runQueuedCalls();
        }
        else
        {
            std::lock_guard<std::mutex> lock(itsDefCallsMutex);
            itsRecursiveCallsQueue.emplace([this, p_channel, p_callbackName]()
                                           { unsubscribeUnguarded(p_channel, p_callbackName); });
        }
    }

    /**
     * @brief Remove all callbacks for a topic
     *
     * @param channel
     */
    void clear(const std::string& p_channel)
    {
        if (itsPubSubMutex.try_lock())
        {
            clearUnguarded(p_channel);
            itsPubSubMutex.unlock();
            runQueuedCalls();
        }
        else
        {
            std::lock_guard<std::mutex> lock(itsDefCallsMutex);
            itsRecursiveCallsQueue.emplace([this, p_channel]()
                                           { clear(p_channel); });
        }
    }

    /**
     * @brief Remove all callbacks.
     */
    void clear()
    {
        if (itsPubSubMutex.try_lock())
        {
            clearUnguarded();
            itsPubSubMutex.unlock();
            runQueuedCalls();
        }
        else
        {
            std::lock_guard<std::mutex> lock(itsDefCallsMutex);
            itsRecursiveCallsQueue.emplace([this]()
                                           { clear(); });
        }
    }

private:
    struct MapEntryType
    {
        const SubscribeCallback itsCallback;
        const UBaseType_t itsPriority;
        const BaseType_t itsAffinity;
        const bool itsAlwaysAsync;

        MapEntryType(SubscribeCallback& p_callback,
                     UBaseType_t p_priority,
                     BaseType_t p_affinity,
                     bool p_alwaysAsync) :
            itsCallback(p_callback),
            itsPriority(p_priority),
            itsAffinity(p_affinity),
            itsAlwaysAsync(p_alwaysAsync)
        {}
    };

    using CallbackMap = std::map<std::string, MapEntryType>;
    using SubscriptionMap = std::map<std::string, CallbackMap>;
    using RecursiveCalls = std::function<void()>;

    inline static const char TAG[] = "PubSub";

    std::shared_mutex itsPubSubMutex;
    std::mutex itsDefCallsMutex;
    SubscriptionMap itsSubscriptions;
    std::queue<RecursiveCalls> itsRecursiveCallsQueue;
    bool itsInRecursion;

    /**
     * @brief Private constructor enforcing singleton pattern
     */
    PublishSubscribe()
    {
    }

    /**
     * @brief Publish a message to a specific channel
     *
     * @param p_channel
     * @param p_args
     */
    void publishUnguarded(const std::string& p_channel, Types... p_args)
    {
        ESP_LOGI(TAG, "Publishing '%s'", p_channel.c_str());
        if (itsSubscriptions.contains(p_channel)) {
            for (const auto& kv : itsSubscriptions[p_channel])
            {
                if (kv.second.itsAlwaysAsync)
                {
                    ESP_LOGI(TAG, "  ~> %s", kv.first.c_str());
                    auto& callback = kv.second.itsCallback;
                    DeferredCallsQueue::get().addDeferredCall([callback, p_args...]()
                                                              { callback(p_args...); },
                                                              kv.second.itsPriority,
                                                              kv.second.itsAffinity);
                }
                else
                {
                    ESP_LOGI(TAG, "  -> %s", kv.first.c_str());
                    kv.second.itsCallback(p_args...);
                }
            }
        }
    }

    void publishAsyncUnguarded(const std::string& p_channel, Types... p_args, int p_prio = -1)
    {
        ESP_LOGI(TAG, "Publishing '%s'", p_channel.c_str());
        if (itsSubscriptions.contains(p_channel)) {
            for (const auto& kv : itsSubscriptions[p_channel])
            {
                ESP_LOGI(TAG, "  ~> %s", kv.first.c_str());
                auto& callback = kv.second.itsCallback;
                DeferredCallsQueue::get().addDeferredCall([callback, p_args...]()
                                                          { callback(p_args...); },
                                                          (p_prio < 0) ? kv.second.itsPriority : p_prio,
                                                          kv.second.itsAffinity);
            }
        }
    }

    std::string subscribe(const std::string& p_channel,
                          SubscribeCallback& p_callback,
                          UBaseType_t p_priority,
                          BaseType_t p_affinity,
                          bool p_alwaysAsync)
    {
        auto callbackName = generateRandomString();
        subscribe(p_channel, callbackName, p_callback, p_priority, p_affinity, p_alwaysAsync);
        return callbackName;
    }

    void subscribe(const std::string& p_channel,
                   const std::string& p_callbackName,
                   SubscribeCallback& p_callback,
                   UBaseType_t p_priority,
                   BaseType_t p_affinity,
                   bool p_alwaysAsync)
    {
        if (itsPubSubMutex.try_lock())
        {
            subscribeUnguarded(p_channel, p_callbackName, p_callback, p_priority, p_affinity, p_alwaysAsync);
            itsPubSubMutex.unlock();
            runQueuedCalls();
        }
        else
        {
            std::lock_guard<std::mutex> lock(itsDefCallsMutex);
            itsRecursiveCallsQueue.emplace([this, p_channel, p_callbackName, p_callback, p_priority, p_affinity,
                                            p_alwaysAsync]()
            {
                subscribe(p_channel, p_callbackName, p_callback, p_priority, p_affinity, p_alwaysAsync);
            });
        }
    }

    void subscribeUnguarded(const std::string& p_channel,
                            const std::string& p_callbackName,
                            SubscribeCallback& p_callback,
                            UBaseType_t p_priority,
                            BaseType_t p_affinity,
                            bool p_alwaysAsync)
    {
        if (unlikely(itsSubscriptions[p_channel].count(p_callbackName) > 0))
        {
            ESP_LOGE(TAG, "callback name '%s' is already taken, NOT overwriting", p_callbackName.c_str());
            ESP_ERROR_CHECK(ESP_FAIL);
        }
        else
        {
            itsSubscriptions[p_channel].try_emplace(p_callbackName, p_callback, p_priority, p_affinity, p_alwaysAsync);
        }
    }

    /**
     * @brief Unsubscribe from a topic
     *
     * @param p_channel
     * @param p_callbackName
     */
    inline void unsubscribeUnguarded(const std::string& p_channel, const std::string& p_callbackName)
    {
        itsSubscriptions[p_channel].erase(p_callbackName);
    }

    /**
     * @brief Remove all callbacks for a topic
     *
     * @param channel
     */
    void clearUnguarded(const std::string& p_channel)
    {
        itsSubscriptions[p_channel].clear();
    }

    /**
     * @brief Remove all callbacks.
     */
    void clearUnguarded()
    {
        itsSubscriptions.clear();
    }

    /**
     * @brief Generates a random string
     *
     * @param p_length length of string, defaults to 16 characters
     * @return std::string with specified length
     */
    static std::string generateRandomString(unsigned int p_length = 16)
    {
        char chars[p_length];
        for (unsigned int i = 0; i < p_length; i++)
        {
            chars[i] = ' ' + 1 + (rand() % 64);
        }
        return std::string(chars, p_length);
    }

    /**
     * @brief Run all queued calls and in the process, remove them from the queue
     */
    void runQueuedCalls()
    {
        itsDefCallsMutex.lock();
        if (!itsRecursiveCallsQueue.empty())
        {
            typeof(itsRecursiveCallsQueue) recursiveCallsQueueCopy;
            // use queue in local version while emptying global version
            itsRecursiveCallsQueue.swap(recursiveCallsQueueCopy);
            itsDefCallsMutex.unlock();

            while (!itsRecursiveCallsQueue.empty())
            {
                // execute queued call
                recursiveCallsQueueCopy.front()();
                // remove call from queue
                recursiveCallsQueueCopy.pop();
            }
        }
        else
        {
            itsDefCallsMutex.unlock();
        }
    }
};
