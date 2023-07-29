#include <stdio.h>
#include <unistd.h>
#include <PublishSubscribe.hpp>
#include "test_app_main.hpp"


TEST_CASE("simple", "[PublishSubscribe]")
{
    PublishSubscribe<int>::get().subscribeSync("topic1", [](int arg) {
        coutCapture << "arg=" << arg << "\n";
    });
    coutCapture << "before\n";
    PublishSubscribe<int>::get().publish("topic1", 42);
    coutCapture << "after\n";
    expectedOutput = "before\narg=42\nafter\n";
}

TEST_CASE("async", "[PublishSubscribe]")
{
    PublishSubscribe<int>::get().subscribeAsyncWithPrio("topic2", [](int arg) {
        coutCapture << "arg1=" << arg << "\n";
    }, 0);
    PublishSubscribe<int>::get().subscribeSync("topic2", [](int arg) {
        coutCapture << "arg2=" << arg << "\n";
    });
    coutCapture << "before\n";
    PublishSubscribe<int>::get().publish("topic2", 41);
    coutCapture << "middle\n";
    usleep(100 * 1000);
    PublishSubscribe<int>::get().publishAsyncWithPrio("topic2", 42, 0);
    coutCapture << "after\n";
    expectedOutput = "before\narg2=41\nmiddle\narg1=41\nafter\narg2=42\narg1=42\n";
}

TEST_CASE("recursive", "[PublishSubscribe]")
{
    PublishSubscribe<int>::get().subscribeSync("topic3", [](int arg) {
        PublishSubscribe<int>::get().publish("topic4", 44);
        coutCapture << "arg1=" << arg << "\n";
    });
    PublishSubscribe<int>::get().subscribeSync("topic4", [](int arg) {
        coutCapture << "arg2=" << arg << "\n";
    });
    coutCapture << "before\n";
    PublishSubscribe<int>::get().publish("topic3", 43);
    coutCapture << "after\n";
    expectedOutput = "before\narg2=44\narg1=43\nafter\n";
}

TEST_CASE("deferred subscription", "[PublishSubscribe]")
{
    PublishSubscribe<int>::get().subscribeSync("topic5", [](int arg) {
        // this subscription will only be in effect after the end of this function!
        PublishSubscribe<int>::get().subscribeSync("topic6", [](int arg) {
            coutCapture << "arg6=" << arg << "\n";
        });
        // this message does not yet have any recipients
        PublishSubscribe<int>::get().publish("topic6", 44);
        coutCapture << "arg5=" << arg << "\n";
    });
    coutCapture << "before\n";
    PublishSubscribe<int>::get().publish("topic5", 43);
    coutCapture << "after\n";
    expectedOutput = "before\narg5=43\nafter\n";
}
