#pragma once

#if defined(ESP_PLATFORM) || defined(ESP_32)

#include "../interfaces/IStorage.h"
#include "ESPIDFFileSystem.h"
#include <sys/stat.h>
#include <sys/statvfs.h>

namespace EmbeddedTerminal
{
    class ESPIDFSDMMCStorageMedia : public IStorageMedia
    {
    public:
        explicit ESPIDFSDMMCStorageMedia(const ETString &name = "sdmmc", const Path &mountPoint = "/sdcard")
            : name_(name),
              mountPoint_(mountPoint),
              fileSystem_(mountPoint)
        {
        }

        const char *name() const override { return name_.c_str(); }
        IFileSystem *fileSystem() override { return &fileSystem_; }

        bool isAvailable() const override
        {
            struct stat st;
            return stat(mountPoint_.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
        }

        unsigned long long totalBytes() const override
        {
            struct statvfs statBuffer;
            if (statvfs(mountPoint_.c_str(), &statBuffer) != 0)
                return 0;
            return static_cast<unsigned long long>(statBuffer.f_blocks) * static_cast<unsigned long long>(statBuffer.f_frsize);
        }

        unsigned long long usedBytes() const override
        {
            auto total = totalBytes();
            auto free = freeBytes();
            return total > free ? (total - free) : 0;
        }

        unsigned long long capacity() const override
        {
            return totalBytes();
        }

        unsigned long long freeBytes() const override
        {
            struct statvfs statBuffer;
            if (statvfs(mountPoint_.c_str(), &statBuffer) != 0)
                return 0;
            return static_cast<unsigned long long>(statBuffer.f_bavail) * static_cast<unsigned long long>(statBuffer.f_frsize);
        }

    private:
        ETString name_;
        Path mountPoint_;
        ESPIDFFileSystem fileSystem_;
    };
}

#endif
