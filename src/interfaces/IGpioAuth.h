#ifndef IGPIOAUTH_H
#define IGPIOAUTH_H

#include "../ETTypes.h"

namespace EmbeddedTerminal
{
    class IGpioAuth
    {
    public:
        virtual ~IGpioAuth() {}
        virtual bool verifyPassword(const ETString &password) = 0;
    };
}

#endif // IGPIOAUTH_H