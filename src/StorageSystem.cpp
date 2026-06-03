#include "StorageSystem.h"

namespace EmbeddedTerminal
{

    // Default max copy size: 10 MB
    size_t StorageSystem::maxCopySize_ = 10 * 1024 * 1024;

    void StorageSystem::setMaxCopySize(size_t bytes)
    {
        StorageSystem::maxCopySize_ = bytes;
    }

    size_t StorageSystem::getMaxCopySize()
    {
        return StorageSystem::maxCopySize_;
    }

} // namespace EmbeddedTerminal
