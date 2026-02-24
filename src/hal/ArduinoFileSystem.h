#pragma once
#ifdef ARDUINO
#include "interfaces/IFileSystem.h"
#include "hal/ArduinoFile.h"
#include <FS.h>

namespace EmbeddedTerminal
{

    class ArduinoFileSystem : public IFileSystem
    {
    public:
        explicit ArduinoFileSystem(FS &fs) : _mount(&fs) {}
        virtual ~ArduinoFileSystem() = default;

        ETFile open(const char *path, const char *mode = FILE_MODE_READ, const bool create = false) override
        {
            if (!_mount)
                return ETFile();

            File f;
            // map modes r/w/a to Arduino mode strings
            if (strcmp(mode, FILE_MODE_READ) == 0)
            {
                f = _mount->open(path, "r");
            }
            else if (strcmp(mode, FILE_MODE_WRITE) == 0)
            {
                // create/truncate
                if (create)
                {
                    // open for write (create)
                    f = _mount->open(path, "w");
                }
                else
                {
                    f = _mount->open(path, "r+");
                }
            }
            else if (strcmp(mode, FILE_MODE_APPEND) == 0)
            {
                f = _mount->open(path, "a");
            }
            else
            {
                f = _mount->open(path, mode);
            }

            if (!f)
                return ETFile();

            // name: last path component
            ETString name;
            const char *p = strrchr(path, '/');
            if (p)
                name = ETString(p + 1);
            else
                name = ETString(path);

            auto filePtr = std::make_shared<ArduinoFile>(f, name, ETString(path));
            return ETFile(filePtr);
        }

        bool exists(const char *path) override { return _mount && _mount->exists(path); }

        bool isDirectory(const char *path) override
        {
            if (!_mount)
                return false;
            File f = _mount->open(path);
            bool res = f && f.isDirectory();
            f.close();
            return res;
        }

        bool isEmpty(const char *path) override
        {
            if (!_mount)
                return true;
            File dir = _mount->open(path);
            if (!dir || !dir.isDirectory())
            {
                // if file, empty if size==0
                File f = _mount->open(path);
                bool r = (f && f.size() == 0);
                f.close();
                return r;
            }
            // directory: check first entry
            File entry = dir.openNextFile();
            bool empty = !entry;
            if (entry)
                entry.close();
            dir.close();
            return empty;
        }

        bool remove(const char *path) override { return _mount && _mount->remove(path); }
        bool mkdir(const char *path) override { return _mount && _mount->mkdir(path); }
        bool rmdir(const char *path) override { return _mount && _mount->rmdir(path); }

        ETVector<ETString> list(const char *path) const override
        {
            ETVector<ETString> files;
            if (!_mount)
                return files;
            File dir = _mount->open(path);
            if (!dir || !dir.isDirectory())
                return files;

            ETString name = dir.getNextFileName();
            do
            {
                files.push_back(name);
                name = dir.getNextFileName();
            } while (!name.empty());

            dir.close();
            return files;
        }

        unsigned long long capacity() const override
        {
            // Not all FS expose capacity via API — returning totalBytes()
            return totalBytes();
        }
        unsigned long long totalBytes() const override
        {
            // Some platforms provide totalBytes; attempt to query via FS class if available
            // Fallback: 0
#ifdef ESP32
            return _mount ? _mount->totalBytes() : 0;
#else
            (void)_mount;
            return 0;
#endif
        }
        unsigned long long usedBytes() const override
        {
#ifdef ESP32
            return _mount ? _mount->usedBytes() : 0;
#else
            (void)_mount;
            return 0;
#endif
        }

    protected:
        FS *_mount;
    };

} // namespace EmbeddedTerminal
#endif // ARDUINO
