#include <unity.h>

#include "../../../src/ScriptRunner.h"

using namespace EmbeddedTerminal;

namespace
{
    ETString replaceAll_(const ETString &input, const ETString &needle, const ETString &replacement)
    {
        if (needle.empty())
        {
            return input;
        }

        ETString result = input;
        size_t pos = 0;
        while ((pos = result.find(needle, pos)) != ETString::npos)
        {
            ETString left = result.substr(0, pos);
            ETString right = result.substr(pos + needle.length());
            result = left + replacement + right;
            pos += replacement.length();
        }

        return result;
    }

    struct Harness
    {
        ETVector<ETString> executed;
        int lastExitCode = 0;
        bool commandBusy = false;
        uint64_t nowMs = 0;
        ScriptRunner runner;

        Harness()
            : runner(
                  [this](const ParsedCommand &command)
                  {
                      dispatch(command);
                  },
                  [this]()
                  {
                      return lastExitCode;
                  },
                  [this]()
                  {
                      return commandBusy;
                  },
                  [this]()
                  {
                      return nowMs;
                  },
                  [this](const ETString &input, const ETString &variableName, const ETString &value)
                  {
                      return substitute(input, variableName, value);
                  })
        {
        }

        void dispatch(const ParsedCommand &command)
        {
            ETString keyword = command.keywords.empty() ? "" : command.keywords[0];
            ETString arguments = command.arguments.empty() ? "" : command.arguments[0];
            executed.push_back(keyword + ":" + arguments);

            if (keyword == "run")
            {
                commandBusy = true;
                lastExitCode = 0;
                return;
            }

            if (keyword == "fail")
            {
                commandBusy = false;
                lastExitCode = 1;
                return;
            }

            commandBusy = false;
            lastExitCode = 0;
        }

        static ETString substitute(const ETString &input, const ETString &variableName, const ETString &value)
        {
            ETString result = input;
            result = replaceAll_(result, "${" + variableName + "}", value);
            result = replaceAll_(result, "$" + variableName, value);
            return result;
        }
    };

    void tickUntilIdle(Harness &harness, size_t maxTicks = 32)
    {
        size_t ticks = 0;
        while (harness.runner.isActive() && ticks < maxTicks)
        {
            harness.runner.tick();
            ++ticks;
        }
    }
}

void setUp(void) {}
void tearDown(void) {}

void test_script_runner_rejects_empty_script(void)
{
    Harness harness;
    ETString errorMessage;

    TEST_ASSERT_FALSE(harness.runner.enqueueScript("   ", errorMessage));
    TEST_ASSERT_TRUE(errorMessage.find("empty script") != ETString::npos);
}

void test_script_runner_executes_command_chain(void)
{
    Harness harness;
    ETString errorMessage;

    TEST_ASSERT_TRUE(harness.runner.enqueueScript("collect one; collect two", errorMessage));
    tickUntilIdle(harness);

    TEST_ASSERT_EQUAL(2, harness.executed.size());
    TEST_ASSERT_EQUAL_STRING("collect:one", harness.executed[0].c_str());
    TEST_ASSERT_EQUAL_STRING("collect:two", harness.executed[1].c_str());
    TEST_ASSERT_FALSE(harness.runner.isActive());
}

void test_script_runner_handles_if_else_branch(void)
{
    Harness harness;
    ETString errorMessage;

    TEST_ASSERT_TRUE(harness.runner.enqueueScript("if fail; then collect yes; else collect no; fi", errorMessage));
    tickUntilIdle(harness);

    TEST_ASSERT_EQUAL(2, harness.executed.size());
    TEST_ASSERT_EQUAL_STRING("fail:", harness.executed[0].c_str());
    TEST_ASSERT_EQUAL_STRING("collect:no", harness.executed[1].c_str());
    TEST_ASSERT_FALSE(harness.runner.isActive());
}

void test_script_runner_respects_delay(void)
{
    Harness harness;
    ETString errorMessage;

    TEST_ASSERT_TRUE(harness.runner.enqueueScript("collect first; delay 25; collect second", errorMessage));
    harness.runner.tick();

    TEST_ASSERT_EQUAL(1, harness.executed.size());
    TEST_ASSERT_EQUAL_STRING("collect:first", harness.executed[0].c_str());
    TEST_ASSERT_TRUE(harness.runner.isActive());

    harness.runner.tick();
    TEST_ASSERT_EQUAL(1, harness.executed.size());

    harness.nowMs = 25;
    harness.runner.tick();

    TEST_ASSERT_EQUAL(2, harness.executed.size());
    TEST_ASSERT_EQUAL_STRING("collect:second", harness.executed[1].c_str());
    tickUntilIdle(harness);
    TEST_ASSERT_FALSE(harness.runner.isActive());
}

void test_script_runner_pauses_for_busy_command(void)
{
    Harness harness;
    ETString errorMessage;

    TEST_ASSERT_TRUE(harness.runner.enqueueScript("run; collect done", errorMessage));
    harness.runner.tick();

    TEST_ASSERT_EQUAL(1, harness.executed.size());
    TEST_ASSERT_EQUAL_STRING("run:", harness.executed[0].c_str());
    TEST_ASSERT_TRUE(harness.runner.isActive());

    harness.runner.tick();
    TEST_ASSERT_EQUAL(1, harness.executed.size());

    harness.commandBusy = false;
    harness.runner.tick();

    TEST_ASSERT_EQUAL(2, harness.executed.size());
    TEST_ASSERT_EQUAL_STRING("collect:done", harness.executed[1].c_str());
    tickUntilIdle(harness);
    TEST_ASSERT_FALSE(harness.runner.isActive());
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_script_runner_rejects_empty_script);
    RUN_TEST(test_script_runner_executes_command_chain);
    RUN_TEST(test_script_runner_handles_if_else_branch);
    RUN_TEST(test_script_runner_respects_delay);
    RUN_TEST(test_script_runner_pauses_for_busy_command);
    UNITY_END();
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern "C" void app_main()
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    process_tests();
}
#elif defined(ARDUINO)
void setup()
{
    delay(2500);
    process_tests();
}
void loop() {}
#else
int main()
{
    process_tests();
    return 0;
}
#endif
