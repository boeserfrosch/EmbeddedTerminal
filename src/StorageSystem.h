#ifndef ET_STORAGE_SYSTEM_H
#define ET_STORAGE_SYSTEM_H

#include "interfaces/IStorage.h"
#include "ETTypes.h"
#include <cstring>
#include <mutex>

namespace EmbeddedTerminal
{
    class StorageSystem : public IStorageSystem
    {
    public:
        StorageSystem() = default;
        ~StorageSystem() override = default;

        bool mountMedia(std::shared_ptr<IStorageMedia> media, const Path &mountPoint) override
        {
            std::lock_guard<std::mutex> guard(mountMutex_);
            if (!media)
                return false;

            if (mountPoints_.find(mountPoint) != mountPoints_.end())
                return false;
            mountPoints_[mountPoint] = media;
            return true;
        }

        bool unmountMedia(const ETString &name) override
        {
            std::lock_guard<std::mutex> guard(mountMutex_);
            // Find mount point by media name and erase directly while holding the lock
            for (auto it = mountPoints_.begin(); it != mountPoints_.end(); ++it)
            {
                if (it->second && strcmp(it->second->name(), name.c_str()) == 0)
                {
                    mountPoints_.erase(it);
                    return true;
                }
            }
            return false;
        }

        bool unmountMediaFromMountpoint(const Path &mountPoint) override
        {
            std::lock_guard<std::mutex> guard(mountMutex_);
            return mountPoints_.erase(mountPoint) > 0;
        }

        ETVector<std::shared_ptr<IStorageMedia>> media() const override
        {
            std::lock_guard<std::mutex> guard(mountMutex_);
            ETVector<std::shared_ptr<IStorageMedia>> result;
            for (const auto &kv : mountPoints_)
                result.push_back(kv.second);
            return result;
        }

        std::shared_ptr<IStorageMedia> getMedia(const ETString &name) const override
        {
            std::lock_guard<std::mutex> guard(mountMutex_);
            // Find media mount point by media name
            for (const auto &kv : mountPoints_)
            {
                if (kv.second && strcmp(kv.second->name(), name.c_str()) == 0)
                    return kv.second;
            }
            return nullptr;
        }
        std::shared_ptr<IStorageMedia> getMediaFromPath(const Path &path) const override
        {
            std::lock_guard<std::mutex> guard(mountMutex_);
            Path p = path;
            if (!p.isAbsolute())
            {
                p = Path("/") + p; // Make it absolute for easier matching
            }
            const Path *longestMatch = nullptr;
            std::shared_ptr<IStorageMedia> found = nullptr;
            for (const auto &kv : mountPoints_)
            {
                Path prefix = kv.first + "/";
                if (p.isChildOf(prefix) && (!longestMatch || prefix.isChildOf(*longestMatch)))
                {
                    longestMatch = &kv.first;
                    found = kv.second;
                }
            }
            return found;
        }

        ETFile open(const Path &path, const char *mode = FILE_MODE_READ, const bool create = false) override
        {
            auto media = getMediaFromPath(path);
            if (!media)
                return ETFile();
            Path fsPath = stripMediaPrefix(path);
            auto fs = media->fileSystem();
            if (!fs)
                return ETFile();
            return fs->open(fsPath, mode, create);
        }

        bool exists(const Path &path) override
        {
            auto media = getMediaFromPath(path);
            if (!media)
            {
                return false;
            }
            Path fsPath = stripMediaPrefix(path);
            auto fs = media->fileSystem();
            if (!fs)
                return false;
            return fs->exists(fsPath);
        }
        bool isDirectory(const Path &path) override
        {
            auto media = getMediaFromPath(path);
            if (!media)
                return false;
            Path fsPath = stripMediaPrefix(path);
            auto fs = media->fileSystem();
            if (!fs)
                return false;
            return fs->isDirectory(fsPath);
        }

        bool isEmpty(const Path &path) override
        {
            auto media = getMediaFromPath(path);
            if (!media)
                return false;
            Path fsPath = stripMediaPrefix(path);
            auto fs = media->fileSystem();
            if (!fs)
                return false;
            return fs->isEmpty(fsPath);
        }

