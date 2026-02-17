#ifndef FILE_H
#define FILE_H

#include "interfaces/IFile.h"
#include "interfaces/IFileSystem.h"
#include "ETTypes.h"
#if (defined(ESP_PLATFORM) || defined(ESP32)) && not defined(ARDUINO)
namespace EmbeddedTerminal
{
    class File : public IFile
    {
    }
};
#elif defined(ARDUINO)
#include <FS.h>
#include <SD.h>

namespace EmbeddedTerminal
{
    class File : public IFile
    {
    private:
        ::File _file; // Arduino File object
        ETString _path;
        bool _isDir;

    public:
        File(const ETString &path, const char *mode = FILE_MODE_READ, bool create = false)
            : _path(path), _isDir(false)
        {
            auto openMode = FILE_READ;
            if (mode[0] == 'r')
                openMode = FILE_READ;
            else if (mode[0] == 'w')
                openMode = FILE_WRITE;
            else if (mode[0] == 'a')
                openMode = FILE_APPEND;

            _file = SD.open(path.c_str(), openMode);

            if (!_file && create)
            {
                // Try to create the file if it doesn't exist
                _file = SD.open(path.c_str(), FILE_WRITE);
                _file.close();
                _file = SD.open(path.c_str(), openMode);
            }

            if (_file && _file.isDirectory())
            {
                _isDir = true;
            }
        }

        ~File()
        {
            close();
        }

        int read() override
        {
            if (!_file)
                return -1;
            return _file.read();
        }

        size_t read(void *buffer, size_t size) override
        {
            if (!_file)
                return 0;
            return _file.read(reinterpret_cast<uint8_t *>(buffer), size);
        }

        size_t write(unsigned char c) override
        {
            if (!_file)
                return 0;
            return _file.write(c);
        }

        size_t write(const void *buffer, size_t size) override
        {
            if (!_file)
                return 0;
            return _file.write(reinterpret_cast<const uint8_t *>(buffer), size);
        }

        ETString readString() override
        {
            return readAll();
        }

        ETString readAll() override
        {
            if (!_file)
                return "";

            ETString result;
            while (_file.available())
            {
                result.push_back(static_cast<char>(_file.read()));
            }
            return result;
        }

        bool writeAll(const ETString &content) override
        {
            if (!_file)
                return false;

            _file.seek(0);
            size_t written = _file.write(reinterpret_cast<const uint8_t *>(content.c_str()), content.length());
            return written == content.length();
        }

        bool seek(size_t position) override
        {
            if (!_file)
                return false;
            return _file.seek(position);
        }

        size_t position() const override
        {
            if (!_file)
                return 0;
            return _file.position();
        }

        size_t size() const override
        {
            if (!_file)
                return 0;
            return _file.size();
        }

        void close() override
        {
            if (_file)
            {
                _file.close();
            }
        }

        bool isOpen() const override
        {
            return static_cast<bool>(_file);
        }

        ETString name() const override
        {
            if (!_file)
                return "";
            return ETString(_file.name());
        }

        ETString path() const override
        {
            return _path;
        }
    };
}
#endif
#endif // FILE_H
