#pragma once

#include "hal/common/ConfigurableGpioPolicy.h"
#include "hal/common/CompileTimeGpioAuth.h"

#if defined(ARDUINO)
#include "hal/arduino/ArduinoGpioInterface.h"
#endif

#if defined(ESP_PLATFORM)
#include "hal/espidf/ESPIDFGpioInterface.h"
#endif

#include <memory>
#include <cctype>

#ifndef ET_GPIO_DEFAULT_POLICY
#define ET_GPIO_DEFAULT_POLICY "DENY"
#endif

#ifndef ET_GPIO_POLICY_PROFILE
#define ET_GPIO_POLICY_PROFILE "default"
#endif

#ifndef ET_GPIO_DEFAULT_ALLOW
#define ET_GPIO_DEFAULT_ALLOW 0
#endif

#ifndef ET_GPIO_ALLOWED_PINS
#define ET_GPIO_ALLOWED_PINS ""
#endif

#ifndef ET_GPIO_FORCED_EXCLUDED_PINS
#define ET_GPIO_FORCED_EXCLUDED_PINS ""
#endif

#ifndef ET_GPIO_ADMIN_PASSWORD_HASH
#define ET_GPIO_ADMIN_PASSWORD_HASH ""
#endif

namespace EmbeddedTerminal
{
    class DefaultGpioSupport
    {
    public:
        DefaultGpioSupport()
            : allPinsCsv_(pinsToCsv_(defaultAvailablePins_())),
              allowedPinsCsv_(effectiveAllowedPinsCsv_(allPinsCsv_)),
              policy_(ETString(ET_GPIO_POLICY_PROFILE), ET_GPIO_DEFAULT_ALLOW != 0,
                      allowedPinsCsv_, ETString(ET_GPIO_FORCED_EXCLUDED_PINS)),
              auth_(ETString(ET_GPIO_ADMIN_PASSWORD_HASH))
        {
#if defined(ARDUINO)
            gpioArduino_.reset(new ArduinoGpioInterface(parsePinsCsv_(allPinsCsv_)));
#elif defined(ESP_PLATFORM)
            gpioEspIdf_.reset(new ESPIDFGpioInterface(parsePinsCsv_(allPinsCsv_)));
#endif
        }

        bool available() const
        {
#if defined(ARDUINO)
            return gpioArduino_.get() != nullptr;
#elif defined(ESP_PLATFORM)
            return gpioEspIdf_.get() != nullptr;
#else
            return false;
#endif
        }

        IGpioInterface *gpio()
        {
#if defined(ARDUINO)
            return gpioArduino_.get();
#elif defined(ESP_PLATFORM)
            return gpioEspIdf_.get();
#else
            return nullptr;
#endif
        }

        IGpioPolicy &policy()
        {
            return policy_;
        }

        IGpioAuth *auth()
        {
            return ETString(ET_GPIO_ADMIN_PASSWORD_HASH).trim().empty() ? nullptr : &auth_;
        }

    private:
        static ETString effectiveAllowedPinsCsv_(const ETString &allPinsCsv)
        {
            ETString configured = ETString(ET_GPIO_ALLOWED_PINS).trim();
            if (!configured.empty())
            {
                return configured;
            }
            return allPinsCsv;
        }

        static ETVector<int> defaultAvailablePins_()
        {
            ETVector<int> pins;
#if defined(ARDUINO)
#ifdef NUM_DIGITAL_PINS
            for (int pin = 0; pin < static_cast<int>(NUM_DIGITAL_PINS); pin++)
            {
                pins.push_back(pin);
            }
#endif
#elif defined(ESP_PLATFORM)
            for (int pin = 0; pin < static_cast<int>(GPIO_NUM_MAX); pin++)
            {
                pins.push_back(pin);
            }
#endif
            return pins;
        }

        static ETString pinsToCsv_(const ETVector<int> &pins)
        {
            ETString csv;
            for (size_t i = 0; i < pins.size(); i++)
            {
                if (i > 0)
                {
                    csv += ",";
                }
                csv += "GPIO" + toETString(static_cast<size_t>(pins[i]));
            }
            return csv;
        }

        static ETVector<int> parsePinsCsv_(const ETString &csv)
        {
            ETVector<int> pins;
            auto entries = split(csv, ",");

            for (const auto &rawEntry : entries)
            {
                ETString entry = rawEntry.trim();
                if (entry.empty())
                {
                    continue;
                }

                std::string upper = static_cast<std::string>(entry);
                for (auto &ch : upper)
                {
                    ch = static_cast<char>(::toupper(static_cast<unsigned char>(ch)));
                }

                ETString numericPart;
                if (upper.rfind("GPIO", 0) == 0)
                {
                    numericPart = upper.substr(4);
                }
                else
                {
                    bool digits = true;
                    for (auto ch : upper)
                    {
                        if (!::isdigit(static_cast<unsigned char>(ch)))
                        {
                            digits = false;
                            break;
                        }
                    }
                    if (!digits)
                    {
                        continue;
                    }
                    numericPart = upper;
                }

                if (numericPart.empty())
                {
                    continue;
                }

                for (size_t i = 0; i < numericPart.length(); i++)
                {
                    if (!::isdigit(static_cast<unsigned char>(numericPart[i])))
                    {
                        numericPart = "";
                        break;
                    }
                }
                if (numericPart.empty())
                {
                    continue;
                }

                pins.push_back(static_cast<int>(ETString::toull(numericPart.c_str())));
            }

            return pins;
        }

        ETString allPinsCsv_;
        ETString allowedPinsCsv_;
        ConfigurableGpioPolicy policy_;
        CompileTimeGpioAuth auth_;

#if defined(ARDUINO)
        std::unique_ptr<ArduinoGpioInterface> gpioArduino_;
#elif defined(ESP_PLATFORM)
        std::unique_ptr<ESPIDFGpioInterface> gpioEspIdf_;
#endif
    };
}