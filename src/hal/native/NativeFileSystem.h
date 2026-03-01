#ifndef NATIVE_FILE_SYSTEM_H
#define NATIVE_FILE_SYSTEM_H

#if defined(__cplusplus) && __cplusplus >= 201703L

#include "hal/native/NativeFile.h"
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
        ETFile open(const Path &path, const char *mode = FILE_MODE_READ, const bool create = false) override
        {
            if (create)
            {
                fs::path fsPath(path.c_str());
                fs::create_directories(fsPath.parent_path());
                std::ofstream ofs(fsPath, std::ios::app);
                ofs.close();
            }

            auto openMode = std::ios::in;
            if (strcmp(mode, FILE_MODE_READ) == 0)
                openMode = std::ios::in;
            else if (strcmp(mode, "w") == 0)
                openMode = std::ios::out | std::ios::trunc;
            else if (strcmp(mode, "a") == 0)
                openMode = std::ios::out | std::ios::app;
            else
                return ETFile();

            return ETFile(std::make_shared<NativeFile>(std::fstream(path.c_str(), openMode), false));
        }

        bool exists(const Path &path) override
        {
            return std::filesystem::exists(path.c_str());
        }

        bool isDirectory(const Path &path) override
        {
            return std::filesystem::is_directory(path.c_str());
        }

        bool isEmpty(const Path &path) override
        {
            if (!std::filesystem::exists(path.c_str()))
                return true;
            if (std::filesystem::is_regular_file(path.c_str()))
                return std::filesystem::file_size(path.c_str()) == 0;
            if (std::filesystem::is_directory(path.c_str()))
                return std::filesystem::is_empty(path.c_str());
            return true;
        }

        bool remove(const Path &path) override
        {
            return std::filesystem::remove(path.c_str());
        }

        bool mkdir(const Path &path) override
        {
            return std::filesystem::create_directories(path.c_str());
        }

        bool rmdir(const Path &path) override
        {
            return std::filesystem::remove_all(path.c_str()) > 0;
        }

        ETVector<Path> list(const Path &path, const ETString &prefix = "") const override
        {
            ETVector<Path> result;
            if (!std::filesystem::exists(ETString(path).c_str()) || !std::filesystem::is_directory(ETString(path).c_str()))
                return result;

            for (const auto &entry : std::filesystem::directory_iterator(ETString(path).c_str()))
            {
                auto p = entry.path();
                Path pathObj = Path(p.filename().string());
                if (prefix.empty() || pathObj.getName().startsWith(prefix))
                {
                    result.push_back(pathObj);
                }
            }
            return result;
        }
    };
} // namespace EmbeddedTerminal
#endif
#endif
