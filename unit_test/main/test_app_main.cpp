#include <esp_system.h>
#include "test_app_main.hpp"

std::ostringstream capturedOutput;
std::string expectedOutput;
teestream coutCapture(std::cout, capturedOutput);

extern "C"
{
void setUp(void)
{
    // this is run before each test
    capturedOutput.clear();
    capturedOutput.str("");
    expectedOutput.clear();
}

void tearDown(void)
{
    // this is run after each test
    usleep(300 * 1000);
    TEST_ASSERT_EQUAL_STRING(expectedOutput.c_str(), capturedOutput.str().c_str());
}

void app_main(void)
{
    UnityBegin(NULL);

    //unity_run_menu();
    unity_run_all_tests();

    usleep(300 * 1000);

    UnityEnd();

    usleep(300 * 1000);

    esp_restart();
}
}