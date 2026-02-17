// Platform-independent file system interface
#ifndef IFILESYSTEM_H
#define IFILESYSTEM_H

#include "ETTypes.h"
#include "ETFile.h"
namespace EmbeddedTerminal
{

    class IFileSystem
    {
    public:
        virtual ~IFileSystem() {}

        virtual ETFile open(const char *path, const char *mode = FILE_MODE_READ, const bool create = false) = 0;
        virtual ETFile open(const ETString &path, const char *mode = FILE_MODE_READ, const bool create = false)
        {
            return open(path.c_str(), mode, create);
        };
        virtual bool exists(const char *path) = 0;
        virtual bool exists(const ETString &path) { return exists(path.c_str()); };

        virtual bool isDirectory(const char *path) = 0;
        virtual bool isDirectory(const ETString &path) { return isDirectory(path.c_str()); };

        // Checks if the directory or file is empty
        virtual bool isEmpty(const char *path) = 0;
        virtual bool isEmpty(const ETString &path) { return isEmpty(path.c_str()); };

        virtual bool remove(const char *path) = 0;
        virtual bool remove(const ETString &path) { return remove(path.c_str()); };

        virtual bool mkdir(const char *path) = 0;
        virtual bool mkdir(const ETString &path) { return mkdir(path.c_str()); };

        virtual bool rmdir(const char *path) = 0;
        virtual bool rmdir(const ETString &path) { return rmdir(path.c_str()); };

        virtual ETVector<ETString> list(const char *path) const = 0;
        virtual ETVector<ETString> list(const ETString &path) const { return list(path.c_str()); };

        virtual unsigned long long capacity() const = 0;
        virtual unsigned long long totalBytes() const = 0;
        virtual unsigned long long usedBytes() const = 0;
    };
}
#endif // IFILESYSTEM_H
