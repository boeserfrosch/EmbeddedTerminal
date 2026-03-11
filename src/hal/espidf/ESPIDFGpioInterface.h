#pragma once

#if defined(ESP_PLATFORM)

#include "interfaces/IGpioInterface.h"
#include "ETTypes.h"

extern "C"
{
#include "driver/gpio.h"
}

namespace EmbeddedTerminal
{
    class ESPIDFGpioInterface : public IGpioInterface
    {
    public:
        explicit ESPIDFGpioInterface(const ETVector<int> &pins)
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

            gpio_config_t conf = {};
            conf.pin_bit_mask = (1ULL << static_cast<uint64_t>(it->second));
            conf.mode = (mode == GpioMode::Output) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
            conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            conf.pull_up_en = GPIO_PULLUP_DISABLE;
            conf.intr_type = GPIO_INTR_DISABLE;

            return gpio_config(&conf) == ESP_OK;
        }

        bool write(const ETString &pinId, bool high) override
        {
            auto it = pinMap_.find(pinId);
            if (it == pinMap_.end())
            {
                return false;
            }

            return gpio_set_level(static_cast<gpio_num_t>(it->second), high ? 1 : 0) == ESP_OK;
        }

        bool read(const ETString &pinId, bool &value) const override
        {
            auto it = pinMap_.find(pinId);
            if (it == pinMap_.end())
            {
                return false;
            }

            int level = gpio_get_level(static_cast<gpio_num_t>(it->second));
            value = (level != 0);
            return true;
        }

    private:
        ETMap<ETString, int> pinMap_;
        ETVector<GpioPinInfo> pins_;
    };
}

#endif