#ifndef ET_PROFILING_H
#define ET_PROFILING_H

#ifndef ET_ENABLE_PROFILING
#define ET_ENABLE_PROFILING 0
#endif

#include "ETTypes.h"

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#if defined(ESP_PLATFORM)
#include "esp_heap_caps.h"
#include "esp_timer.h"
#endif
#endif

#include <chrono>

namespace EmbeddedTerminal
{
    namespace Profiling
    {
        struct Sample
        {
            uint64_t timeUs = 0;
            size_t heapBytes = 0;
            size_t stackWatermarkBytes = 0;
        };

        inline uint64_t nowUs()
        {
#if defined(ARDUINO)
            return static_cast<uint64_t>(micros());
#elif defined(ESP_PLATFORM)
            return static_cast<uint64_t>(esp_timer_get_time());
#else
            auto now = std::chrono::steady_clock::now().time_since_epoch();
            return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
#endif
        }

        inline size_t heapBytes()
        {
#if defined(ESP_PLATFORM)
            return heap_caps_get_free_size(MALLOC_CAP_8BIT);
#elif defined(ARDUINO)
            return ESP.getFreeHeap();
#else
            return 0;
#endif
        }

        inline size_t stackWatermarkBytes()
        {
#if defined(ESP_PLATFORM) || defined(ARDUINO)
            return static_cast<size_t>(uxTaskGetStackHighWaterMark(nullptr)) * sizeof(StackType_t);
#else
            return 0;
#endif
        }

        inline Sample capture()
        {
            Sample sample;
            sample.timeUs = nowUs();
            sample.heapBytes = heapBytes();
            sample.stackWatermarkBytes = stackWatermarkBytes();
            return sample;
        }

        inline ETString formatReport(const ETString &scope, const Sample &start, const Sample &end)
        {
            ETString message = "[profile] ";
            message += scope;
            message += " duration=";
            message += toETString(static_cast<size_t>(end.timeUs - start.timeUs));
            message += "us";

            message += " heap=";
            message += toETString(start.heapBytes);
            message += "->";
            message += toETString(end.heapBytes);

            message += " stack=";
            message += toETString(start.stackWatermarkBytes);
            message += "->";
            message += toETString(end.stackWatermarkBytes);

            return message;
        }
    }
}

#endif // ET_PROFILING_H
