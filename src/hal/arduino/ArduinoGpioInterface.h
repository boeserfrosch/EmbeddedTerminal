#pragma once

#if defined(ARDUINO)

#include "interfaces/IGpioInterface.h"
#include "ETTypes.h"
#include <Arduino.h>

namespace EmbeddedTerminal
{
    class ArduinoGpioInterface : public IGpioInterface
    {
    public:
        ArduinoGpioInterface(const ETVector<int> &pins)
        {
            for (auto pin : pins)
            {
                ETString id = "GPIO" + toETString(static_cast<size_t>(pin));
                pinMap_[id] = pin;

                GpioPinInfo info;
                info.id = id;
                info.displayName = id;
                pins_.push_back(info);
            }
        }

        ETVector<GpioPinInfo> listPins() const override
        {
            return pins_;
        }

        bool exists(const ETString &pinId) const override
        {
            return pinMap_.find(pinId) != pinMap_.end();
        }

        bool setMode(const ETString &pinId, GpioMode mode) override
        {
            auto it = pinMap_.find(pinId);
            if (it == pinMap_.end())
            {
                return false;
            }

            pinMode(it->second, mode == GpioMode::Output ? OUTPUT : INPUT);
            return true;
        }

        bool write(const ETString &pinId, bool high) override
        {
            auto it = pinMap_.find(pinId);
            if (it == pinMap_.end())
            {
                return false;
            }

            digitalWrite(it->second, high ? HIGH : LOW);
            return true;
        }

        bool read(const ETString &pinId, bool &value) const override
        {
            auto it = pinMap_.find(pinId);
            if (it == pinMap_.end())
            {
                return false;
            }

            value = digitalRead(it->second) == HIGH;
            return true;
        }

    private:
        ETMap<ETString, int> pinMap_;
        ETVector<GpioPinInfo> pins_;
    };
}

#endif