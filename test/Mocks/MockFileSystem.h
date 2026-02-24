#ifndef MOCKFILESYSTEM_H
#define MOCKFILESYSTEM_H

#include "../../src/interfaces/IFileSystem.h"
#include "../../src/ETTypes.h"
#include "MockFile.h"

using namespace EmbeddedTerminal;

class MockFileSystem : public IFileSystem
{
public:
    unsigned long long _capacity = 1024 * 1024 * 1024;  // 1GB
    unsigned long long _totalBytes = 1024 * 1024 * 512; // 512 MB
    unsigned long long _usedBytes = 256 * 1024 * 1024;  // 256 MB
    ETMap<ETString, std::shared_ptr<MockFile>>
        files;

    MockFileSystem() : files()
    {
        createDirectory("/"); // Ensure root directory exists
    }

    bool exists(const char *path) override
    {
        return files.find(path) != files.end();
    }
    bool exists(const ETString &path) override
    {
        return exists(path.c_str());
    }
    bool mkdir(const char *path) override
    {
        if (files.find(path) != files.end())
            return false; // Already exists
        files[path] = std::make_shared<MockFile>(path, "", 0);
        files[path]->_isDirectory = true;
        return true;
    }
    bool mkdir(const ETString &path) override
    {
        return mkdir(path.c_str());
    }
    bool rmdir(const char *path) override
    {
        if (isEmpty(path))
        {
            files.erase(path);
            return true;
        }
        return false;
    }
    bool rmdir(const ETString &path) override
    {
        return rmdir(path.c_str());
    }
    bool remove(const char *path) override
    {
        if (files.find(path) == files.end())
            return false; // Does not exist
        files.erase(path);

        return true;
    }
    bool remove(const ETString &path) override
    {
        return remove(path.c_str());
    }
    ETVector<ETString> list(const char *path) const override
    {
        ETVector<ETString> result;
        ETString dirPath = path;
        if (dirPath.empty())
            dirPath = "/";
        // Ensure trailing slash for root and non-root dirs
        if (dirPath != "/" && dirPath.back() != '/')
            dirPath += "/";

        for (const auto &entry : files)
        {
            const ETString &key = entry.first;
            if (key.length() <= dirPath.length())
                continue;
            if (key.substr(0, dirPath.length()) == dirPath)
            {
                ETString rest = key.substr(dirPath.length());
                // Only direct children: no further slashes
                if (!rest.empty() && !rest.contains('/'))
                {
                    result.push_back(entry.second->name());
                }
            }
            // Special case for root directory
            else if (dirPath == "/" && key.find('/', 1) == ETString::npos && key != "/")
            {
                result.push_back(entry.second->name());
            }
        }
        return result;
    }
    ETVector<ETString> list(const ETString &path) const override
    {
        return list(path.c_str());
    }
    ETFile open(const char *path, const char *mode = FILE_MODE_READ, const bool create = false) override
    {
        if (exists(path))
        {
            auto file = files[path];
            file->open = true; // Ensure open flag is set
            // DON'T reset position - position is persistent across open/close for same file
            return ETFile(file);
        }
        return ETFile(std::make_shared<MockFile>(path));
    }
    ETFile open(const ETString &path, const char *mode = FILE_MODE_READ, const bool create = false) override { return open(path.c_str(), mode, create); }

    unsigned long long capacity() const override { return _capacity; }
    unsigned long long totalBytes() const override { return _totalBytes; } // 512MB
    unsigned long long usedBytes() const override { return _usedBytes; }   // 256MB

    void createFile(const ETString &path, const ETString &content, size_t size = 0)
    {
        auto realPath = path;
        if (path[0] != '/')
        {
            realPath = "/" + path;
        }
        files[realPath] = std::make_shared<MockFile>(extractName(realPath), content, size);
        createDirectory(dirname(realPath));
    }

    void createDirectory(const ETString &path)
    {
        auto realPath = path;
        if (path[0] != '/')
        {
            realPath = "/" + path;
        }
        if (files.find(realPath) != files.end())
            return; // Already exists

        files[realPath] = std::make_shared<MockFile>(extractName(realPath), "", 0);
        files[realPath]->_isDirectory = true;
        createDirectory(dirname(realPath));
    }

    bool isDirectory(const char *path) override
    {
        auto it = files.find(path);
        if (it != files.end())
        {
            return it->second->_isDirectory;
        }
        return false;
    }

    bool isEmpty(const char *path) override
    {
        ETString dirPath = path;
        if (dirPath.empty())
            dirPath = "/";
        // Ensure trailing slash for root and non-root dirs
        if (dirPath != "/" && dirPath.back() != '/')
            dirPath += "/";

        for (const auto &entry : files)
        {
            const ETString &key = entry.first;
            if (key.length() <= dirPath.length())
                continue;
            if (key.substr(0, dirPath.length()) == dirPath)
            {
                ETString rest = key.substr(dirPath.length());
                // Only direct children: no further slashes
                if (!rest.empty() && rest.find('/') == ETString::npos)
                {
                    return false;
                }
            }
            // Special case for root directory
            else if (dirPath == "/" && key.find('/', 1) == ETString::npos && key != "/")
            {
                return false;
            }
        }
        return true;
    }

    ETString extractName(ETString path)
    {
        if (path.empty() || path == "/")
            return "";
        // Remove trailing slash except for root
        if (path.length() > 1 && path.back() == '/')
            path.pop_back();
        size_t pos = path.find_last_of('/');
        if (pos == ETString::npos)
            return path;
        return path.substr(pos + 1);
    }

    ETString dirname(ETString path)
    {
        if (path.empty() || path == "/")
            return "/";

        // 1) Strip trailing slashes, but leave a single leading slash if that's all we have.
        size_t end = path.length();
        while (end > 1 && path[end - 1] == '/')
            --end;

        // 2) Find the last slash in [0..end-1]
        auto pos = path.find_last_of('/', end - 1);
        if (pos == std::string::npos)
        {
            // no slash => the “directory” is root
            return "/";
        }
        if (pos == 0)
        {
            // the only slash is at the front => parent is "/"
            return "/";
        }

        // 3) Return everything up to (but not including) that slash
        return path.substr(0, pos);
    }
};

#endif // MOCKFILESYSTEM_H
