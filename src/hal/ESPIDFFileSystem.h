#pragma once
#if defined(ESP_PLATFORM) || defined(ESP_32)
#include "interfaces/IFileSystem.h"
#include "hal/ESPIDFFile.h"
#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>
#include <cstring>
#include <string>

namespace EmbeddedTerminal
{
    class ESPIDFFileSystem : public IFileSystem
    {
    public:
        explicit ESPIDFFileSystem(const Path &mount_point) : mount_point_(mount_point) {}
        virtual ~ESPIDFFileSystem() = default;

        ETFile open(const Path &path, const char *mode = FILE_MODE_READ, const bool create = false) override
        {
            auto full_path = this->fullPath_(path);
            struct stat st;
            if (!create && stat(full_path, &st) != 0)
            {
                return ETFile();
            }
            FILE *f = nullptr;
            if (create)
            {
                f = fopen(full_path, "r");
                if (!f)
                {
                    f = fopen(full_path, "w");
                    if (f)
                        fclose(f);
                }
            }
            f = fopen(full_path, mode);
            if (!f)
                return ETFile();
            return ETFile(std::make_shared<ESPIDFFile>(f));
        }

        bool exists(const Path &path) override
        {
            Path full_path = fullPath_(path);
            struct stat st;
            return stat(full_path, &st) == 0;
        }

        bool isDirectory(const Path &path) override
        {
            Path full_path = fullPath_(path);
            struct stat st;
            return stat(full_path, &st) == 0 && S_ISDIR(st.st_mode);
        }

        bool isEmpty(const Path &path) override
        {
            Path full_path = fullPath_(path);
            struct stat st;
            if (stat(full_path, &st) != 0)
                return false;
            if (S_ISDIR(st.st_mode))
            {
                DIR *dir = opendir(full_path);
                if (!dir)
                    return false;
                struct dirent *entry;
                bool empty = true;
                while ((entry = readdir(dir)) != nullptr)
                {
                    if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
                    {
                        empty = false;
                        break;
                    }
                }
                closedir(dir);
                return empty;
            }
            if (S_ISREG(st.st_mode))
                return st.st_size == 0;
            return false;
        }

        bool remove(const Path &path) override
        {
            Path full_path = fullPath_(path);
            struct stat st;
            if (stat(full_path, &st) != 0 || !S_ISREG(st.st_mode))
                return false;
            return ::remove(full_path) == 0;
        }

        bool mkdir(const Path &path) override
        {
            Path full_path = fullPath_(path);
            struct stat st;
            if (stat(full_path, &st) == 0)
                return S_ISDIR(st.st_mode);
            return ::mkdir(full_path, S_IRWXU | S_IRWXG | S_IRWXO) == 0;
        }
        bool rmdir(const Path &path) override
        {
            Path full_path = fullPath_(path);
            struct stat st;
            if (stat(full_path, &st) != 0 || !S_ISDIR(st.st_mode))
                return false;
            return ::rmdir(full_path) == 0;
        }

        ETVector<Path> list(const Path &path, const ETString &prefix = "") const override
        {
            ETVector<Path> files;
            Path full_path = fullPath_(path);
            struct stat st;
            if (stat(full_path, &st) != 0 || !S_ISDIR(st.st_mode))
                return files;
            DIR *dir = opendir(full_path);
            if (!dir)
                return files;
            struct dirent *entry;
            while ((entry = readdir(dir)) != nullptr)
            {
                if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
                {
                    Path p = Path(".") + Path(entry->d_name);
                    if (prefix.empty() || p.getName().startsWith(prefix))
                    {
                        files.push_back(p);
                    }
                }
            }
            closedir(dir);
            return files;
        }

    protected:
        Path fullPath_(const Path &path) const
        {
            Path p = path;
            if (p.isAbsolute())
                p = Path(".") + p; // Make it relative to mount point

            Path full_path = mount_point_ + p;
            return full_path;
        }

    private:
        Path mount_point_;
    };
}
#endif
