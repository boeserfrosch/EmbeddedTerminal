#ifndef MOCKGPIOAUTH_H
#define MOCKGPIOAUTH_H

#include "../../src/interfaces/IGpioAuth.h"

class MockGpioAuth : public EmbeddedTerminal::IGpioAuth
{
public:
    ETString expectedPassword = "secret";

    bool verifyPassword(const ETString &password) override
    {
        return password == expectedPassword;
    }
};

#endif // MOCKGPIOAUTH_H