#ifndef DIRECTORYNAVIGATOR_H
#define DIRECTORYNAVIGATOR_H

#include "interfaces/IStorage.h"
#include "interfaces/IDirectoryNavigator.h"
#include "ETTypes.h"

namespace EmbeddedTerminal
{

    class DirectoryNavigator : public IDirectoryNavigator
    {
    public:
        DirectoryNavigator(IStorageSystem *storage, const Path &root = Path::root())
            : storage_(storage), currentDir_(root)
        {
            if (!storage_ || !storage_->exists(currentDir_))
            {
                currentDir_ = Path::root();
            }
            if (!currentDir_.isAbsolute())
            {
                currentDir_ = Path::root();
            }
            if (currentDir_.isRoot() || ETString(currentDir_) == "/")
            {
                currentDir_ = Path::root();
            }
        }

        ETFile open(const Path &path, const char *mode = FILE_MODE_READ, const bool create = false) const override
        {
            if (storage_)
            {
                return storage_->open(resolvePath(path), mode, create);
            }
            return ETFile();
        }

        bool exists(const Path &path) const override
        {
            if (storage_)
            {

                return storage_->exists(resolvePath(path));
            }
            return false;
        }
        bool cd(const Path &path) override
        {
            if (!storage_)
                return false;

            auto newPath = resolvePath(path);
            // Special case: absolute root path
            if (path.isRoot() || newPath.isRoot())
            {
                if (storage_->exists(Path::root()) && storage_->isDirectory(Path::root()))
                {
                    currentDir_ = Path::root();
                    return true;
                }
                return false;
            }
            if (storage_->exists(newPath) && storage_->isDirectory(newPath))
            {
                currentDir_ = newPath;
                return true;
            }
            return false;
        }

        Path pwd() const override
        {
            return currentDir_;
        }

        Path pwd(const Path &path) const override
        {
            return resolvePath(path);
        }

        ETVector<Path> ls() const override
        {
            return ls(currentDir_);
        }

        ETVector<Path> ls(const Path &path, const ETString &prefix = "") const override
        {
            if (!storage_)
            {
                return ETVector<Path>();
            }

            return storage_->list(resolvePath(path), prefix);
        }

        bool mkdir(const Path &path) override
        {
            if (storage_)
            {
                return storage_->mkdir(resolvePath(path));
            }
            return false;
        }

        bool rmdir(const Path &path) override
        {
            if (storage_)
            {
                return storage_->rmdir(resolvePath(path));
            }
            return false;
        }

        bool isEmpty(const Path &path) override
        {
            if (storage_)
            {
                return storage_->isEmpty(resolvePath(path));
            }
            return false;
        }

        bool remove(const Path &path) override
        {
            if (storage_)
            {
                return storage_->remove(resolvePath(path));
            }
            return false;
        }

        bool isDirectory(const Path &path) const override
        {
            if (storage_)
            {
                return storage_->isDirectory(resolvePath(path));
            }
            return false;
        }

        IStorageSystem *getStorageSystem() const override
        {
            return storage_;
        }

    private:
        IStorageSystem *storage_;
        Path currentDir_;

    protected:
        Path resolvePath(const Path &path) const
        {
            if (path.isEmpty() || path.isRoot())
                return currentDir_;
            if (path.isAbsolute())
            {
                return path.isRoot() ? Path::root() : path;
            }
            return currentDir_ + path;
        }
    };

}
#endif // DIRECTORYNAVIGATOR_H
