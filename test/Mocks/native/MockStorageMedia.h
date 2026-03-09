#pragma once
#include "../../../src/interfaces/IStorage.h"
#include "MockFileSystem.h"
#include <map>

using namespace EmbeddedTerminal;

class MockStorageMedia : public IStorageMedia
{
protected:
    IFileSystem *fs_;
    ETString mediaName_;
    bool available_ = true;
    unsigned long long total_ = 1024 * 1024;
    unsigned long long used_ = 0;
    unsigned long long free_ = 0;
    unsigned long long capacity_ = 0;

public:
    MockStorageMedia(const ETString &name, bool available = true, unsigned long long total = 1024 * 1024, unsigned long long used = 0, unsigned long long capacity = 1024 * 1024, unsigned long long free = 1024 * 1024, IFileSystem *fs = nullptr)
        : fs_(fs), mediaName_(name), available_(available), total_(total), used_(used), free_(free), capacity_(capacity) {}
    const char *name() const override { return mediaName_.c_str(); }
    IFileSystem *fileSystem() override { return fs_; }
    bool isAvailable() const override { return available_; }
    unsigned long long totalBytes() const override { return total_; }
    unsigned long long usedBytes() const override { return used_; }
    unsigned long long capacity() const override { return capacity_; }
    unsigned long long freeBytes() const override { return free_; }

    void setAvailable(bool available) { available_ = available; }
    void setTotalBytes(unsigned long long total) { total_ = total; }
    void setUsedBytes(unsigned long long used) { used_ = used; }
    void setCapacity(unsigned long long capacity) { capacity_ = capacity; }
};
