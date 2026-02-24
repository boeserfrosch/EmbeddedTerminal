#ifndef NATIVE_FILE_SYSTEM_H
#define NATIVE_FILE_SYSTEM_H

#if __cplusplus >= 201703L

#include "NativeFile.h"
#include "interfaces/IFileSystem.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <system_error>
#include <cerrno>
#include <filesystem>
namespace fs = std::filesystem;
namespace EmbeddedTerminal
{
    class NativeFileSystem : public IFileSystem
    {
    public:
        ETFile open(const char *path, const char *mode = FILE_MODE_READ, const bool create = false) override
        {
            return ETFile(std::make_shared<NativeFile>(path, mode, create));
        }

        bool exists(const char *path) override
        {
            return std::filesystem::exists(path);
        }

        bool isDirectory(const char *path) override
        {
            return std::filesystem::is_directory(path);
        }

        bool isEmpty(const char *path) override
        {
            if (!std::filesystem::exists(path))
                return true;
            if (std::filesystem::is_regular_file(path))
                return std::filesystem::file_size(path) == 0;
            if (std::filesystem::is_directory(path))
                return std::filesystem::is_empty(path);
            return true;
        }

        bool remove(const char *path) override
        {
            return std::filesystem::remove(path);
        }

        bool mkdir(const char *path) override
        {
            return std::filesystem::create_directories(path);
        }

        bool rmdir(const char *path) override
        {
            return std::filesystem::remove_all(path) > 0;
        }

        ETVector<ETString> list(const char *path) const override
        {
            ETVector<ETString> result;
            if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
                return result;

            for (const auto &entry : std::filesystem::directory_iterator(path))
            {
                auto p = entry.path();
                result.push_back(p.filename().string());
            }
            return result;
        }

        unsigned long long capacity() const override
        {
            try
            {
                auto spaceInfo = std::filesystem::space(std::filesystem::current_path());
                return static_cast<unsigned long long>(spaceInfo.capacity);
            }
            catch (...)
            {
                return 0;
            }
        }

        unsigned long long totalBytes() const override
        {
            try
            {
                auto spaceInfo = std::filesystem::space(std::filesystem::current_path());
                return static_cast<unsigned long long>(spaceInfo.capacity);
            }
            catch (...)
            {
                return 0;
            }
        }

        unsigned long long usedBytes() const override
        {
            try
            {
                auto spaceInfo = std::filesystem::space(std::filesystem::current_path());
                return static_cast<unsigned long long>(spaceInfo.capacity - spaceInfo.available);
            }
            catch (...)
            {
                return 0;
            }
        }
    };
} // namespace EmbeddedTerminal
#endif
#endif