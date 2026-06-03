// Platform conditional includes
#if defined(ARDUINO)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#endif

#include <unity.h>
#include <deque>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace
{
    struct GpioWriteCall
    {
        int pin = -1;
        int value = 0;
        int tick = 0;
    };

    class MockGpioScenario
    {
    public:
        void pushReads(int pin, const std::vector<int> &values)
        {
            auto &queue = readQueues_[pin];
            for (size_t i = 0; i < values.size(); ++i)
            {
                queue.push_back(values[i]);
            }
        }

        int read(int pin)
        {
            auto queueIt = readQueues_.find(pin);
            if (queueIt != readQueues_.end() && !queueIt->second.empty())
            {
                int value = queueIt->second.front();
                queueIt->second.pop_front();
                lastRead_[pin] = value;
                return value;
            }

            auto lastIt = lastRead_.find(pin);
            if (lastIt != lastRead_.end())
            {
                return lastIt->second;
            }

            lastRead_[pin] = 0;
            return 0;
        }

        void write(int pin, int value, int tick)
        {
            GpioWriteCall call;
            call.pin = pin;
            call.value = value;
            call.tick = tick;
            writes_.push_back(call);
        }

        size_t countWrites(int pin, int value) const
        {
            size_t count = 0;
            for (size_t i = 0; i < writes_.size(); ++i)
            {
                if (writes_[i].pin == pin && writes_[i].value == value)
                {
                    ++count;
                }
            }
            return count;
        }

        bool hasWrite(int pin, int value) const
        {
            for (size_t i = 0; i < writes_.size(); ++i)
            {
                if (writes_[i].pin == pin && writes_[i].value == value)
                {
                    return true;
                }
            }
            return false;
        }

        const std::vector<GpioWriteCall> &writes() const
        {
            return writes_;
        }

    private:
        std::map<int, std::deque<int>> readQueues_;
        std::map<int, int> lastRead_;
        std::vector<GpioWriteCall> writes_;
    };

    enum class SystemState
    {
        Idle,
        Running,
        Error
    };

    class Script2SchedulerHarness
    {
    public:
        static constexpr int LED = 20;
        static constexpr int RELAY = 21;
        static constexpr int BUTTON = 10;
        static constexpr int FAULT = 11;
        static constexpr int BUZZER = 22;
        static constexpr int TICK_MS = 50;

        Script2SchedulerHarness(MockGpioScenario &gpio)
            : gpio_(gpio)
        {
        }

        void runStartup()
        {
            for (int i = 0; i < 6; ++i)
            {
                gpio_.write(LED, 1, tick_);
                gpio_.write(LED, 0, tick_);
            }
        }

        void pushButtonInputs(const std::vector<int> &values)
        {
            gpio_.pushReads(BUTTON, values);
        }

        void pushFaultInputs(const std::vector<int> &values)
        {
            gpio_.pushReads(FAULT, values);
        }

        void runTicks(size_t count)
        {
            for (size_t i = 0; i < count; ++i)
            {
                runTick();
            }
        }

        void runTick()
        {
            taskWatchdog();
            taskButton();
            taskFault();
            taskPwm();
            taskBuzzer();
            processEvents();
            ++tick_;
        }

        SystemState state() const
        {
            return state_;
        }

        int watchdogKicks() const
        {
            return watchdogKicks_;
        }

    private:
        void pushEvent(const std::string &event)
        {
            eventQueue_.push_back(event);
        }

        void processEvents()
        {
            while (!eventQueue_.empty())
            {
                std::string event = eventQueue_.front();
                eventQueue_.erase(eventQueue_.begin());
                handleEvent(event);
            }
        }

        void taskWatchdog()
        {
            ++watchdogKicks_;
        }

        void taskButton()
        {
            int value = gpio_.read(BUTTON);

            // Debounce with 2 consecutive highs and edge gate.
            if (value == 1)
            {
                ++buttonDebounce_;
                if (buttonDebounce_ == 2)
                {
                    pushEvent("BUTTON_PRESS");
                }
            }
            else
            {
                buttonDebounce_ = 0;
            }

            buttonLast_ = value;
        }

        void taskFault()
        {
            int value = gpio_.read(FAULT);

            if (value == 1)
            {
                ++faultCounter_;
                if (faultCounter_ > 5)
                {
                    pushEvent("FAULT");
                    faultCounter_ = 0;
                }
            }
            else
            {
                faultCounter_ = 0;
            }
        }

        void taskPwm()
        {
            ++pwmCounter_;
            if (pwmCounter_ >= pwmPeriod_)
            {
                pwmCounter_ = 0;
            }

            gpio_.write(LED, (pwmCounter_ < pwmDuty_) ? 1 : 0, tick_);
        }

        void taskBuzzer()
        {
            if (state_ != SystemState::Error)
            {
                return;
            }

            ++beepCounter_;
            if (beepCounter_ > 10)
            {
                gpio_.write(BUZZER, 1, tick_);
            }
            if (beepCounter_ > 15)
            {
                gpio_.write(BUZZER, 0, tick_);
                beepCounter_ = 0;
            }
        }

        void handleEvent(const std::string &event)
        {
            if (event == "BUTTON_PRESS")
            {
                if (state_ == SystemState::Idle)
                {
                    gpio_.write(RELAY, 1, tick_);
                    state_ = SystemState::Running;
                }
                else if (state_ == SystemState::Running)
                {
                    gpio_.write(RELAY, 0, tick_);
                    state_ = SystemState::Idle;
                }
            }

            if (event == "FAULT")
            {
                gpio_.write(RELAY, 0, tick_);
                state_ = SystemState::Error;
            }
        }

        MockGpioScenario &gpio_;
        SystemState state_ = SystemState::Idle;
        std::vector<std::string> eventQueue_;
        int buttonLast_ = 0;
        int buttonDebounce_ = 0;
        int pwmCounter_ = 0;
        int pwmPeriod_ = 20;
        int pwmDuty_ = 5;
        int faultCounter_ = 0;
        int beepCounter_ = 0;
        int watchdogKicks_ = 0;
        int tick_ = 0;
    };

    std::string readFixture(const char *path)
    {
        std::vector<std::string> candidates;
        candidates.push_back(path);

        std::string sourcePath = __FILE__;
        size_t splitPos = sourcePath.find_last_of("\\/");
        if (splitPos != std::string::npos)
        {
            std::string sourceDir = sourcePath.substr(0, splitPos);
            candidates.push_back(sourceDir + "/" + path);
        }

        for (size_t i = 0; i < candidates.size(); ++i)
        {
            std::ifstream file(candidates[i].c_str(), std::ios::in | std::ios::binary);
            if (!file.is_open())
            {
                continue;
            }

            std::string content;
            file.seekg(0, std::ios::end);
            content.resize(static_cast<size_t>(file.tellg()));
            file.seekg(0, std::ios::beg);
            file.read(&content[0], static_cast<std::streamsize>(content.size()));
            return content;
        }

        return "";
    }
}

