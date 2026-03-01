#pragma once

#include "interfaces/IStorage.h"

namespace EmbeddedTerminal
{
    class StorageMediaAdapter : public IStorageMedia
    {
    public:
        StorageMediaAdapter(const ETString &name,
                            IFileSystem *fileSystem,
                            bool available = true,
                            unsigned long long totalBytes = 0,
                            unsigned long long usedBytes = 0,
                            unsigned long long capacity = 0,
                            unsigned long long freeBytes = 0)
            : name_(name),
              fileSystem_(fileSystem),
              available_(available),
              totalBytes_(totalBytes),
              usedBytes_(usedBytes),
              capacity_(capacity),
              freeBytes_(freeBytes)
        {
        }

        const char *name() const override { return name_.c_str(); }
        IFileSystem *fileSystem() override { return fileSystem_; }
        bool isAvailable() const override { return available_; }
        unsigned long long totalBytes() const override { return totalBytes_; }
        unsigned long long usedBytes() const override { return usedBytes_; }
        unsigned long long capacity() const override { return capacity_; }
        unsigned long long freeBytes() const override { return freeBytes_; }

        void setAvailable(bool available) { available_ = available; }
        void setTotalBytes(unsigned long long value) { totalBytes_ = value; }
        void setUsedBytes(unsigned long long value) { usedBytes_ = value; }
        void setCapacity(unsigned long long value) { capacity_ = value; }
        void setFreeBytes(unsigned long long value) { freeBytes_ = value; }

    private:
        ETString name_;
        IFileSystem *fileSystem_;
        bool available_;
        unsigned long long totalBytes_;
        unsigned long long usedBytes_;
        unsigned long long capacity_;
        unsigned long long freeBytes_;
    };
}