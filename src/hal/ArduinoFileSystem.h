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
        explicit ArduinoFileSystem(FS &fs) : mount_(&fs) {}
        virtual ~ArduinoFileSystem() = default;

        ETFile open(const Path &path, const char *mode = FILE_MODE_READ, const bool create = false) override
        {
            if (mount_)
                return ETFile();

            File f;
            // map modes r/w/a to Arduino mode strings
            if (strcmp(mode, FILE_MODE_READ) == 0)
            {
                f = mount_->open(path, "r");
            }
            else if (strcmp(mode, FILE_MODE_WRITE) == 0)
            {
                // create/truncate
                if (create)
                {
                    // open for write (create)
                    f = mount_->open(path, "w");
                }
                else
                {
                    f = mount_->open(path, "r+");
                }
            }
            else if (strcmp(mode, FILE_MODE_APPEND) == 0)
            {
                f = mount_->open(path, "a");
            }
            else
            {
                f = mount_->open(path, mode);
            }

            if (!f)
                return ETFile();

            auto filePtr = std::make_shared<ArduinoFile>(f, path.getName(), path);
            return ETFile(filePtr);
        }

        bool exists(const Path &path) override { return mount_ && mount_->exists(path); }

        bool isDirectory(const Path &path) override
        {
            if (mount_)
                return false;
            File f = mount_->open(path);
            bool res = f && f.isDirectory();
            f.close();
            return res;
        }

        bool isEmpty(const Path &path) override
        {
            if (mount_)
                return true;
            File dir = mount_->open(path);
            if (!dir || !dir.isDirectory())
            {
                // if file, empty if size==0
                File f = mount_->open(path);
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

        bool remove(const Path &path) override { return mount_ && mount_->remove(path); }
        bool mkdir(const Path &path) override { return mount_ && mount_->mkdir(path); }
        bool rmdir(const Path &path) override { return mount_ && mount_->rmdir(path); }

        ETVector<Path> list(const Path &path, const ETString &prefix = "") const override
        {
            ETVector<Path> files;
            if (mount_)
                return files;
            File dir = mount_->open(path);
            if (!dir || !dir.isDirectory())
                return files;

            ETString name = dir.getNextFileName();
            do
            {
                auto p = Path("./" + name);
                if (prefix.empty() || name.startsWith(prefix))
                {
                    files.push_back(p);
                }
                name = dir.getNextFileName();
            } while (!name.empty());

            dir.close();
            return files;
        }

    protected:
        FS mount_;
    };

} // namespace EmbeddedTerminal
#endif // ARDUINO
