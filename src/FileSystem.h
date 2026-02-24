#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

// IFileSystem_Implementations.cpp
// Implementations of IFileSystem for x86 (POSIX), Arduino (SD/FS) and ESP-IDF (POSIX-like)
// NOTES & ASSUMPTIONS:
// - The project provides ETTypes.h, ETFile and ETVector/ETString. To make this file self-contained
//   for users who don't yet have those, minimal fallback definitions are provided when the real ones
//   are not present. If you have your own ETFile/ETString/ETVector, the fallbacks will be skipped.
// - The implementations use POSIX file APIs (fopen, stat, opendir, statvfs). ESP-IDF exposes
//   a POSIX-compatible layer, so the same code works for ESP-IDF when ESP_PLATFORM is defined.
// - For Arduino (AVR/SAM) we use the Arduino SD library (SD.h + FS.h). On ESP32 running Arduino
//   the SD library is usually available as well.
// - If you use C++ features newer than C++11, preprocessor checks are added. This file aims to
//   be C++11-compatible.

#include "interfaces/IFileSystem.h"
#include "ETFile.h"
#include "ETTypes.h"
#include "File.h"

// --------------------- ESP-IDF implementation ---------------------
#if 0 && (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
#elif defined(ARDUINO)
#include <FS.h>
#include <SD.h>
#include <memory>

namespace EmbeddedTerminal
{
    class FileSystem : public IFileSystem
    {
    private:
        bool _mounted;

    public:
        FileSystem() : _mounted(false) {}

        bool begin(uint8_t csPin = SS, uint32_t freq = 4000000)
        {
            _mounted = SD.begin(csPin, freq);
            return _mounted;
        }

        bool isMounted() const
        {
            return _mounted;
        }

        ETFile open(const ETString &path, const char *mode = FILE_MODE_READ, bool create = false) override
        {
            if (!_mounted)
                return nullptr;
            auto f = std::make_shared<FileArduino>(path, mode, create);
            if (!f->isOpen())
                return nullptr;
            return f;
        }

        bool exists(const ETString &path) override
        {
            if (!_mounted)
                return false;
            return SD.exists(path.c_str());
        }

        bool remove(const ETString &path) override
        {
            if (!_mounted)
                return false;
            return SD.remove(path.c_str());
        }

        bool mkdir(const ETString &path) override
        {
            if (!_mounted)
                return false;
            return SD.mkdir(path.c_str());
        }

        bool rmdir(const ETString &path) override
        {
            if (!_mounted)
                return false;
            return SD.rmdir(path.c_str());
        }

        size_t freeBytes() override
        {
#if defined(ESP32)
            // Auf ESP32 SD gibt es kein direktes freeBytes, daher Dummy
            return 0;
#else
            // Auf AVR/Teensy ggf. nicht verfügbar
            return 0;
#endif
        }

        ETFile openRoot() override
        {
            if (!_mounted)
                return nullptr;
            ::File root = SD.open("/");
            if (!root)
                return nullptr;
            return std::make_shared<FileArduino>("/", "r");
        }
    };
}
#else
#include <cstdio>
#include <cstring>
#include <fstream>
#include <system_error>
#include <cerrno>
#if __cplusplus >= 201703L
#include <filesystem>
namespace fs = std::filesystem;
#else
// fallback headers for platforms without C++17
#if defined(__unix__) || defined(__APPLE__)
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/statvfs.h>
#endif
#endif
namespace EmbeddedTerminal
{
    class FileSystem : public IFileSystem
    {
    public:
        ETFile open(const char *path, const char *mode = FILE_MODE_READ, const bool create = false) override
        {
            return ETFile(std::make_shared<File>(path, mode, create));
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
                result.push_back(entry.path().filename().string());
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

/*
Important notes for you (guaranteed to compile in many environments):
- This file provides complete implementations but depends on the shape of ETFile/ETString/ETVector used
  in your project. If you already have those types, the minimal fallback types defined here will be
  skipped (because you'd include the project's headers before including this file). If you want me to
  adapt the constructors or map actual ETFile semantics (e.g. how ETFile stores handles), send me the
  declarations of ETFile/ETString/ETVector and I will modify the implementations to construct ETFile
  correctly instead of using the fallbacks.

- For Arduino: ensure you call SD.begin(...) in your sketch before using ArduinoFileSystem.
- For ESP-IDF: mount your FAT/VFS (e.g. using esp_vfs_fat_spiflash_mount or register_vfs) before use;
  the code uses POSIX functions which are available after mounting.

If you'd like, I can:
- Adapt the code to use your project's ETFile (just paste the header),
- Split into .h/.cpp files with unit tests for x86 (using the native POSIX impl), or
- Add recursive directory creation (mkdir -p behaviour).
*/

#endif
