
#pragma once

#include "ETFile.h"
#include "interfaces/IFileSystem.h"

#if defined(ARDUINO)
#include "SD_MMC.h"
#include "ArduinoFile.h"

namespace EmbeddedTerminal
{
    class SDMMCFileSystem : public IFileSystem
    {
    public:
        SDMMCFileSystem() {}
        ~SDMMCFileSystem() = default;

        ETFile open(const char *path, const char *mode = FILE_MODE_READ, const bool create = false) override
        {
            if (!create && !SD_MMC.exists(path))
                return ETFile();
            auto f = SD_MMC.open(path, mode, create);
            if (!f)
                return ETFile();
            return ETFile(std::make_shared<ArduinoFile>(f));
        }

        bool exists(const char *path) override
        {
            return SD_MMC.exists(path);
        }

        bool isDirectory(const char *path) override
        {
            auto f = SD_MMC.open(path);
            if (!f)
                return false;
            bool isDir = f.isDirectory();
            f.close();
            return isDir;
        }

        virtual bool isEmpty(const char *path) override
        {
            auto f = SD_MMC.open(path);
            if (!f)
                return false;
            if (f.isDirectory())
            {
                bool empty = f.getNextFileName().length() == 0;
                f.close();
                return empty;
            }
            bool empty = f.size() == 0;
            f.close();
            return empty;
        }

        bool remove(const char *path) override
        {
            if (!SD_MMC.exists(path))
                return false;
            return SD_MMC.remove(path);
        }

        bool mkdir(const char *path) override
        {
            return SD_MMC.mkdir(path);
        }

        bool rmdir(const char *path) override
        {
            if (!SD_MMC.exists(path))
                return false;
            return SD_MMC.rmdir(path);
        }

        ETVector<ETString> list(const char *path) const override
        {
            ETVector<ETString> content;
            auto f = SD_MMC.open(path);
            if (!f)
                return content;
            ETString fName = f.getNextFileName();
            while (!fName.empty())
            {
                content.push_back(fName.substr(fName.find_last_of("/\\")));
                f = SD_MMC.open(fName.c_str());
                fName = f.getNextFileName();
            }
            return content;
        }

        unsigned long long capacity() const override
        {
            return SD_MMC.cardSize();
        }
        unsigned long long totalBytes() const override
        {
            return SD_MMC.totalBytes();
        }
        unsigned long long usedBytes() const override
        {
            return SD_MMC.usedBytes();
        }
    };
}

#elif defined(ESP_PLATFORM) || defined(ESP_32)

#include "ESPIDFFile.h"
#include <sys/stat.h>
#include "sdmmc_cmd.h"
#include "ff.h"

#include <dirent.h>

namespace EmbeddedTerminal
{
    class SDMMCFileSystem : public IFileSystem
    {
    public:
        SDMMCFileSystem(sdmmc_card_t *card, const ETString &mount_point) : card_(card), mount_point_(mount_point) {}
        ~SDMMCFileSystem() = default;

        ETFile open(const char *path, const char *mode = FILE_MODE_READ, const bool create = false) override
        {
            ETString full_path = mount_point_ + ETString((path[0] == '/') ? path : ETString("/") + path);
            struct stat st;
            if (!create && stat(full_path.c_str(), &st) != 0)
            {
                return ETFile();
            }
            FILE *f = nullptr;
            if (create)
            {
                f = fopen(full_path.c_str(), "r");
                if (!f)
                {
                    f = fopen(full_path.c_str(), "w");
                    if (f)
                    {
                        fclose(f);
                    }
                }
            }
            f = fopen(full_path.c_str(), mode);
            if (!f)
            {
                return ETFile();
            }
            return ETFile(std::make_shared<ESPIDFFile>(f));
        }

        bool exists(const char *path) override
        {
            ETString full_path = mount_point_ + ETString(path[0] == '/' ? path : ETString("/") + path);
            struct stat st;
            return stat(full_path.c_str(), &st) == 0;
        }

