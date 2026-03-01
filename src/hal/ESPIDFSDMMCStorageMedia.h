#pragma once
#include "../interfaces/IStorage.h"
#include "SDMMCFileSystem.h"

#if defined(ESP_PLATFORM) || defined(ESP_32)
#include <string>
namespace EmbeddedTerminal
{
    class ESPIDFSDMMCStorageMedia : public IStorageMedia
    {
    public:
        ESPIDFSDMMCStorageMedia(const ETString &name, SDMMCFileSystem *fs)
            : name_(name), fs_(fs) {}
        const char *name() const override { return name_.c_str(); }
        IFileSystem *fileSystem() override { return fs_; }
        bool isAvailable() const override { return true; }
        unsigned long long capacity() const override { return fs_ ? fs_->capacity() : 0; }
        unsigned long long usedBytes() const override { return fs_ ? fs_->usedBytes() : 0; }
        unsigned long long freeBytes() const override { return fs_ ? (fs_->capacity() - fs_->usedBytes()) : 0; }

    private:
        ETString name_;
        SDMMCFileSystem *fs_;
    };
}
#endif
