#ifndef ET_STORAGE_SYSTEM_H
#define ET_STORAGE_SYSTEM_H

#include "interfaces/IStorage.h"

namespace EmbeddedTerminal
{
    class StorageSystem : public IStorageSystem
    {
    public:
        StorageSystem() = default;
        ~StorageSystem() override = default;

        bool mountMedia(IStorageMedia *media, const Path &mountPoint) override
        {
            if (mountPoints_.find(mountPoint) != mountPoints_.end())
                return false;
            mountPoints_[mountPoint] = media;
            return true;
        }

        bool unmountMedia(const ETString &name) override
        {
            // Find mount point by media name
            for (auto it = mountPoints_.begin(); it != mountPoints_.end(); ++it)
            {
                if (strcmp(it->second->name(), name.c_str()) == 0)
                {
                    return unmountMediaFromMountpoint(it->first);
                }
            }
            return false;
        }

        bool unmountMediaFromMountpoint(const Path &mountPoint) override
        {
            return mountPoints_.erase(mountPoint) > 0;
        }

        ETVector<IStorageMedia *> media() const override
        {
            ETVector<IStorageMedia *> result;
            for (const auto &kv : mountPoints_)
                result.push_back(kv.second);
            return result;
        }

        IStorageMedia *getMedia(const ETString &name) const override
        {
            // Find media mount point by media name
            for (const auto &kv : mountPoints_)
            {
                if (strcmp(kv.second->name(), name.c_str()) == 0)
                    return kv.second;
            }
            return nullptr;
        }
        IStorageMedia *getMediaFromPath(const Path &path) const override
        {
            Path p = path;
            if (!p.isAbsolute())
            {
                p = "/" + p; // Make it absolute for easier matching
            }

            Path longestMatch;
            IStorageMedia *found = nullptr;
            for (const auto &kv : mountPoints_)
            {
                Path prefix = kv.first + "/";
                if (p.isChildOf(prefix) && (longestMatch.isEmpty() || prefix.isChildOf(longestMatch)))
                {
                    longestMatch = prefix;
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
            ETString fsPath = stripMediaPrefix(path);
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
            ETString fsPath = stripMediaPrefix(path);
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
            ETString fsPath = stripMediaPrefix(path);
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
            ETString fsPath = stripMediaPrefix(path);
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
            ETString fsPath = stripMediaPrefix(path);
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
            ETString fsPath = stripMediaPrefix(path);
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
            ETString fsPath = stripMediaPrefix(path);
            auto fs = media->fileSystem();
            if (!fs)
                return false;
            return fs->rmdir(fsPath);
        }
        ETVector<Path> list(const Path &path, const ETString &prefix = "") const override
        {
            auto media = getMediaFromPath(path);
            if (!media)
                return ETVector<Path>();
            ETString fsPath = stripMediaPrefix(path);
            auto fs = media->fileSystem();
            if (!fs)
                return ETVector<Path>();
            return fs->list(fsPath, prefix);
        }

        bool copyFile(const Path &srcPath, const Path &dstPath) override
        {
            // Booth src and dst must be valid and src have to exist and media must exist
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

            ETString srcFSPath = stripMediaPrefix(srcPath);
            ETString dstFSPath = stripMediaPrefix(dstPath);

            ETFile srcFile = srcFS->open(srcFSPath, FILE_MODE_READ);
            if (!srcFile.isOpen())
                return false;
            ETString content = srcFile.readAll();
            srcFile.close();
            ETFile dstFile = dstFS->open(dstFSPath, FILE_MODE_WRITE, true);
            if (!dstFile.isOpen())
                return false;
            bool ok = dstFile.writeAll(content);
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
            ETString fsPath = stripMediaPrefix(path);
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
            for (const auto &kv : mountPoints_)
            {
                ETString prefix = kv.first + "/";
                if (path.isChildOf(prefix) == 0)
                    return path.relativeTo(prefix);
            }
            return path;
        }

    private:
        ETMap<Path, IStorageMedia *> mountPoints_;
    };
}

#endif // ET_STORAGE_SYSTEM_H