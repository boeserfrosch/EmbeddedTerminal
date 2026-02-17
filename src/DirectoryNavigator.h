#ifndef DIRECTORYNAVIGATOR_H
#define DIRECTORYNAVIGATOR_H

#include "interfaces/IFileSystem.h"
#include "ETTypes.h"

namespace EmbeddedTerminal
{

    class DirectoryNavigator
    {
    public:
        DirectoryNavigator(IFileSystem *fs, const ETString &root = "/")
            : fs_(fs), currentDir_(root)
        {
            if (!fs_ || !fs_->exists(currentDir_))
            {
                currentDir_ = "/";
            }
        }

        ETFile open(const char *path, const char *mode = FILE_MODE_READ, const bool create = false) const
        {
            if (fs_)
            {
                return fs_->open(resolvePath(path), mode, create);
            }
            return ETFile();
        }
        ETFile open(const ETString &path, const char *mode = FILE_MODE_READ, const bool create = false) const
        {
            return open(path.c_str(), mode, create);
        }

        bool exists(const char *path) const
        {
            if (fs_)
            {
                return fs_->exists(resolvePath(path).c_str());
            }
            return false;
        }
        bool exists(const ETString &path) const
        {
            return exists(path.c_str());
        }

        // Change directory (cd)
        bool cd(const char *path)
        {
            ETString newPath = resolvePath(path);
            if (fs_ && fs_->exists(newPath) && fs_->isDirectory(newPath.c_str()))
            {
                currentDir_ = newPath;
                return true;
            }
            return false;
        }
        bool cd(const ETString &path)
        {
            return cd(path.c_str());
        }

        // Get current directory (pwd)
        ETString pwd() const
        {
            return currentDir_;
        }

        ETString pwd(const char *path) const
        {
            return resolvePath(path);
        }
        ETString pwd(const ETString &path) const
        {
            return pwd(path.c_str());
        }

        // List contents (ls)
        ETVector<ETString> ls() const
        {
            return ls(currentDir_.c_str());
        }

        ETVector<ETString> ls(const ETString &path) const
        {
            return ls(path.c_str());
        }
        ETVector<ETString> ls(const char *path) const
        {
            if (fs_)
            {
                return fs_->list(resolvePath(path).c_str());
            }
            return ETVector<ETString>();
        }

        // Make directory (mkdir)
        bool mkdir(const ETString &path)
        {
            return mkdir(path.c_str());
        }

        bool mkdir(const char *path)
        {
            if (fs_)
            {
                return fs_->mkdir(resolvePath(path));
            }
            return false;
        }

        // Remove directory (rmdir)
        bool rmdir(const ETString &path) { return rmdir(path.c_str()); }
        bool rmdir(const char *path)
        {
            if (fs_)
            {
                return fs_->rmdir(resolvePath(path));
            }
            return false;
        }

        bool isEmpty(const char *path)
        {
            if (fs_)
            {
                return fs_->isEmpty(resolvePath(path));
            }
            return false;
        }
        bool isEmpty(ETString path) { return isEmpty(path.c_str()); }

        // Remove file (remove)
        bool remove(const ETString &path)
        {
            return remove(path.c_str());
        }
        bool remove(const char *path)
        {
            if (fs_)
            {
                return fs_->remove(resolvePath(path));
            }
            return false;
        }

        bool isDirectory(const char *path) const
        {
            if (fs_)
            {
                return fs_->isDirectory(resolvePath(path));
            }
            return false;
        }
        bool isDirectory(ETString path) const
        {
            return isDirectory(path.c_str());
        }

    private:
        IFileSystem *fs_;
        ETString currentDir_;

    protected:
        // Resolve relative/absolute paths
        ETString resolvePath(const char *path) const
        {
            if (path == nullptr || path[0] == '\0')
                return currentDir_;
            ETString tempPath;
            if (path[0] == '/')
                tempPath = path; // Absolute
            else
            {
                // Simple join for relative paths
                if (currentDir_.back() == '/')
                {
                    tempPath = currentDir_ + path;
                }
                else
                    tempPath = currentDir_ + "/" + path;
            }

            // Remove . and ..
            ETVector<ETString> parts;
            size_t start = 1;
            size_t end = 0;
            while ((end = tempPath.find('/', start)) != ETString::npos)
            {
                ETString part = tempPath.substr(start, end - start);
                if (part == "..")
                {
                    if (!parts.empty())
                        parts.pop_back();
                }
                else if (part != "." && !part.empty())
                {
                    parts.push_back(part);
                }
                start = end + 1;
            }
            if (start < tempPath.length())
            {
                ETString part = tempPath.substr(start);
                if (part == "..")
                {
                    if (!parts.empty())
                        parts.pop_back();
                }
                else if (part != "." && !part.empty())
                {
                    parts.push_back(part);
                }
            }
            return "/" + join(parts, "/");
        }
    };

}
#endif // DIRECTORYNAVIGATOR_H