        bool remove(const Path &path) override
        {
            auto media = getMediaFromPath(path);
            if (!media)
                return false;
            Path fsPath = stripMediaPrefix(path);
            auto fs = media->fileSystem();
            if (!fs)
                return false;
            return fs->remove(fsPath);
        }
        bool mkdir(const Path &path) override
        {
            auto media = getMediaFromPath(path);
            if (!media)
                return false;
            Path fsPath = stripMediaPrefix(path);
            auto fs = media->fileSystem();
            if (!fs)
                return false;
            return fs->mkdir(fsPath);
        }
        bool rmdir(const Path &path) override
        {
            auto media = getMediaFromPath(path);
            if (!media)
                return false;
            Path fsPath = stripMediaPrefix(path);
            auto fs = media->fileSystem();
            if (!fs)
                return false;
            return fs->rmdir(fsPath);
        }
        ETVector<Path> list(const Path &path, const ETString &prefix = ETString("")) const override
        {
            auto media = getMediaFromPath(path);
            if (!media)
                return ETVector<Path>();
            Path fsPath = stripMediaPrefix(path);
            auto fs = media->fileSystem();
            if (!fs)
                return ETVector<Path>();
            return fs->list(fsPath, prefix);
        }

        // Configure maximum allowed copy size (in bytes). Default set in StorageSystem.cpp
        static void setMaxCopySize(size_t bytes);
        static size_t getMaxCopySize();

        bool copyFile(const Path &srcPath, const Path &dstPath) override
        {
            // Both src and dst must be valid and src must exist and media must exist
            if (srcPath.isEmpty() || dstPath.isEmpty())
                return false;

            if (!exists(srcPath))
                return false;

            auto src = getMediaFromPath(srcPath);
            auto dst = getMediaFromPath(dstPath);
            if (!src || !dst)
                return false;

            auto srcFS = src->fileSystem();
            auto dstFS = dst->fileSystem();

            if (!srcFS || !dstFS)
                return false;

            Path srcFSPath = stripMediaPrefix(srcPath);
            Path dstFSPath = stripMediaPrefix(dstPath);

            ETFile srcFile = srcFS->open(srcFSPath, FILE_MODE_READ);
            if (!srcFile.isOpen())
                return false;

            // Optional size check to avoid OOM on constrained devices
            size_t totalSize = srcFile.size();
            size_t maxCopy = StorageSystem::getMaxCopySize();
            if (totalSize > 0 && maxCopy > 0 && totalSize > maxCopy)
            {
                srcFile.close();
                return false;
            }

            ETFile dstFile = dstFS->open(dstFSPath, FILE_MODE_WRITE, true);
            if (!dstFile.isOpen())
            {
                srcFile.close();
                return false;
            }

            const size_t CHUNK_SIZE = 4096;
            std::vector<char> buffer(CHUNK_SIZE);
            size_t readBytes = 0;
            bool ok = true;
            while ((readBytes = srcFile.read(buffer.data(), CHUNK_SIZE)) > 0)
            {
                size_t written = dstFile.write(buffer.data(), readBytes);
                if (written != readBytes)
                {
                    ok = false;
                    break;
                }
            }

            srcFile.close();
            dstFile.close();
            return ok;
        }
        bool moveFile(const Path &srcPath, const Path &dstPath) override
        {
            if (!copyFile(srcPath, dstPath))
                return false;

            return removeFile(srcPath);
        }
        bool removeFile(const Path &path) override
        {
            auto m = getMediaFromPath(path);
            if (!m)
                return false;
            Path fsPath = stripMediaPrefix(path);
            auto fs = m->fileSystem();
            if (!fs)
                return false;
            return fs->remove(fsPath);
        }

    protected:
        /**
         * @brief Strips the media prefix from the given path. For example, if the path is "/SD/file.txt" and there is a media mounted at "/SD", it will return "file.txt". If the path does not start with any media prefix, it will return the original path.
         * @param path The path to strip the media prefix from.
         */
        Path stripMediaPrefix(const Path &path) const
        {
            std::lock_guard<std::mutex> guard(mountMutex_);
            const Path *longestMatch = nullptr;
            for (const auto &kv : mountPoints_)
            {
                Path prefix = kv.first + "/";
                if (path.isChildOf(prefix))
                {
                    if (!longestMatch)
                    {
                        longestMatch = &kv.first;
                    }
                    else
                    {
                        Path currentLongestPrefix = (*longestMatch) + "/";
                        if (prefix.isChildOf(currentLongestPrefix))
                            longestMatch = &kv.first;
                    }
                }
            }
            if (longestMatch)
            {
                Path prefix = (*longestMatch) + "/";
                return path.relativeTo(prefix);
            }
            return path;
        }

    private:
        ETMap<Path, std::shared_ptr<IStorageMedia>> mountPoints_;
        mutable std::mutex mountMutex_;
        static size_t maxCopySize_;
    };
}

#endif // ET_STORAGE_SYSTEM_H