        bool isDirectory(const char *path) override
        {
            ETString full_path = mount_point_ + ETString(path[0] == '/' ? path : ETString("/") + path);
            struct stat st;
            return stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
        }

        virtual bool isEmpty(const char *path) override
        {
            ETString full_path = mount_point_ + ETString(path[0] == '/' ? path : ETString("/") + path);
            struct stat st;
            if (stat(full_path.c_str(), &st) != 0)
                return false;
            if (S_ISDIR(st.st_mode))
            {
                DIR *dir = opendir(full_path.c_str());
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

        bool remove(const char *path) override
        {
            ETString full_path = mount_point_ + ETString(path[0] == '/' ? path : ETString("/") + path);
            struct stat st;
            if (stat(full_path.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
                return false;
            return ::remove(full_path.c_str()) == 0;
        }

        bool mkdir(const char *path) override
        {
            UnityPrint("Attempting to create directory: ");
            UnityPrint(path);
            ETString full_path = mount_point_ + ETString(path[0] == '/' ? path : ETString("/") + path);
            struct stat st;
            if (stat(full_path.c_str(), &st) == 0)
            {
                // Return true if it already exists as a directory, false if it exists but is not a directory
                UnityPrint("Directory already exists: ");
                UnityPrint(full_path.c_str());
                UnityPrint(", is it a directory? ");
                UnityPrint(S_ISDIR(st.st_mode) ? "Yes" : "No");
                UnityPrint("\n");
                return S_ISDIR(st.st_mode);
            }
            UnityPrint("Creating directory: ");
            UnityPrint(full_path.c_str());
            UnityPrint("\n");

            return ::mkdir(full_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0;
        }

        bool rmdir(const char *path) override
        {
            ETString full_path = mount_point_ + ETString(path[0] == '/' ? path : ETString("/") + path);
            struct stat st;
            if (stat(full_path.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
                return false;
            return ::rmdir(full_path.c_str()) == 0;
        }

        ETVector<ETString> list(const char *path) const override
        {
            ETVector<ETString> content;
            ETString full_path = mount_point_ + ETString(path[0] == '/' ? path : ETString("/") + path);
            struct stat st;
            if (stat(full_path.c_str(), &st) != 0 || !S_ISDIR(st.st_mode))
                return content;
            DIR *dir = opendir(full_path.c_str());
            if (!dir)
                return content;
            struct dirent *entry;
            while ((entry = readdir(dir)) != nullptr)
            {
                if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
                    content.push_back(ETString(entry->d_name));
            }
            closedir(dir);
            return content;
        }

        unsigned long long capacity() const override
        {
            return card_->csd.capacity * card_->csd.sector_size;
        }

        unsigned long long totalBytes() const override
        {
            FATFS *fsinfo;
            DWORD fre_clust;
            if (f_getfree("0:", &fre_clust, &fsinfo) != 0)
                return 0;
            uint64_t size = ((uint64_t)(fsinfo->csize)) * (fsinfo->n_fatent - 2)
#if _MAX_SS != 512
                            * (fsinfo->ssize);
#else
                            * 512;
#endif
            return size;
        }

        unsigned long long usedBytes() const override
        {
            FATFS *fsinfo;
            DWORD fre_clust;
            if (f_getfree("0:", &fre_clust, &fsinfo) != 0)
                return 0;
            uint64_t size = ((uint64_t)(fsinfo->csize)) * ((fsinfo->n_fatent - 2) - (fsinfo->free_clst))
#if _MAX_SS != 512
                            * (fsinfo->ssize);
#else
                            * 512;
#endif
            return size;
        }

        sdmmc_card_t *card() const
        {
            return card_;
        }
        const char *mount_point() const { return mount_point_.c_str(); }

    private:
        sdmmc_card_t *card_;
        ETString mount_point_;
    };
}

#endif