#pragma once
#ifdef ESP32
#include "ETFile.h"
#include "interfaces/IFileSystem.h"
#include "SD_MMC.h"
#include "ArduinoFile.h"

namespace EmbeddedTerminal
{
    class SDMMCFileSystem : public IFileSystem
    {
    private:
    public:
        SDMMCFileSystem()
        {
        }
        ~SDMMCFileSystem() = default;

        ETFile open(const char *path, const char *mode = FILE_MODE_READ, const bool create = false) override
        {
            return ETFile(std::make_shared<ArduinoFile>(SD_MMC.open(path, mode, create)));
        }

        bool exists(const char *path) override
        {
            return SD_MMC.exists(path);
        }

        bool isDirectory(const char *path) override
        {
            return SD_MMC.exists(path) ? SD_MMC.open(path).isDirectory() : false;
        }

        // Checks if the directory or file is empty
        virtual bool isEmpty(const char *path) override
        {
            if (!SD_MMC.exists(path))
            {
                return false;
            }
            auto file = SD_MMC.open(path);
            if (file.isDirectory())
            {
                return file.getNextFileName().length() == 0;
            }
            return file.size() == 0;
        }

        bool remove(const char *path) override
        {
            return SD_MMC.remove(path);
        }

        bool mkdir(const char *path) override
        {
            return SD_MMC.mkdir(path);
        }

        bool rmdir(const char *path) override
        {
            return rmdir(path);
        }

        ETVector<ETString> list(const char *path) const override
        {
            ETVector<ETString> content;
            if (!SD_MMC.exists(path))
                return content;

            auto f = SD_MMC.open(path);
            ETString fName = f.getNextFileName();
            while (!fName.empty())
            {
                content.push_back(fName.substr(fName.find_last_of("/\\")));
                f = SD_MMC.open(fName);
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
} // namespace EmbeddedTerminal
#endif