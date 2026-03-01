#pragma once

#if defined(__cplusplus) && __cplusplus >= 201703L

#include "interfaces/IStorage.h"
#include "hal/native/NativeFileSystem.h"
#include <filesystem>

namespace EmbeddedTerminal
{
    class NativeSuggestedStorageMedia : public IStorageMedia
    {
    public:
        explicit NativeSuggestedStorageMedia(const ETString &name = "native", const Path &rootPath = ".")
            : name_(name),
              rootPath_(rootPath)
        {
        }

        const char *name() const override { return name_.c_str(); }
        IFileSystem *fileSystem() override { return &fileSystem_; }

        bool isAvailable() const override
        {
            std::error_code ec;
            return std::filesystem::exists(rootPath_.c_str(), ec);
        }

        unsigned long long totalBytes() const override
        {
            std::error_code ec;
            auto stats = std::filesystem::space(rootPath_.c_str(), ec);
            if (ec)
                return 0;
            return static_cast<unsigned long long>(stats.capacity);
        }

        unsigned long long usedBytes() const override
        {
            std::error_code ec;
            auto stats = std::filesystem::space(rootPath_.c_str(), ec);
            if (ec)
                return 0;
            return static_cast<unsigned long long>(stats.capacity - stats.available);
        }

        unsigned long long capacity() const override
        {
            return totalBytes();
        }

        unsigned long long freeBytes() const override
        {
            std::error_code ec;
            auto stats = std::filesystem::space(rootPath_.c_str(), ec);
            if (ec)
                return 0;
            return static_cast<unsigned long long>(stats.available);
        }

    private:
        ETString name_;
        Path rootPath_;
        NativeFileSystem fileSystem_;
    };
}

#endif
