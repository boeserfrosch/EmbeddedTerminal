#ifndef MOCKGPIOINTERFACE_H
#define MOCKGPIOINTERFACE_H

#include "interfaces/IGpioInterface.h"

class MockGpioInterface : public EmbeddedTerminal::IGpioInterface
{
public:
    ETMap<ETString, bool> values;
    ETMap<ETString, EmbeddedTerminal::GpioMode> modes;

    MockGpioInterface()
    {
        values["GPIO2"] = false;
        values["GPIO5"] = true;
        values["PA5"] = false;

        modes["GPIO2"] = EmbeddedTerminal::GpioMode::Output;
        modes["GPIO5"] = EmbeddedTerminal::GpioMode::Input;
        modes["PA5"] = EmbeddedTerminal::GpioMode::Output;
    }

    ETVector<EmbeddedTerminal::GpioPinInfo> listPins() const override
    {
        ETVector<EmbeddedTerminal::GpioPinInfo> pins;
        for (const auto &entry : values)
        {
            EmbeddedTerminal::GpioPinInfo info;
            info.id = entry.first;
            info.displayName = entry.first;
            pins.push_back(info);
        }
        return pins;
    }

    bool exists(const ETString &pinId) const override
    {
        return values.find(pinId) != values.end();
    }

    bool setMode(const ETString &pinId, EmbeddedTerminal::GpioMode mode) override
    {
        if (!exists(pinId))
        {
            return false;
        }
        modes[pinId] = mode;
        return true;
    }

    bool write(const ETString &pinId, bool high) override
    {
        if (!exists(pinId))
        {
            return false;
        }
        values[pinId] = high;
        return true;
    }

    bool read(const ETString &pinId, bool &value) const override
    {
        auto it = values.find(pinId);
        if (it == values.end())
        {
            return false;
        }
        value = it->second;
        return true;
    }
};

#endif // MOCKGPIOINTERFACE_H