void setUp(void) {}
void tearDown(void) {}

void test_script_fixtures_present_and_contain_expected_sections(void)
{
    std::string script1 = readFixture("script1.et");
    std::string script2 = readFixture("script2.et");

    TEST_ASSERT_FALSE(script1.empty());
    TEST_ASSERT_FALSE(script2.empty());

    TEST_ASSERT_TRUE(script1.find("while true") != std::string::npos);
    TEST_ASSERT_TRUE(script1.find("gpio write 21 1") != std::string::npos);
    TEST_ASSERT_TRUE(script2.find("watchdog kick") != std::string::npos);
    TEST_ASSERT_TRUE(script2.find("gpio read 10") != std::string::npos);
    TEST_ASSERT_TRUE(script2.find("while true") != std::string::npos);
}

void test_script2_startup_sequence_led_behavior(void)
{
    MockGpioScenario gpio;
    Script2SchedulerHarness harness(gpio);

    harness.runStartup();

    TEST_ASSERT_EQUAL(6, gpio.countWrites(Script2SchedulerHarness::LED, 1));
    TEST_ASSERT_EQUAL(6, gpio.countWrites(Script2SchedulerHarness::LED, 0));
}

void test_script2_button_press_starts_system(void)
{
    MockGpioScenario gpio;
    Script2SchedulerHarness harness(gpio);

    harness.pushButtonInputs({0, 1, 1});
    harness.runTicks(3);

    TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Running), static_cast<int>(harness.state()));
    TEST_ASSERT_TRUE(gpio.hasWrite(Script2SchedulerHarness::RELAY, 1));
}

void test_script2_button_press_stops_system(void)
{
    MockGpioScenario gpio;
    Script2SchedulerHarness harness(gpio);

    harness.pushButtonInputs({0, 1, 1});
    harness.runTicks(3);

    harness.pushButtonInputs({0, 0, 1, 1});
    harness.runTicks(4);

    TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Idle), static_cast<int>(harness.state()));
    TEST_ASSERT_TRUE(gpio.hasWrite(Script2SchedulerHarness::RELAY, 1));
    TEST_ASSERT_TRUE(gpio.hasWrite(Script2SchedulerHarness::RELAY, 0));
}

void test_script2_fault_during_operation_sets_error_state(void)
{
    MockGpioScenario gpio;
    Script2SchedulerHarness harness(gpio);

    harness.pushButtonInputs({1, 1});
    harness.runTicks(2);

    harness.pushFaultInputs({1, 1, 1, 1, 1, 1});
    harness.runTicks(6);

    TEST_ASSERT_EQUAL(static_cast<int>(SystemState::Error), static_cast<int>(harness.state()));
    TEST_ASSERT_TRUE(gpio.hasWrite(Script2SchedulerHarness::RELAY, 0));
}

void test_script2_scheduler_and_watchdog_execute_each_tick(void)
{
    MockGpioScenario gpio;
    Script2SchedulerHarness harness(gpio);

    harness.runTicks(25);

    TEST_ASSERT_EQUAL(25, harness.watchdogKicks());
    TEST_ASSERT_TRUE(gpio.countWrites(Script2SchedulerHarness::LED, 1) > 0);
    TEST_ASSERT_TRUE(gpio.countWrites(Script2SchedulerHarness::LED, 0) > 0);
}

void test_script2_error_state_drives_buzzer_pattern(void)
{
    MockGpioScenario gpio;
    Script2SchedulerHarness harness(gpio);

    harness.pushButtonInputs({1, 1});
    harness.runTicks(2);

    harness.pushFaultInputs({1, 1, 1, 1, 1, 1});
    harness.runTicks(6);

    harness.runTicks(20);

    TEST_ASSERT_TRUE(gpio.hasWrite(Script2SchedulerHarness::BUZZER, 1));
    TEST_ASSERT_TRUE(gpio.hasWrite(Script2SchedulerHarness::BUZZER, 0));
}

void process_tests()
{
    UNITY_BEGIN();
    RUN_TEST(test_script_fixtures_present_and_contain_expected_sections);
    RUN_TEST(test_script2_startup_sequence_led_behavior);
    RUN_TEST(test_script2_button_press_starts_system);
    RUN_TEST(test_script2_button_press_stops_system);
    RUN_TEST(test_script2_fault_during_operation_sets_error_state);
    RUN_TEST(test_script2_scheduler_and_watchdog_execute_each_tick);
    RUN_TEST(test_script2_error_state_drives_buzzer_pattern);
    UNITY_END();
}

#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
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
