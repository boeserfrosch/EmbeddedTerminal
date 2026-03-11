#pragma once

#include "interfaces/IGpioAuth.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace EmbeddedTerminal
{
    class CompileTimeGpioAuth : public IGpioAuth
    {
    public:
        explicit CompileTimeGpioAuth(const ETString &expectedHashHex) : expectedHashHex_(expectedHashHex.trim())
        {
        }

        bool verifyPassword(const ETString &password) override
        {
            if (expectedHashHex_.empty())
            {
                return false;
            }

            uint32_t expectedHash = parseHex_(expectedHashHex_);
            uint32_t providedHash = fnv1a32_(password);
            return expectedHash == providedHash;
        }

        static ETString hashPasswordHex(const ETString &password)
        {
            auto hash = fnv1a32_(password);
            char buf[11] = {0};
#if defined(ARDUINO)
            snprintf(buf, sizeof(buf), "0x%08lX", static_cast<unsigned long>(hash));
#else
            std::snprintf(buf, sizeof(buf), "0x%08X", static_cast<unsigned int>(hash));
#endif
            return ETString(buf);
        }

    private:
        static uint32_t fnv1a32_(const ETString &password)
        {
            const uint32_t offsetBasis = 2166136261u;
            const uint32_t prime = 16777619u;

            uint32_t hash = offsetBasis;
            auto plain = static_cast<std::string>(password);
            for (unsigned char byte : plain)
            {
                hash ^= static_cast<uint32_t>(byte);
                hash *= prime;
            }
            return hash;
        }

        static uint32_t parseHex_(const ETString &hashHex)
        {
            std::string value = static_cast<std::string>(hashHex);
            return static_cast<uint32_t>(std::strtoul(value.c_str(), nullptr, 16));
        }

        ETString expectedHashHex_;
    };
}