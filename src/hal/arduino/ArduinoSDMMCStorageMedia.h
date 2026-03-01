#pragma once

#if defined(ARDUINO) && defined(ESP32)

#include "interfaces/IStorage.h"
#include "hal/arduino/ArduinoFileSystem.h"
#include <SD_MMC.h>

namespace EmbeddedTerminal
{
    class ArduinoSDMMCStorageMedia : public IStorageMedia
    {
    public:
        explicit ArduinoSDMMCStorageMedia(const ETString &name = "sdmmc")
            : name_(name),
              fileSystem_(SD_MMC)
        {
        }

        bool begin()
        {
            return SD_MMC.begin();
        }

        const char *name() const override { return name_.c_str(); }
        IFileSystem *fileSystem() override { return &fileSystem_; }
        bool isAvailable() const override { return SD_MMC.cardType() != CARD_NONE; }
        unsigned long long totalBytes() const override { return SD_MMC.totalBytes(); }
        unsigned long long usedBytes() const override { return SD_MMC.usedBytes(); }
        unsigned long long capacity() const override { return SD_MMC.cardSize(); }
        unsigned long long freeBytes() const override
        {
            auto total = totalBytes();
            auto used = usedBytes();
            return total > used ? (total - used) : 0;
        }

    private:
        ETString name_;
        ArduinoFileSystem fileSystem_;
    };
}

#endif
