#ifndef IGPIOINTERFACE_H
#define IGPIOINTERFACE_H

#include "../ETTypes.h"

namespace EmbeddedTerminal
{
    enum class GpioMode
    {
        Input,
        Output
    };

    struct GpioPinInfo
    {
        ETString id;
        ETString displayName;
    };

    class IGpioInterface
    {
    public:
        virtual ~IGpioInterface() {}

        virtual ETVector<GpioPinInfo> listPins() const = 0;
        virtual bool exists(const ETString &pinId) const = 0;
        virtual bool setMode(const ETString &pinId, GpioMode mode) = 0;
        virtual bool write(const ETString &pinId, bool high) = 0;
        virtual bool read(const ETString &pinId, bool &value) const = 0;
    };
}

#endif // IGPIOINTERFACE_H