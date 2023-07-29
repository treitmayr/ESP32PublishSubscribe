# ESP32PublishSubscribe
Implementation of the Publish/Subscribe Pattern for the Espressif ESP32 microcontroller.

## PublishSubscribe

This part of the library library implements the publish/subscribe pattern specifically for the ESP32 microcontroller. Its main features are:

* Thread-safety:
  Messages may be published and subscribed from multiple threads concurrently.
* Reentrant:
  Messages may be published and new subscriptions may be requested from within subscription handlers.
* Messages consisting of variable-length argument lists:
  Argument lists may be designed according to the needs of a specific event type.
* Asynchronous and synchronous publishing and subscriptions
* Flexible task priority for asynchronous operations, default to the task priority of the current task.

Header file: [PublishSubscribe.hpp](include/PublishSubscribe.hpp)

Examples: See [testPublishSubscribe.cpp](unit_test/main/testPublishSubscribe.cpp)

## DeferredCallsQueue

Execute functions in a deferred and asynchronous way.

Header file: [DeferredCallsQueue.hpp](include/DeferredCallsQueue.hpp)

Examples: See [testDeferredCalls.cpp](unit_test/main/testDeferredCalls.cpp)

#  Copyright/License
(c) 2023 Thomas Reitmayr\
MIT License

**Note:**
This library is originally based on work from https://github.com/nbdy/pubsupp
which is Copyright (c) 2021 Pascal Eberlein, MIT License

