#include <stdio.h>
#include <unistd.h>
#include <DeferredCallsQueue.hpp>
#include "test_app_main.hpp"


TEST_CASE("simple", "[DeferredCallsQueue]")
{
    coutCapture << "before\n";
    DeferredCallsQueue& dcq = DeferredCallsQueue::get();
    dcq.addDeferredCall([]() { coutCapture << "deferred call\n"; }, 0);
    coutCapture << "after\n";
    expectedOutput = "before\nafter\ndeferred call\n";
}

TEST_CASE("recursive", "[DeferredCallsQueue]")
{
    coutCapture << "before\n";
    DeferredCallsQueue& dcq = DeferredCallsQueue::get();
    dcq.addDeferredCall([]() {
        coutCapture << "deferred call1\n";
        DeferredCallsQueue::get().addDeferredCall([]() { coutCapture << "deferred call2\n"; }, 0);
    });
    coutCapture << "after\n";
    expectedOutput = "before\nafter\ndeferred call1\ndeferred call2\n";
}

TEST_CASE("queue size", "[DeferredCallsQueue]")
{
    coutCapture << "before\n";
    DeferredCallsQueue& dcq = DeferredCallsQueue::get();
    for (int i = 0; i < DeferredCallsQueue::itsQueueSize; i++)
    {
        dcq.addDeferredCall([i]() { coutCapture << i << " "; }, 0);
    }
    coutCapture << "middle\n";
    usleep(300 * 1000);
    coutCapture << "after\n";
    expectedOutput = "before\nmiddle\n";
    for (int i = 0; i < DeferredCallsQueue::itsQueueSize; i++)
    {
        expectedOutput += std::to_string(i) + " ";
    }
    expectedOutput += "after\n";
}

TEST_CASE("overflow", "[DeferredCallsQueue]")
{
    coutCapture << "before\n";
    DeferredCallsQueue& dcq = DeferredCallsQueue::get();
    int upperLimit = DeferredCallsQueue::itsQueueSize + 3;
    for (int i = 0; i < upperLimit; i++) {
        dcq.addDeferredCall([i]() { coutCapture << i << " "; }, 0);
    }
    coutCapture << "middle\n";
    usleep(300 * 1000);
    coutCapture << "after\n";
    expectedOutput = "before\n";
    for (int i = 0; i < (upperLimit - DeferredCallsQueue::itsQueueSize - 1); i++)
    {
        expectedOutput += std::to_string(i) + " ";
    }
    expectedOutput += "middle\n";
    for (int i = (upperLimit - DeferredCallsQueue::itsQueueSize - 1); i < upperLimit; i++)
    {
        expectedOutput += std::to_string(i) + " ";
    }
    expectedOutput += "after\n";
}

TEST_CASE("priority", "[DeferredCallsQueue]")
{
    coutCapture << "before\n";
    DeferredCallsQueue& dcq = DeferredCallsQueue::get();
    for (int i = 0; i < DeferredCallsQueue::itsQueueSize / 2; i++) {
        dcq.addDeferredCall([i]() { coutCapture << i << " "; }, i % 2);
    }
    usleep(200 * 1000);
    coutCapture << "after\n";
    expectedOutput = "before\n";
    for (int i = 1; i < DeferredCallsQueue::itsQueueSize / 2; i += 2)
    {
        expectedOutput += std::to_string(i) + " ";
    }
    for (int i = 0; i < DeferredCallsQueue::itsQueueSize / 2; i += 2)
    {
        expectedOutput += std::to_string(i) + " ";
    }
    expectedOutput += "after\n";
}

TEST_CASE("many args", "[DeferredCallsQueue]")
{
    coutCapture << "before\n";
    DeferredCallsQueue& dcq = DeferredCallsQueue::get();
    for (int i = 0; i < DeferredCallsQueue::itsQueueSize; i++) {
        int j = i + 1;
        int k = j + 1;
        dcq.addDeferredCall([i,j,k]()
        {
            coutCapture << i << "/" << j << "/" << k << " ";
        }, 0);
    }
    usleep(400 * 1000);
    coutCapture << "after\n";
    expectedOutput = "before\n";
    for (int i = 0; i < DeferredCallsQueue::itsQueueSize; i++)
    {
        expectedOutput += std::to_string(i) + "/" +
                          std::to_string(i + 1) + "/" +
                          std::to_string(i + 2) + " ";
    }
    expectedOutput += "after\n";
}
